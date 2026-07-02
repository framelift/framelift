#include "FFmpegPlayer.h"

#include "FFmpegPlayerInternal.h"

#include "FFmpegAudioOutput.h"
#include "FFmpegError.h"
#include "FFmpegHwDecode.h"
#include "FFmpegPacketQueue.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace ffplay_detail;


// Pin the FFmpeg-free mirrors in FFmpegError.h against the real AVERROR_* values, so a
// future libav change (or a typo in a mirror) is a compile error here rather than a
// silent misclassification at runtime. ENOENT/EACCES use the errno convention AVERROR(e).
static_assert(kAvErrInvalidData == AVERROR_INVALIDDATA, "AVERROR_INVALIDDATA mirror drifted");
static_assert(kAvErrEof == AVERROR_EOF, "AVERROR_EOF mirror drifted");
static_assert(kAvErrDemuxerNotFound == AVERROR_DEMUXER_NOT_FOUND, "AVERROR_DEMUXER_NOT_FOUND mirror drifted");
static_assert(kAvErrDecoderNotFound == AVERROR_DECODER_NOT_FOUND, "AVERROR_DECODER_NOT_FOUND mirror drifted");
static_assert(kAvErrProtocolNotFound == AVERROR_PROTOCOL_NOT_FOUND, "AVERROR_PROTOCOL_NOT_FOUND mirror drifted");
static_assert(kAvErrStreamNotFound == AVERROR_STREAM_NOT_FOUND, "AVERROR_STREAM_NOT_FOUND mirror drifted");
static_assert(kAvErrBsfNotFound == AVERROR_BSF_NOT_FOUND, "AVERROR_BSF_NOT_FOUND mirror drifted");
static_assert(kAvErrNoEnt == AVERROR(ENOENT), "AVERROR(ENOENT) mirror drifted");
static_assert(kAvErrAccess == AVERROR(EACCES), "AVERROR(EACCES) mirror drifted");

// ── Demux session ─────────────────────────────────────────────────────────────

