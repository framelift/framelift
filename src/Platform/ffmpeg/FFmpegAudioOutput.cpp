#include "FFmpegAudioOutput.h"

#include "FFmpegClock.h"

#include <framelift/Log.h>

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
}

#include <SDL3/SDL.h>

#include <algorithm>

FFmpegAudioOutput::FFmpegAudioOutput()
{
    // Refcounted — the host already holds an SDL_INIT_AUDIO ref; this keeps the
    // class self-contained if that ever changes.
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        Log::Warn("FFmpegAudioOutput: SDL_InitSubSystem(AUDIO) failed: {}", SDL_GetError());
    }
}

FFmpegAudioOutput::~FFmpegAudioOutput()
{
    Close();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool FFmpegAudioOutput::Open(int srcRate, const AVChannelLayout& srcLayout, AVSampleFormat srcFmt)
{
    Close();

    std::lock_guard lock(mutex_);

    dstRate_ = srcRate;
    dstChannels_ = 2;
    bytesPerSec_ = dstRate_ * dstChannels_ * static_cast<int>(sizeof(float));

    AVChannelLayout dstLayout;
    av_channel_layout_default(&dstLayout, dstChannels_);
    SwrContext* swr = nullptr;
    const int ret = swr_alloc_set_opts2(
        &swr, &dstLayout, AV_SAMPLE_FMT_FLT, dstRate_, &srcLayout, srcFmt, srcRate, 0, nullptr
    );
    av_channel_layout_uninit(&dstLayout);
    if (ret < 0 || !swr)
    {
        Log::Error("FFmpegAudioOutput: swr_alloc_set_opts2 failed");
        bytesPerSec_ = 0;
        return false;
    }
    if (swr_init(swr) < 0)
    {
        Log::Error("FFmpegAudioOutput: swr_init failed");
        swr_free(&swr);
        bytesPerSec_ = 0;
        return false;
    }
    swr_ = swr;

    SDL_AudioSpec spec{};
    spec.format = SDL_AUDIO_F32;
    spec.channels = dstChannels_;
    spec.freq = dstRate_;
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!stream_)
    {
        Log::Error("FFmpegAudioOutput: SDL_OpenAudioDeviceStream failed: {}", SDL_GetError());
        swr_free(&swr_);
        bytesPerSec_ = 0;
        return false;
    }

    lastQueuedPts_ = 0.0;
    ApplyGainLocked();
    SDL_ResumeAudioStreamDevice(stream_);
    Log::Info("FFmpegAudioOutput: opened {} Hz stereo F32", dstRate_);
    return true;
}

void FFmpegAudioOutput::Feed(const AVFrame* frame, double ptsSec)
{
    std::lock_guard lock(mutex_);
    if (!stream_ || !swr_ || !frame)
    {
        return;
    }

    const int inRate = frame->sample_rate > 0 ? frame->sample_rate : dstRate_;
    // Output sample count incl. samples still buffered inside the resampler.
    const int64_t maxOut = av_rescale_rnd(swr_get_delay(swr_, inRate) + frame->nb_samples, dstRate_, inRate, AV_ROUND_UP);
    if (maxOut <= 0)
    {
        return;
    }

    const auto byteCap = static_cast<size_t>(maxOut) * dstChannels_ * sizeof(float);
    if (buffer_.size() < byteCap)
    {
        buffer_.resize(byteCap);
    }

    uint8_t* outPtr = buffer_.data();
    const int converted = swr_convert(
        swr_, &outPtr, static_cast<int>(maxOut), const_cast<const uint8_t**>(frame->extended_data), frame->nb_samples
    );
    if (converted <= 0)
    {
        return;
    }

    const int bytes = converted * dstChannels_ * static_cast<int>(sizeof(float));
    if (!SDL_PutAudioStreamData(stream_, buffer_.data(), bytes))
    {
        Log::Warn("FFmpegAudioOutput: SDL_PutAudioStreamData failed: {}", SDL_GetError());
        return;
    }

    lastQueuedPts_ = ptsSec + static_cast<double>(converted) / static_cast<double>(dstRate_);
}

double FFmpegAudioOutput::MasterClock() const
{
    std::lock_guard lock(mutex_);
    const int64_t queued = stream_ ? SDL_GetAudioStreamQueued(stream_) : 0;
    return ComputeMasterClock(lastQueuedPts_, queued, bytesPerSec_);
}

int64_t FFmpegAudioOutput::QueuedBytes() const
{
    std::lock_guard lock(mutex_);
    return stream_ ? SDL_GetAudioStreamQueued(stream_) : 0;
}

bool FFmpegAudioOutput::HasDevice() const
{
    std::lock_guard lock(mutex_);
    return stream_ != nullptr;
}

void FFmpegAudioOutput::SetPaused(bool paused)
{
    std::lock_guard lock(mutex_);
    if (!stream_)
    {
        return;
    }
    if (paused)
    {
        SDL_PauseAudioStreamDevice(stream_);
    }
    else
    {
        SDL_ResumeAudioStreamDevice(stream_);
    }
}

void FFmpegAudioOutput::SetVolume(int volume0to100)
{
    std::lock_guard lock(mutex_);
    volume_ = std::clamp(volume0to100, 0, 100);
    ApplyGainLocked();
}

void FFmpegAudioOutput::SetMute(bool muted)
{
    std::lock_guard lock(mutex_);
    muted_ = muted;
    ApplyGainLocked();
}

void FFmpegAudioOutput::ApplyGainLocked()
{
    if (!stream_)
    {
        return;
    }
    SDL_SetAudioStreamGain(stream_, muted_ ? 0.0f : static_cast<float>(volume_) / 100.0f);
}

void FFmpegAudioOutput::Flush()
{
    std::lock_guard lock(mutex_);
    if (stream_)
    {
        SDL_ClearAudioStream(stream_);
    }
    lastQueuedPts_ = 0.0;
}

void FFmpegAudioOutput::Close()
{
    std::lock_guard lock(mutex_);
    if (stream_)
    {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
    if (swr_)
    {
        swr_free(&swr_);
    }
    bytesPerSec_ = 0;
    lastQueuedPts_ = 0.0;
}
