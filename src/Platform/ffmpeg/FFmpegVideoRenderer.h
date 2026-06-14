#pragma once

#include <cstdint>
#include <memory>

// GL 3.3-core blitter for the FFmpeg backend: uploads software-decoded RGBA
// frames to a texture and draws them, letterboxed, to the current framebuffer
// (the default FBO the host renders the UI over).
//
// It resolves its own GL entry points through the host-provided getProcAddr, so
// it needs no system GL headers and no link dependency beyond the live GL context.
// Every method must run on the thread that owns that context (the host's main /
// render thread) — never the decode thread.
class FFmpegVideoRenderer
{
public:
    FFmpegVideoRenderer();
    ~FFmpegVideoRenderer();

    FFmpegVideoRenderer(const FFmpegVideoRenderer&) = delete;
    FFmpegVideoRenderer& operator=(const FFmpegVideoRenderer&) = delete;

    // Resolve GL functions and build the shader / VAO / texture. Returns false if
    // any entry point is missing or the shader fails to compile/link (the caller
    // then just clears to black). getProcAddr(name, ud) resolves a GL function.
    bool Init(void* (*getProcAddr)(const char* name, void* ud), void* ud);

    // Upload a tightly packed RGBA8 frame (w*h*4 bytes, top row first).
    void Upload(const uint8_t* rgba, int w, int h);

    // Upload the subtitle overlay: a tightly packed RGBA8 image (w*h*4 bytes, top
    // row first) sized to the on-screen video rectangle, so it maps 1:1 over the
    // letterboxed video when drawn. Straight (non-premultiplied) alpha.
    void UploadOverlay(const uint8_t* rgba, int w, int h);

    // Clear the framebuffer (fbW x fbH px) to black and, once a frame has been
    // uploaded, draw it centered with aspect-ratio-preserving letterboxing. When
    // drawOverlay is set and an overlay has been uploaded, alpha-composite it over
    // the video within the same letterboxed rectangle.
    void Draw(int fbW, int fbH, bool drawOverlay = false);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
