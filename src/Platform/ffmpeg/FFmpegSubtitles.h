#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

// libass-backed subtitle pipeline for the FFmpeg backend (issue #8, Phase 5).
//
// This is an "FFmpeg* sibling" — together with FFmpegPlayer it is one of the only
// translation units permitted to include <ass/ass.h> / <libav*/...>. It owns the
// libass library + renderer + a single active track, guarded by a mutex so the
// subtitle decode thread (ProcessPacket / BeginTrack) and the render thread
// (RenderOverlay) never touch libass concurrently.
//
// Decoding is uniform across formats: libavcodec decodes ASS/SSA and SRT alike into
// AVSubtitle rects carrying ASS markup (it synthesises an ASS header for text subs),
// which is fed to ass_process_chunk. Rendering composites the resulting ASS_Image
// list into a straight-alpha RGBA8 overlay sized to the on-screen video rectangle.

// Opaque libass / libav types (defined only in the .cpp).
struct ass_library;
struct ass_renderer;
struct ass_track;
struct AVCodecContext;
struct AVPacket;
struct AVFormatContext;

class FFmpegSubtitles
{
public:
    // Outcome of a render: whether the caller should upload + draw the overlay, and
    // whether the pixel buffer was rewritten (so an unchanged frame skips re-upload).
    enum class RenderResult : std::uint8_t
    {
        None,      // nothing to draw this frame — do not composite the overlay
        Unchanged, // same as the previous frame — keep drawing the existing texture
        Updated,   // outRgba was rewritten — re-upload before drawing
    };

    FFmpegSubtitles();
    ~FFmpegSubtitles();

    FFmpegSubtitles(const FFmpegSubtitles&) = delete;
    FFmpegSubtitles& operator=(const FFmpegSubtitles&) = delete;

    // True when libass initialised (library + renderer). When false, all operations
    // are no-ops and RenderOverlay returns None.
    [[nodiscard]] bool Ok() const noexcept;

    // Start a fresh track seeded with a decoder's ASS codec header (header may be
    // null / size 0). Replaces any current track.
    void BeginTrack(const unsigned char* header, int headerSize);

    // Decode one subtitle packet and append its events to the current track.
    // tbNum/tbDen are the subtitle stream's time_base.
    void ProcessPacket(AVCodecContext* dec, AVPacket* pkt, int tbNum, int tbDen);

    // Open an external subtitle file and pre-load ALL of its events into a fresh
    // track (events carry absolute timestamps, so no live demux/sync is needed).
    // Returns false if the file could not be opened/decoded.
    bool LoadExternalFile(const char* path);

    // Drop all buffered events (used on seek for embedded/live tracks).
    void FlushEvents();

    // Discard the current track entirely (subtitles disabled / switching away).
    void ClearTrack();

    // Render subtitles for timeMs (playback time in ms) into outRgba — a tightly
    // packed RGBA8 image of size vpW*vpH (top row first, straight alpha) matching
    // the on-screen video rectangle. storageW/H are the native video size, used as
    // the ASS layout reference. See RenderResult for the contract.
    RenderResult RenderOverlay(int vpW, int vpH, int storageW, int storageH, long long timeMs,
                               std::vector<unsigned char>& outRgba);

private:
    // Decode every subtitle packet of an opened format context (selected stream) into
    // the current track. Used by LoadExternalFile. Caller holds mutex_.
    bool PreloadFromFormatLocked(AVFormatContext* fmt);

    mutable std::mutex mutex_;
    ass_library* lib_ = nullptr;
    ass_renderer* renderer_ = nullptr;
    ass_track* track_ = nullptr;
};
