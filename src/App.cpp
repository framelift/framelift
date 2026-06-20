#include "App.h"
#include "Cli.h"
#include "IconData.h"
#include "SettingsMapping.h"
#include "CoreSettings.h"
#include "GraphicsApi.h"
#include "GraphicsSettings.h"
#include "IGraphicsBackend.h"
#include "DirWatcher.h"
#include "FFmpegPlayer.h"
#include "SdlAppWindow.h"
#include "Theme.h"
#include <framelift/ContextHelpers.h>
#include <framelift/Log.h>
#include <framelift/Events.h>
#include <framelift/IModule.h>
#include <framelift/services/IHistory.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <ranges>

// ── Static storage ────────────────────────────────────────────────────────────

Services App::services_;
PluginRegistry App::registry_;

::Services& App::Services()
{
    return services_;
}

PluginRegistry& App::Registry()
{
    return registry_;
}

// ── Helpers ───────────────────────────────────────────────────────────────────
// ParamsFromSettings / PlaybackOptsFromSettings live in SettingsMapping.h (pure,
// unit-tested).

// ── Constructor / Destructor ──────────────────────────────────────────────────

App::App(const char* title, const int width, const int height, const int cliArgc, const char* const* cliArgv)
    : cliArgc_(cliArgc), cliArgv_(cliArgv), player_(std::make_unique<FFmpegPlayer>()),
      dirWatcher_(CreateDirWatcher())
{
    // ── Phase 1: Platform init ────────────────────────────────────────────────
    // Resolve the pref dir and load settings BEFORE creating the window: the graphics
    // backend — and thus the SDL window flag (SDL_WINDOW_OPENGL vs SDL_WINDOW_VULKAN) —
    // is fixed at window-creation time, so graphics.backend must be known up front.
    char prefBuf[512] = {};
    (void)SdlAppWindow::ResolvePrefPath("", "FrameLift", prefBuf, sizeof(prefBuf));
    const std::string prefDir = prefBuf;
    const std::string settingsPath = prefDir.empty() ? "settings.ini" : prefDir + "settings.ini";

    // App owns the Settings instance; load it before any plugin sees it.
    settings_.Load(settingsPath);

    // User plugin enablement manifest (opt-out): load before plugins are scanned.
    pluginsPath_ = prefDir.empty() ? "plugins.ini" : prefDir + "plugins.ini";
    pluginConfig_.Load(pluginsPath_);

    appWindow_ =
        std::make_unique<SdlAppWindow>(title, width, height, GraphicsApiFromString(settings_.Get<GraphicsSettings>().backend));

    // Let the UI context create plugin-icon textures through the active backend.
    uiCtx_.SetGraphicsBackend(static_cast<IGraphicsBackend*>(appWindow_->GetGraphicsBackend()));

    if (!prefDir.empty())
    {
        const std::string iniPath = prefDir + "imgui.ini";
        appWindow_->SetImGuiIniPath(iniPath.c_str());
    }
    (void)appWindow_->SetWindowIconFromMemory(kIconData, kIconDataSize);

    InitRender();
    InitImGui();

    // Apply the persisted theme before the first frame (GL context is current,
    // NewFrame has not run yet). Seed the snapshot so later Saves can diff.
    Theme::ApplyStyle(settings_.Get<ThemeSettings>());
    Theme::RebuildFonts(settings_.Get<ThemeSettings>());
    appliedTheme_ = {settings_.Get<ThemeSettings>().preset, settings_.Get<ThemeSettings>().accentColor, settings_.Get<ThemeSettings>().fontFile,
                     settings_.Get<ThemeSettings>().fontSize};

    fileDialogService_.Init(appWindow_.get());

    // ── Phase 2: Build PluginContext and register services ────────────────────
    pluginCtx_ = std::make_unique<PluginContext>(prefDir, &settings_, settingsPath, &pluginConfig_, pluginsPath_);

    pluginCtx_->RegisterService<IMediaPlayer>(player_.get());
    pluginCtx_->RegisterService<IAppWindow>(appWindow_.get());
    pluginCtx_->RegisterService<IDirWatcher>(dirWatcher_.get());
    pluginCtx_->RegisterService<Hotkeys>(&keys_);
    pluginCtx_->RegisterService<FocusManager>(&focus_);
    pluginCtx_->RegisterService<IFileDialog>(&fileDialogService_);

    // Playback options update when settings change.
    player_->SetPlaybackOptions(PlaybackOptsFromSettings(settings_.Get<PlaybackSettings>()));
    if (auto* ffmpeg = dynamic_cast<FFmpegPlayer*>(player_.get()))
    {
        ffmpeg->SetVideoDecodeMode(VideoDecodeModeFromSettings(settings_.Get<PlaybackSettings>()));
    }
    player_->SetReadAheadCache(ReadAheadOptsFromSettings(settings_.Get<CacheSettings>()));
    player_->SetSubtitleStyle(SubtitleStyleFromSettings(settings_.Get<SubtitleSettings>()));
    player_->SetAudioPreferences(AudioPrefsFromSettings(settings_.Get<AudioSettings>()));
    player_->SetAudioNormalize(
        settings_.Get<AudioSettings>().normalizeEnabled,
        settings_.Get<AudioSettings>().normalizeEnabled ? ParamsFromSettings(settings_.Get<AudioSettings>()) : AudioNormalizeParams{}
    );
    appWindow_->SetVSync(settings_.Get<PlaybackSettings>().videoSync);

    // Track idle state for TogglePauseAction (see DrainMediaEvents).
    player_->ObserveProperty(PlayerProperty::IdleActive);
    framelift::Subscribe<NotificationEvent>(
        *pluginCtx_,
        [this](const NotificationEvent&)
        {
            PulseAudioDucking();
        }
    );
    framelift::RegisterSettingsChangeCallback(
        *pluginCtx_,
        [this]()
        {
            const Settings& s = pluginCtx_->GetSettingsDirect();
            player_->SetPlaybackOptions(PlaybackOptsFromSettings(s.Get<PlaybackSettings>()));
            if (auto* ffmpeg = dynamic_cast<FFmpegPlayer*>(player_.get()))
            {
                ffmpeg->SetVideoDecodeMode(VideoDecodeModeFromSettings(s.Get<PlaybackSettings>()));
            }
            player_->SetReadAheadCache(ReadAheadOptsFromSettings(s.Get<CacheSettings>()));
            player_->SetSubtitleStyle(SubtitleStyleFromSettings(s.Get<SubtitleSettings>()));
            player_->SetAudioPreferences(AudioPrefsFromSettings(s.Get<AudioSettings>()));
            appWindow_->SetVSync(s.Get<PlaybackSettings>().videoSync);
            player_->SetAudioNormalize(
                s.Get<AudioSettings>().normalizeEnabled, s.Get<AudioSettings>().normalizeEnabled ? ParamsFromSettings(s.Get<AudioSettings>()) : AudioNormalizeParams{}
            );

            // This callback fires mid-frame (during SettingsMenu's
            // Save inside Render), so only flag what changed; the
            // actual ImGui work happens at the top of next Render().
            if (s.Get<ThemeSettings>().preset != appliedTheme_.preset || s.Get<ThemeSettings>().accentColor != appliedTheme_.accentColor)
            {
                themeStyleDirty_ = true;
            }
            if (s.Get<ThemeSettings>().fontFile != appliedTheme_.fontFile || s.Get<ThemeSettings>().fontSize != appliedTheme_.fontSize)
            {
                fontAtlasDirty_ = true;
            }
            appliedTheme_ = {s.Get<ThemeSettings>().preset, s.Get<ThemeSettings>().accentColor, s.Get<ThemeSettings>().fontFile, s.Get<ThemeSettings>().fontSize};
        }
    );

    // ── Phase 3: Plugins ──────────────────────────────────────────────────────
    // The ContextMenu plugin owns the right-click menu: it registers the
    // ContextMenu service that other plugins extend, and assembles its core items
    // plus their sections on its first frame.
    LoadPlugins();

    BuildRenderables();
}