void FFmpegPlayer::PlayFile(const std::string& path, double resumePos)
{
    FRAMELIFT_PERF_START("file-open");
    FRAMELIFT_PERF_START("file-load-metadata");
    QueueEvent(MakeLifecycle(MediaEventType::StartFile));
    SetIdle(false);
    eofReached_ = false;
    paused_ = false;
    droppedFrames_ = 0;
    mistimedFrames_ = 0;
    decodeErrors_ = 0;
    cache_.ResetMetrics();
    displayWidth_ = 0;
    displayHeight_ = 0;
    seekSkipPts_ = -1e18; // no carry-over skip from a prior file's seek
    seekRefresh_ = false;
    {
        std::lock_guard lock(mutex_);
        hwDecName_ = "no"; // reset until the video decoder is (re)armed below
        videoClock_.Reset();
        subtitleSeekClockOverrideActive_ = false;
        hasPendingSeek_ = false; // discard any seek queued before this load
        hasPendingAudioSwitch_ = false;
        hasPendingSubSwitch_ = false;
    }
    {
        // Drop the previous file's track snapshot up front so a failed open doesn't
        // leave stale entries in the host's track menus.
        std::lock_guard lock(tracksMutex_);
        tracks_.clear();
        selectedAudioId_ = -1;
        selectedSubId_ = -1;
    }
    EmitFlag(PlayerProperty::Pause, false);
    EmitFlag(PlayerProperty::EofReached, false);
    EmitFlag(PlayerProperty::Seeking, false);
    UpdateCoreIdle();

    AVFormatContext* fmt = nullptr;
    InstallFFmpegLogCallback(); // Qt may have clobbered the global callback since construction.
    int openRet = 0;
    {
        PerfScope perf("file-open-input");
        openRet = avformat_open_input(&fmt, path.c_str(), nullptr, nullptr);
    }
    if (openRet < 0)
    {
        Log::Error("FFmpegPlayer: failed to open {}", path);
        QueueEvent(MakeEndFile(ClassifyAvError(openRet)));
        FRAMELIFT_PERF_END("file-load-metadata");
        FRAMELIFT_PERF_END("file-open");
        EmitPlaybackSummary("open-error");
        return;
    }
    int streamInfoRet = 0;
    {
        PerfScope perf("file-stream-info");
        streamInfoRet = avformat_find_stream_info(fmt, nullptr);
    }
    if (streamInfoRet < 0)
    {
        Log::Error("FFmpegPlayer: failed to read stream info for {}", path);
        avformat_close_input(&fmt);
        QueueEvent(MakeEndFile(ClassifyAvError(streamInfoRet)));
        FRAMELIFT_PERF_END("file-load-metadata");
        FRAMELIFT_PERF_END("file-open");
        EmitPlaybackSummary("stream-info-error");
        return;
    }

    const AVCodec* vCodec = nullptr;
    const int vIdx = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, &vCodec, 0);
    const int defaultAudioIdx = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    // ── Video decoder (optional, immutable for this file) ─────────────────────
    // hw is declared after vDec so it is destroyed *after* avcodec_free_context(&vDec)
    // (the decoder unwinds through the device ctx / get_format on free).
    AVCodecContext* vDec = nullptr;
    FFmpegHwDecode hw;
    AVStream* vStream = nullptr;
    int W = 0;
    int H = 0;
    hasVideo_ = false;
    {
        PerfScope perf("video-decoder-open");
        if (vIdx >= 0 && vCodec)
        {
            vStream = fmt->streams[vIdx];
            vDec = avcodec_alloc_context3(vCodec);
            if (vDec)
            {
                avcodec_parameters_to_context(vDec, vStream->codecpar);
                vDec->pkt_timebase = vStream->time_base;
                // Try the selected hardware decode mode; all modes fall back cleanly to
                // software if the codec, device, or renderer interop path is unavailable.
                (void)TryEnableHardwareDecode(vCodec, vDec, hw);
                vDec->thread_count = hw.Active() ? 1 : 0; // 0 = auto-detect for software decode
                if (avcodec_open2(vDec, vCodec, nullptr) == 0 && vDec->width > 0 && vDec->height > 0)
                {
                    W = vDec->width;
                    H = vDec->height;
                    hasVideo_ = true;
                    displayWidth_ = W;
                    displayHeight_ = H;
                    {
                        std::lock_guard lock(mutex_);
                        hwDecName_ = hw.Active() ? hw.DeviceName() : "no";
                    }
                }
                else
                {
                    Log::Warn("FFmpegPlayer: video decoder unavailable; audio-only playback");
                    avcodec_free_context(&vDec);
                }
            }
        }
    }

    // ── Discover sidecar files and build the audio/subtitle track list ────────
    {
        PerfScope perf("track-discovery");
        ScanExternalSources(path);
        BuildTrackList(fmt, defaultAudioIdx);
    }

    // ── Bind the default audio + subtitle selections ──────────────────────────
    AudioBinding aud;
    {
        PerfScope perf("audio-bind");
        OpenAudioBinding(selectedAudioId_, fmt, aud);
    }

    int subIdx = -1;
    AVCodecContext* sDec = nullptr;
    AVStream* sStream = nullptr;
    {
        PerfScope perf("subtitle-bind");
        OpenSubtitleBinding(selectedSubId_, path, fmt, subIdx, sDec, sStream);
    }

    if (!hasVideo_ && !aud.dec)
    {
        Log::Error("FFmpegPlayer: failed to open any decoder for {}", path);
        avcodec_free_context(&vDec);
        OpenAudioBinding(-1, fmt, aud); // tears down any partial audio binding
        OpenSubtitleBinding(-1, path, fmt, subIdx, sDec, sStream);
        avformat_close_input(&fmt);
        QueueEvent(MakeEndFile(EndFileReason::NoStream));
        FRAMELIFT_PERF_END("file-load-metadata");
        FRAMELIFT_PERF_END("file-open");
        EmitPlaybackSummary("no-stream");
        return;
    }

    // Something plays, but a present stream may have been dropped because its decoder was
    // unavailable — tell the user so the silent audio-only / video-only fallback isn't a
    // mystery. (The both-failed case returned NoStream above, so these are exclusive.)
    if (vIdx >= 0 && !hasVideo_ && aud.dec)
    {
        QueueEvent(MakeNotice(MediaNoticeKind::VideoUnsupported));
    }
    else if (defaultAudioIdx >= 0 && !aud.dec && hasVideo_)
    {
        QueueEvent(MakeNotice(MediaNoticeKind::AudioUnsupported));
    }

    // Duration (seconds): container first, then the playing stream's own duration.
    double durationSec = 0.0;
    if (fmt->duration != AV_NOPTS_VALUE && fmt->duration > 0)
    {
        durationSec = static_cast<double>(fmt->duration) / AV_TIME_BASE;
    }
    else if (hasVideo_ && vStream->duration != AV_NOPTS_VALUE)
    {
        durationSec = static_cast<double>(vStream->duration) * av_q2d(vStream->time_base);
    }
    else if (aud.dec && aud.stream->duration != AV_NOPTS_VALUE)
    {
        durationSec = static_cast<double>(aud.stream->duration) * av_q2d(aud.stream->time_base);
    }
    duration_ = durationSec;

    const AVDictionaryEntry* titleTag = av_dict_get(fmt->metadata, "title", nullptr, 0);
    {
        std::lock_guard lock(mutex_);
        currentPath_ = path;
        mediaTitle_ = titleTag && titleTag->value ? titleTag->value : std::filesystem::path(path).filename().string();
    }
    QueueEvent(MakeLifecycle(MediaEventType::FileLoaded));
    FRAMELIFT_PERF_END("file-load-metadata");
    EmitDouble(PlayerProperty::Duration, durationSec);
    if (aud.dec)
    {
        QueueEvent(MakeLifecycle(MediaEventType::AudioReconfig));
    }

    // Still image: single video frame, no audio — held per image-display-duration.
    const bool isImage = hasVideo_ && !aud.dec && vStream->nb_frames == 1;

    // Why a frame finished playing, decided after the workers are joined.
    enum class Reason
    {
        Eof,  // demuxer reached end of file
        Stop, // shutdown or a new file load
        Seek, // a seek was requested
    };

    // Absolute target for the next session iteration; NaN ⇒ play from here.
    double seekTo = resumePos > 0.0 ? resumePos : std::numeric_limits<double>::quiet_NaN();
    AVPacket* pkt = av_packet_alloc();
    bool stop = false;

    while (!stop)
    {
        // ── Apply a pending audio/subtitle track switch (workers are joined) ───
        {
            bool doAudio = false;
            bool doSub = false;
            int64_t audId = -1;
            int64_t subId = -1;
            {
                std::lock_guard lock(mutex_);
                doAudio = hasPendingAudioSwitch_;
                audId = pendingAudioId_;
                hasPendingAudioSwitch_ = false;
                doSub = hasPendingSubSwitch_;
                subId = pendingSubId_;
                hasPendingSubSwitch_ = false;
            }
            if (doAudio)
            {
                OpenAudioBinding(audId, fmt, aud);
                if (aud.dec)
                {
                    QueueEvent(MakeLifecycle(MediaEventType::AudioReconfig));
                }
            }
            if (doSub)
            {
                OpenSubtitleBinding(subId, path, fmt, subIdx, sDec, sStream);
            }
        }

        // ── Apply a pending seek (single-threaded here: workers are joined) ────
        const bool timingSeekApply = !std::isnan(seekTo);
        if (timingSeekApply)
        {
            FRAMELIFT_PERF_START("seek-apply");
        }
        if (!std::isnan(seekTo))
        {
            const auto ts = static_cast<int64_t>(seekTo * AV_TIME_BASE);
            // A failed seek leaves the demuxer position untouched. Flushing the
            // decoders / clock anyway would blank the picture for a seek that never
            // happened, so only tear down and refresh when the seek actually landed.
            const int ret = av_seek_frame(fmt, -1, ts, AVSEEK_FLAG_BACKWARD);
            if (ret < 0)
            {
                Log::Warn("FFmpegPlayer: seek to {}s failed ({})", seekTo, ret);
            }
            else
            {
                if (aud.external && aud.fmt && av_seek_frame(aud.fmt, -1, ts, AVSEEK_FLAG_BACKWARD) < 0)
                {
                    Log::Warn("FFmpegPlayer: external audio seek to {}s failed", seekTo);
                }
                if (vDec)
                {
                    avcodec_flush_buffers(vDec);
                }
                if (aud.dec)
                {
                    avcodec_flush_buffers(aud.dec);
                }
                if (sDec)
                {
                    avcodec_flush_buffers(sDec);
                }
                // Embedded subtitles re-decode from the seek point; drop their buffered
                // events. External subs are pre-loaded with absolute times — leave them.
                if (subIdx >= 0)
                {
                    subtitles_->FlushEvents();
                }
                audioOut_->Flush();
                {
                    std::lock_guard lock(mutex_);
                    videoClock_.Reset();
                    // Every applied seek (the RequestSeek kick *and* a demux-driven
                    // re-seek during a held burst) resets the clocks here, so both gates
                    // must re-arm: seekSettled_ so the worker re-paints before the next
                    // re-seek, and seekClockValid_ so a held repeat anchors to seekTarget_
                    // instead of reading GetMasterClock()==0 and re-targeting from 0.
                    seekSettled_ = false;
                    seekClockValid_ = false;
                    subtitleSeekClockOverride_ = seekTo;
                    subtitleSeekClockOverrideActive_ = true;
                }
                // Exact (hr-seek): drop decoded frames before the target. Keyframe seek
                // (-inf) presents straight from the keyframe the demuxer landed on.
                seekSkipPts_ = hrSeek_.load() ? seekTo : -1e18;
                seekRefresh_ = true;
                eofReached_ = false;
                EmitFlag(PlayerProperty::EofReached, false);
                QueueEvent(MakeLifecycle(MediaEventType::PlaybackRestart));
                subtitles_->ForceNextUpdate();
                RequestRender();
            }
            // Clear the pending seek and the UI "seeking" state regardless of outcome,
            // so a failed seek doesn't leave the player stuck mid-seek.
            seekTo = std::numeric_limits<double>::quiet_NaN();
            EmitFlag(PlayerProperty::Seeking, false);
            UpdateCoreIdle();
        }

        // ── Spawn workers, then demux until EOF / stop / a new seek ────────────
        audioQ_->Flush();
        videoQ_->Flush();
        subQ_->Flush();
        // Clear the read-ahead accounting + abort flag (the queues were aborted on
        // the seek that brought us here) so the demuxer can refill for this read.
        cache_.Reset();
        if (timingSeekApply)
        {
            FRAMELIFT_PERF_END("seek-apply");
        }
        if (aud.dec)
        {
            audioThread_ = std::thread(&FFmpegPlayer::AudioWorker, this, aud.dec, aud.stream, aud.startOffset);
        }
        if (hasVideo_)
        {
            videoThread_ = std::thread(&FFmpegPlayer::VideoWorker, this, vDec, vStream, W, H, &hw);
        }
        if (subIdx >= 0 && sDec)
        {
            subtitleThread_ = std::thread(&FFmpegPlayer::SubtitleWorker, this, sDec, sStream);
        }
        if (aud.external && aud.fmt)
        {
            extAudioThread_ = std::thread(&FFmpegPlayer::ExternalAudioDemux, this, aud.fmt, aud.streamIndex);
        }

        Reason reason = Reason::Eof;
        for (;;)
        {
            if (StopRequested())
            {
                reason = Reason::Stop;
                break;
            }
            {
                std::lock_guard lock(mutex_);
                // Hold off re-seeking until the current seek has painted a frame, so a
                // burst of held-key repeats steps visibly instead of restarting the
                // decoder before anything is shown. seekSettled_ flips on the present.
                if (hasPendingSeek_ && seekSettled_)
                {
                    reason = Reason::Seek;
                    break;
                }
            }
            if (av_read_frame(fmt, pkt) < 0)
            {
                reason = Reason::Eof;
                break;
            }
            bool pushed = true;
            if (hasVideo_ && pkt->stream_index == vIdx)
            {
                pushed = videoQ_->Push(pkt);
            }
            else if (aud.dec && !aud.external && pkt->stream_index == aud.streamIndex)
            {
                pushed = audioQ_->Push(pkt);
            }
            else if (subIdx >= 0 && pkt->stream_index == subIdx)
            {
                pushed = subQ_->Push(pkt);
            }
            av_packet_unref(pkt);
            if (!pushed)
            {
                // A queue was aborted — a stop or seek is pending; re-resolve which.
                reason = StopRequested() ? Reason::Stop : Reason::Seek;
                break;
            }
        }

        // ── Stop the workers (drain on EOF, abort otherwise) and join ──────────
        if (reason == Reason::Eof)
        {
            videoQ_->SignalEof();
            subQ_->SignalEof();
            // External audio runs on its own clock; abort it (and any backlog) at the
            // video container's EOF so its demux thread can't block the join.
            if (aud.external)
            {
                audioQ_->Abort();
            }
            else
            {
                audioQ_->SignalEof();
            }
        }
        else
        {
            audioQ_->Abort();
            videoQ_->Abort();
            subQ_->Abort();
        }
        if (audioThread_.joinable())
        {
            audioThread_.join();
        }
        if (videoThread_.joinable())
        {
            videoThread_.join();
        }
        if (subtitleThread_.joinable())
        {
            subtitleThread_.join();
        }
        if (extAudioThread_.joinable())
        {
            extAudioThread_.join();
        }

        if (reason == Reason::Stop)
        {
            break;
        }
        if (reason == Reason::Seek)
        {
            seekTo = TakePendingSeek();
            QueueEvent(MakeLifecycle(MediaEventType::Seek));
            EmitFlag(PlayerProperty::Seeking, true);
            continue;
        }

        // ── reason == Eof ─────────────────────────────────────────────────────
        // Decide whether this end advances the playlist (EndFile) or just holds.
        bool emitEnd = true;
        if (isImage && imageDisplayDuration_.load() <= 0.0)
        {
            emitEnd = false; // still image, infinite hold — never auto-advances
        }
        else if (isImage)
        {
            // Slideshow still: hold the configured duration unless interrupted.
            const auto until =
                std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                                       std::chrono::duration<double>(imageDisplayDuration_.load())
                                                   );
            std::unique_lock lock(mutex_);
            if (cv_.wait_until(
                    lock, until,
                    [this]
                    {
                        return shutdown_.load() || hasPendingLoad_ || hasPendingSeek_ || stopRequested_;
                    }
                ))
            {
                emitEnd = false; // a seek/stop arrived first — handle it below
            }
        }
        if (emitEnd)
        {
            eofReached_ = true;
            EmitFlag(PlayerProperty::EofReached, true);
            UpdateCoreIdle();
            QueueEvent(MakeEndFile(EndFileReason::Eof));
        }

        // keep-open: hold the last frame here until a seek or stop. A playlist that
        // advances on EndFile arrives as a new load (Stop); a manual seek resumes;
        // an end-of-playlist Stop() sets stopRequested_ to break the hold and go idle.
        {
            std::unique_lock lock(mutex_);
            cv_.wait(
                lock,
                [this]
                {
                    return shutdown_.load() || hasPendingLoad_ || hasPendingSeek_ || stopRequested_;
                }
            );
        }
        if (StopRequested())
        {
            break;
        }
        seekTo = TakePendingSeek();
        QueueEvent(MakeLifecycle(MediaEventType::Seek));
        EmitFlag(PlayerProperty::Seeking, true);
    }

    av_packet_free(&pkt);
    // Keep the audio device open when another file is already queued (the common playlist
    // advance): the next PlayFile's OpenAudioBinding reuses the still-running QAudioSink
    // when the output format matches, avoiding a device close/reopen per file. Only tear it
    // down when we're genuinely stopping (shutdown), where the next open won't follow.
    bool advancing = false;
    const char* summaryReason = "stop";
    {
        std::lock_guard lock(mutex_);
        advancing = hasPendingLoad_;
        if (shutdown_.load())
        {
            summaryReason = "shutdown";
        }
        else if (hasPendingLoad_)
        {
            summaryReason = "superseded";
        }
        else if (!stopRequested_)
        {
            summaryReason = "ended";
        }
    }
    EmitPlaybackSummary(summaryReason);
    if (!advancing)
    {
        audioOut_->Close();
    }
    avcodec_free_context(&vDec);
    if (aud.dec)
    {
        avcodec_free_context(&aud.dec);
    }
    if (aud.external && aud.fmt)
    {
        avformat_close_input(&aud.fmt);
    }
    if (sDec)
    {
        avcodec_free_context(&sDec);
    }
    subtitles_->ClearTrack();
    avformat_close_input(&fmt);
    {
        std::lock_guard lock(tracksMutex_);
        tracks_.clear();
        externalSources_.clear();
        selectedAudioId_ = -1;
        selectedSubId_ = -1;
    }
}

