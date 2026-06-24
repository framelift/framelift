#pragma once

#include "FileDialogServiceImpl.h"
#include "PackageConfig.h"
#include "FocusManagerImpl.h"
#include "HotkeysImpl.h"
#include "JsonServiceImpl.h"
#include "ModuleContext.h"
#include "PackageLoader.h"
#include "ModuleRegistry.h"
#include "PlaybackControls.h"
#include "Settings.h"
#include <framelift/IRenderable.h>
#include <framelift/platform/IAppWindow.h>
#include <framelift/platform/IDirWatcher.h>
#include <framelift/platform/IMediaPlayer.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

class QtAppWindow;
class FFmpegPlayer;
class WinShell;

// Top-level application object. Owns all subsystems, drives the main loop,
// and co-ordinates rendering. Exactly one instance exists for the program lifetime.
// Has no compile-time knowledge of specific modules — every capability ships as a
// package DLL loaded at runtime from the packages/ directory.
class App
{
public:
    App(const char* title, int width, int height, int cliArgc = 0, const char* const* cliArgv = nullptr);
    ~App();

    int Run();

private:
    // Owner cell for the async resize query: heap-allocated, passed as the opaque
    // user-data pointer, and deleted inside the trampoline once it fires.
    struct AsyncSelf
    {
        const App* app;
    };

    // ── Construction phases (run in order from the ctor) ──
    void InitPlatform(const char* title, int width, int height, const std::string& prefDir, const std::string& settingsPath);
    void InitServices(const std::string& prefDir, const std::string& settingsPath);
    void LoadPackages();
    void BuildRenderables();

    // Wire the player's worker-thread wakeups to the window's queued signals (no GL).
    void SetupPlayerCallbacks();
    void ResizeToVideo() const;

    void Dispatch(const AppEvent& e);
    void DrainMediaEvents();

    // The window's scene-graph video node calls this on the GUI thread with the target
    // framebuffer size: lazily adopts Qt's GL context + builds the renderer on first call,
    // then draws the current frame letterboxed.
    void RenderVideo(int fbW, int fbH);
    // Queued-signal handler: drain media events and apply any pending video-driven resize.
    void OnPlayerWakeup();

    // Process command line, forwarded from main(). main()'s argv stays valid for
    // the program lifetime, so storing the pointer is safe. Broadcast verbatim as
    // a CliCommandEvent at startup; the first positional arg also opens a file.
    int cliArgc_ = 0;
    const char* const* cliArgv_ = nullptr;

    std::unique_ptr<QtAppWindow> appWindow_;
    // App always builds the concrete FFmpegPlayer: it needs the FFmpeg-only entry
    // points (ApplySettings, decode mode, ducking) that aren't on any of the split
    // playback interfaces, and it registers each of those interfaces as a service.
    std::unique_ptr<FFmpegPlayer> player_;
    FFmpegPlayer* ffmpeg_ = nullptr; // alias of player_.get(), kept for readability at call sites
    std::unique_ptr<IDirWatcher> dirWatcher_;

    bool pendingResize_ = false;

    // Set when a media/video state change (other than a routine position tick) may have
    // altered the video image without a freshly decoded frame — e.g. EOF → idle screen,
    // a video reconfig, or a seek. Reserved for the Vulkan compositor; ignored by GL.
    bool pendingVideoRedraw_ = false;

    // First-frame guard: Qt's scene-graph GL context only exists once the SG initializes,
    // so the backend's context adoption + the player's renderer build are deferred to the
    // first RenderVideo() (where the GL context is current).
    bool renderInit_ = false;

    Settings settings_;
    PackageConfig packageConfig_;
    std::string packagesPath_;
    FileDialogServiceImpl fileDialogService_{&settings_};
    FocusManagerImpl focus_;
    HotkeysImpl keys_;
    JsonServiceImpl jsonService_;

    std::unique_ptr<ModuleContext> moduleCtx_;
    std::unique_ptr<PlaybackControls> playbackControls_;
#if FRAMELIFT_MODULE_WIN_SHELL
    // Windows-only: taskbar playback progress + error toasts. Driven off the media
    // event stream in DrainMediaEvents; not part of the plugin registry.
    std::unique_ptr<WinShell> winShell_;
#endif
    PackageLoader packageLoader_;
    ModuleRegistry registry_;

    std::vector<IRenderable*> renderables_;
};