App::~App()
{
    appWindow_->ImGuiShutdown();

    // Clear all DLL-owned lambdas before pluginLoader_ calls FreeLibrary.
    // pluginLoader_ destructs after this body (declared after pluginCtx_),
    // so plugins can still call ctx_->GetService() in their destructors.
    if (pluginCtx_)
    {
        pluginCtx_->ClearSubscriptions();
    }
    keys_.Clear();

    player_.reset(); // destroy the player's render context before the GL context is torn down
}

// ── Plugin loading ────────────────────────────────────────────────────────────

void App::LoadPlugins()
{
    // Load every module package present in the Modules/ subdirectory. Enablement is
    // driven by module JSON (a disabled package isn't built/shipped, so it's absent
    // here); the loader resolves dependencies and load order from embedded metadata.
    // Each plugin registers its own context menu section during Install().
    char baseBuf[512] = {};
    (void)appWindow_->GetBasePath(baseBuf, sizeof(baseBuf));
    const std::string modulesDir = std::string(baseBuf) + "Modules/";
    pluginLoader_.LoadAll(modulesDir, pluginConfig_.DisabledIds());

    for (auto& p : pluginLoader_.Plugins())
    {
        // Record identity before Install so a plugin may list peers during it.
        pluginCtx_->AddPlugin(p.name, /*enabled=*/true, p.info);
        Registry().Add(p.module, *pluginCtx_);
    }

    // Append any plugin packages that are present but not loaded — either disabled
    // by the user or rejected by the resolver — so the settings UI can list them.
    std::vector<std::string> discoveredIds;
    for (auto& package : PluginLoader::DiscoverAvailable(modulesDir))
    {
        discoveredIds.push_back(package.packageId);
        const bool loaded = std::ranges::any_of(
            pluginLoader_.Plugins(),
            [&](const auto& p)
            {
                return p.name == package.packageId;
            }
        );
        if (loaded)
        {
            continue;
        }
        // enabled=false ⇒ user-disabled (unchecked in the UI); enabled=true but not
        // loaded ⇒ resolver-rejected (surfaces as a load failure).
        const bool enabled = pluginConfig_.IsEnabled(package.packageId);
        pluginCtx_->AddPlugin(std::move(package.packageId), enabled, nullptr);
    }

    // Refresh the manifest so it lists every discovered plugin (new ones default to
    // enabled), keeping plugins.ini a complete, hand-editable record.
    pluginConfig_.EnsureKnown(discoveredIds);
    pluginConfig_.Save(pluginsPath_);

    Registry().BindHotkeys(keys_);
    BindPlayerHotkeys();

    // Materialize plugin settings + keybinds on first run: plugins populate their
    // PluginSettingsImpl caches during Install(), but those reach disk only via
    // SaveSettings(). The startup Settings::Save (App ctor, before plugins) writes
    // host sections only, so without this flush plugin sections appear only after
    // the user presses Save in the Settings menu.
    pluginCtx_->SaveSettings();
}