bool FFmpegPlayer::TryEnableHardwareDecode(const AVCodec* codec, AVCodecContext* dec, FFmpegHwDecode& hw)
{
    if (!hwdec_.load())
    {
        return false;
    }

    const auto tryMode = [&](VideoDecodeMode mode, bool warnUnavailable) -> bool
    {
        switch (mode)
        {
        case VideoDecodeMode::VulkanZeroCopy:
#if FRAMELIFT_MODULE_GRAPHICS_VULKAN
            return vulkanZeroCopyAvailable_ && vkHwDevice_ && hw.TryEnableVulkan(codec, dec, vkHwDevice_);
#else
            break;
#endif
        case VideoDecodeMode::Vulkan:
#if !FRAMELIFT_MODULE_GRAPHICS_VULKAN
            break;
#endif
        case VideoDecodeMode::Cuda:
        case VideoDecodeMode::D3D11VA:
        case VideoDecodeMode::DXVA2:
        case VideoDecodeMode::VAAPI:
            return hw.TryEnableBackend(codec, dec, HwBackendFromVideoDecodeMode(mode));
        case VideoDecodeMode::CudaZeroCopy:
            return hw.TryEnableCudaZeroCopy(codec, dec, warnUnavailable);
        case VideoDecodeMode::Off:
        case VideoDecodeMode::Auto:
            break;
        }
        return false;
    };

    const VideoDecodeMode mode = videoDecodeMode_.load();
    if (mode == VideoDecodeMode::Off)
    {
        return false;
    }
    if (mode != VideoDecodeMode::Auto)
    {
        return tryMode(mode, true);
    }

    for (const VideoDecodeMode candidate : AutoVideoDecodePreference())
    {
        if (candidate == VideoDecodeMode::Off)
        {
            break;
        }
#if FRAMELIFT_MODULE_GRAPHICS_VULKAN
        // Vulkan video decode faults on the NVIDIA driver; let Auto fall through to NVDEC.
        if (candidate == VideoDecodeMode::VulkanZeroCopy && vulkanAdapterIsNvidia_)
        {
            continue;
        }
#endif
        if (tryMode(candidate, false))
        {
            return true;
        }
    }
    return false;
}