// ── Renderables ───────────────────────────────────────────────────────────────

void App::BuildRenderables()
{
    // Collect (order, renderable) pairs from plugins and sort. The context menu is
    // itself a plugin (renderOrder 30), so it sorts in alongside the rest.
    using OrderedR = std::pair<int, IRenderable*>;
    std::vector<OrderedR> items;

    for (const auto& p : pluginLoader_.Plugins())
    {
        if (p.renderable)
        {
            items.emplace_back(p.renderOrder, p.renderable);
        }
    }

    std::ranges::stable_sort(
        items,
        [](const OrderedR& a, const OrderedR& b)
        {
            return a.first < b.first;
        }
    );

    renderables_.clear();
    for (auto& r : items | std::views::values)
    {
        renderables_.push_back(r);
    }
}

// ── Render setup ──────────────────────────────────────────────────────────────

void App::InitRender() const
{
    // Hand the player the active graphics backend (opaque IGraphicsBackend*); it
    // builds its matching video renderer from it (GL or Vulkan).
    player_->InitRender(appWindow_->GetGraphicsBackend());

    player_->SetRenderUpdateCallback(
        [](void* ud)
        {
            static_cast<App*>(ud)->appWindow_->PushRenderUpdate();
        },
        const_cast<App*>(this)
    );
    player_->SetWakeupCallback(
        [](void* ud)
        {
            static_cast<App*>(ud)->appWindow_->PushPlayerWakeup();
        },
        const_cast<App*>(this)
    );
}

void App::InitImGui() const
{
    appWindow_->ImGuiInit();
}

// ── Frame rendering ───────────────────────────────────────────────────────────

void App::Render()
{
    // Apply any pending theme changes here — before UIBeginFrame, i.e. outside
    // any NewFrame()..Render() pair, which both calls require.
    if (themeStyleDirty_)
    {
        themeStyleDirty_ = false;
        Theme::ApplyStyle(settings_.Get<ThemeSettings>());
    }
    if (fontAtlasDirty_)
    {
        fontAtlasDirty_ = false;
        Theme::RebuildFonts(settings_.Get<ThemeSettings>());
    }

    int w = 0, h = 0;
    appWindow_->GetSize(w, h);

    // Acquire the frame's render target. The backend may decline (e.g. the Vulkan
    // swapchain is being recreated) — skip rendering and presenting this iteration.
    if (!appWindow_->BeginFrame())
    {
        return;
    }

    player_->RenderFrame(w, h);

    appWindow_->UIBeginFrame();
    uiCtx_.UpdateKeys(&keys_);
    uiCtx_.BeginFrame();
    for (auto* r : renderables_)
    {
        r->Render(uiCtx_);
    }
    appWindow_->UIEndFrame();

    appWindow_->SwapBuffers();
    appWindow_->ImGuiRenderPlatformWindows();
}

// ── Event loop helpers ────────────────────────────────────────────────────────

void App::DrainMediaEvents()
{
    while (true)
    {
        const MediaEvent ev = player_->PollEvent();
        if (ev.type == MediaEventType::None)
        {
            break;
        }
        if (ev.type == MediaEventType::PropertyChange && ev.property.prop == PlayerProperty::IdleActive &&
            ev.property.type == PropertyType::Flag)
        {
            playerIdle_ = ev.property.value.flag != 0;
        }
        Registry().OnMediaEvent(ev);
        if (ev.type == MediaEventType::VideoReconfig)
        {
            pendingResize_ = true;
        }
    }
}

void App::Dispatch(const AppEvent& e)
{
    appWindow_->ImGuiProcessEvent(e);
    switch (e.type) // NOLINT(clang-diagnostic-switch-enum)
    {
    case AppEventType::Quit:
        // Let plugins handle their own Quit cleanup (e.g. Playlist::FlushCurrentPos).
        Registry().OnEvent(e);
        Registry().OnShutdown();
        running_ = false;
        return;
    case AppEventType::WindowExposed:
    case AppEventType::RenderUpdate:
        // The loop renders every frame while visible, so these only need to wake
        // WaitNextEvent; the new video frame is consumed by player_->RenderFrame().
        return;
    case AppEventType::PlayerWakeup:
        DrainMediaEvents();
        return;
    default:
        break;
    }

    if (e.type == AppEventType::DropFile)
    {
        const AppEvent::FilePayload& fp = e.AsFile();
        if (fp.filePath && fp.filePath[0])
        {
            pluginCtx_->Publish<OpenFileRequestEvent>({fp.filePath, true});
        }
        return;
    }

    if (fileDialogService_.HandleEvent(e))
    {
        return;
    }

    if (e.type == AppEventType::KeyDown || e.type == AppEventType::KeyUp)
    {
        if (auto* f = focus_.Focused())
        {
            if (auto* handler = static_cast<IEventHandler*>(f->QueryInterface(IEventHandler::InterfaceId)))
            {
                if (handler->OnEvent(e))
                {
                    return;
                }
            }
        }
        // ReSharper disable once CppExpressionWithoutSideEffects
        keys_.Handle(e);
        return;
    }

    Registry().OnEvent(e);
}

void App::DrainEvents(const int timeoutMs)
{
    AppEvent e;
    (void)appWindow_->WaitNextEvent(e, timeoutMs);
    do
    {
        Dispatch(e);
    } while (appWindow_->PollNextEvent(e));
}

void App::RenderFrame()
{
    RefreshAudioDucking();
    Render();
    if (pendingResize_)
    {
        pendingResize_ = false;
        ResizeToVideo();
    }
}

void App::PulseAudioDucking()
{
    if (!settings_.Get<AudioSettings>().duckingEnabled)
    {
        return;
    }
    audioDuckUntil_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(800);
    if (!audioDucked_)
    {
        audioDucked_ = true;
        if (auto* ffmpeg = dynamic_cast<FFmpegPlayer*>(player_.get()))
        {
            ffmpeg->SetAudioDucked(true);
        }
    }
}

void App::RefreshAudioDucking()
{
    if (!audioDucked_)
    {
        return;
    }
    if (!settings_.Get<AudioSettings>().duckingEnabled || std::chrono::steady_clock::now() >= audioDuckUntil_)
    {
        audioDucked_ = false;
        if (auto* ffmpeg = dynamic_cast<FFmpegPlayer*>(player_.get()))
        {
            ffmpeg->SetAudioDucked(false);
        }
    }
}

void App::ResizeToVideo() const
{
    if (appWindow_->IsFullscreen())
    {
        return;
    }

    struct SizeState
    {
        const App* app;
    };

    player_->GetDisplaySizeAsync(
        [](const DisplaySize* size, bool ok, void* ud)
        {
            auto* s = static_cast<SizeState*>(ud);
            const App* self = s->app;
            delete s;
            if (!ok || !size || size->width <= 0 || size->height <= 0 || self->appWindow_->IsFullscreen())
            {
                return;
            }

            const Rect usable = self->appWindow_->GetDisplayUsableBounds();
            int w = static_cast<int>(size->width);
            int h = static_cast<int>(size->height);
            const float ratio = self->settings_.Get<GeneralSettings>().maxDisplayRatio;
            const int maxW = static_cast<int>(static_cast<float>(usable.w) * ratio);
            const int maxH = static_cast<int>(static_cast<float>(usable.h) * ratio);

            if (w > maxW || h > maxH)
            {
                const float scale = std::min(
                    static_cast<float>(maxW) / static_cast<float>(w), static_cast<float>(maxH) / static_cast<float>(h)
                );
                w = static_cast<int>(static_cast<float>(w) * scale);
                h = static_cast<int>(static_cast<float>(h) * scale);
            }
            self->appWindow_->SetSize(w, h);
        },
        new SizeState{this}
    );
}

// ── Player actions ────────────────────────────────────────────────────────────

void App::TogglePauseAction() const
{
    if (!playerIdle_)
    {
        player_->TogglePause();
        return;
    }

    // Idle: resume the most recent file instead of toggling pause.
    char lastBuf[2048] = {};
    if (const auto* history = pluginCtx_->GetService<IHistory>())
    {
        (void)history->GetMostRecent(lastBuf, sizeof(lastBuf));
    }
    if (!lastBuf[0])
    {
        pluginCtx_->Publish<NotificationEvent>({"No recent files"});
        return;
    }
    if (!std::filesystem::exists(lastBuf))
    {
        pluginCtx_->Publish<NotificationEvent>({"Error: File not found"});
        return;
    }
    pluginCtx_->Publish<OpenFileRequestEvent>({lastBuf, true});
}

void App::AdjustVolumeAndNotify(const int delta) const
{
    player_->AdjustVolume(delta);

    struct VolCtx
    {
        const App* app;
    };

    player_->GetDoubleAsync(
        PlayerProperty::Volume,
        [](const double v, const bool ok, void* ud)
        {
            const auto* c = static_cast<VolCtx*>(ud);
            const auto* self = c->app;
            delete c;
            if (ok)
            {
                const std::string msg = "Volume: " + std::to_string(static_cast<int>(v));
                self->pluginCtx_->Publish<NotificationEvent>({msg.c_str()});
            }
        },
        new VolCtx{this}
    );
}

void App::OpenFileAction()
{
    fileDialogService_.OpenFile(
        [](const char* path, const bool ok, void* ud)
        {
            if (ok && path && path[0])
            {
                static_cast<App*>(ud)->pluginCtx_->Publish<OpenFileRequestEvent>({path, true});
            }
        },
        this
    );
}

void App::BindPlayerHotkeys()
{
    const Settings& cfg = settings_;

    host::Bind(
        keys_, "togglePause", cfg.Get<KeybindSettings>().togglePause,
        [this]
        {
            TogglePauseAction();
        }
    );
    host::Bind(
        keys_, "toggleFullscreen", cfg.Get<KeybindSettings>().toggleFullscreen,
        [this]
        {
            appWindow_->SetFullscreen(!appWindow_->IsFullscreen());
        }
    );
    host::Bind(
        keys_, "quit", cfg.Get<KeybindSettings>().quit,
        [this]
        {
            appWindow_->PushQuitEvent();
        }
    );
    host::Bind(
        keys_, "toggleNormalize", cfg.Get<KeybindSettings>().toggleNormalize,
        [this]
        {
            const bool on = !player_->IsNormalizeEnabled();
            player_->SetAudioNormalize(on, on ? ParamsFromSettings(settings_.Get<AudioSettings>()) : AudioNormalizeParams{});
            pluginCtx_->Publish<NotificationEvent>({on ? "Normalize: On" : "Normalize: Off"});
        }
    );
    host::Bind(
        keys_, "volumeUp", cfg.Get<KeybindSettings>().volumeUp,
        [this]
        {
            AdjustVolumeAndNotify(5);
        }
    );
    host::Bind(
        keys_, "volumeDown", cfg.Get<KeybindSettings>().volumeDown,
        [this]
        {
            AdjustVolumeAndNotify(-5);
        }
    );
    host::Bind(
        keys_, "toggleMute", cfg.Get<KeybindSettings>().toggleMute,
        [this]
        {
            player_->ToggleMute();
            pluginCtx_->Publish<NotificationEvent>({player_->IsMuted() ? "Mute: On" : "Mute: Off"});
        }
    );
    host::Bind(
        keys_, "toggleSubtitles", cfg.Get<KeybindSettings>().toggleSubtitles,
        [this]
        {
            player_->ToggleSubtitles();
            pluginCtx_->Publish<NotificationEvent>(
                {player_->IsSubtitlesEnabled() ? "Subtitles: On" : "Subtitles: Off"}
            );
        }
    );
    host::Bind(
        keys_, "seekForward", cfg.Get<KeybindSettings>().seekForward,
        [this]
        {
            player_->Seek(5);
            pluginCtx_->Publish<NotificationEvent>({"Seek +Short"});
        }
    );
    host::Bind(
        keys_, "seekBack", cfg.Get<KeybindSettings>().seekBack,
        [this]
        {
            player_->Seek(-5);
            pluginCtx_->Publish<NotificationEvent>({"Seek -Short"});
        }
    );
    host::Bind(
        keys_, "seekForwardLong", cfg.Get<KeybindSettings>().seekForwardLong,
        [this]
        {
            player_->Seek(60);
            pluginCtx_->Publish<NotificationEvent>({"Seek +Long"});
        }
    );
    host::Bind(
        keys_, "seekBackLong", cfg.Get<KeybindSettings>().seekBackLong,
        [this]
        {
            player_->Seek(-60);
            pluginCtx_->Publish<NotificationEvent>({"Seek -Long"});
        }
    );
    host::Bind(
        keys_, "openFileDialog", cfg.Get<KeybindSettings>().openFileDialog,
        [this]
        {
            OpenFileAction();
        }
    );
}

// ── Event loop ────────────────────────────────────────────────────────────────

int App::Run()
{
    // Hand the command line to plugins (all loaded and subscribed by now), then
    // open the first positional file/URL argument — the same request drag-drop and
    // the Open-File dialog publish, so Playlist/RemoteStream route it by scheme.
    pluginCtx_->Publish<CliCommandEvent>({cliArgc_, cliArgv_});
    if (const std::string target = ParseOpenTarget(cliArgc_, cliArgv_); !target.empty())
    {
        pluginCtx_->Publish<OpenFileRequestEvent>({target.c_str(), true});
    }

    // Paint one frame before the loop so the window (created hidden) is shown
    // already displaying the idle screen instead of a black framebuffer. The
    // hidden window emits no WindowExposed event, so we cannot rely on the loop
    // to trigger this first render.
    RenderFrame();

    while (running_)
    {
        const bool renderable = appWindow_->IsRenderable();
        // While visible we render every frame; vsync paces via SwapBuffers (poll
        // events with 0 timeout), vsync-off caps at ~250 fps so a static screen
        // doesn't spin. While minimized/occluded we don't paint, so block on
        // events (100 ms) to idle instead of busy-looping.
        const int timeoutMs = !renderable ? 100 : (settings_.Get<PlaybackSettings>().videoSync ? 0 : 4);
        DrainEvents(timeoutMs);
        RefreshAudioDucking();
        if (running_ && appWindow_->IsRenderable())
        {
            RenderFrame();
        }
    }
    return 0;
}
