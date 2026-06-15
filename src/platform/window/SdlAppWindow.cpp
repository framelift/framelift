#include "SdlAppWindow.h"
#include "util.h"
#include <cstddef>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl3.h"

#include "stb_image.h"

// ReSharper disable once CppUnusedIncludeDirective
#include <cstring>
#include <string>

// ── Constructor / Destructor ──────────────────────────────────────────────────

SdlAppWindow::SdlAppWindow(const char* title, const int width, const int height)
{
    // Hint must be set before SDL_Init
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        Fatal(SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Created hidden: the window is shown on the first SwapBuffers() so it never
    // appears as an unpainted (black) framebuffer while plugins/ImGui/fonts load.
    window_ =
        SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    if (!window_)
    {
        Fatal(SDL_GetError());
    }

    glContext_ = SDL_GL_CreateContext(window_);
    if (!glContext_)
    {
        Fatal(SDL_GetError());
    }

    SDL_GL_MakeCurrent(window_, glContext_);
    SDL_GL_SetSwapInterval(0);

    renderUpdateEventType_ = SDL_RegisterEvents(1);
    playerWakeupEventType_ = SDL_RegisterEvents(1);
}

SdlAppWindow::~SdlAppWindow()
{
    if (glContext_)
    {
        SDL_GL_DestroyContext(glContext_);
        glContext_ = nullptr;
    }
    if (window_)
    {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

// ── Window ────────────────────────────────────────────────────────────────────

void SdlAppWindow::GetSize(int& w, int& h) const noexcept
{
    SDL_GetWindowSize(window_, &w, &h);
}

void SdlAppWindow::SetSize(const int w, const int h) noexcept
{
    SDL_SetWindowSize(window_, w, h);
}

bool SdlAppWindow::IsFullscreen() const noexcept
{
    return (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
}

void SdlAppWindow::SetFullscreen(const bool fs) noexcept
{
    SDL_SetWindowFullscreen(window_, fs);
}

void* SdlAppWindow::GetNativeHandle() const noexcept
{
    return window_;
}

bool SdlAppWindow::SetWindowIcon(const char* path) noexcept
{
    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load(path, &w, &h, &ch, 4);
    if (!pixels)
    {
        return false;
    }

    SDL_Surface* surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    if (!surf)
    {
        stbi_image_free(pixels);
        return false;
    }
    const bool ok = SDL_SetWindowIcon(window_, surf);
    SDL_DestroySurface(surf);
    stbi_image_free(pixels);
    return ok;
}

bool SdlAppWindow::SetWindowIconFromMemory(const unsigned char* data, const int size) noexcept
{
    int w = 0, h = 0, ch = 0;
    unsigned char* pixels = stbi_load_from_memory(data, size, &w, &h, &ch, 4);
    if (!pixels)
    {
        return false;
    }

    SDL_Surface* surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
    if (!surf)
    {
        stbi_image_free(pixels);
        return false;
    }
    const bool ok = SDL_SetWindowIcon(window_, surf);
    SDL_DestroySurface(surf);
    stbi_image_free(pixels);
    return ok;
}

void SdlAppWindow::SetTitle(const char* title) noexcept
{
    SDL_SetWindowTitle(window_, title);
}

Rect SdlAppWindow::GetDisplayUsableBounds() const noexcept
{
    const SDL_DisplayID id = SDL_GetDisplayForWindow(window_);
    SDL_Rect r{};
    SDL_GetDisplayUsableBounds(id, &r);
    return {r.x, r.y, r.w, r.h};
}

// ── OpenGL ────────────────────────────────────────────────────────────────────

void* SdlAppWindow::GetGLProcAddr(const char* name) const noexcept
{
    return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name));
}

void SdlAppWindow::SwapBuffers() noexcept
{
    SDL_GL_SwapWindow(window_);

    // Reveal the window only once a complete, painted frame is in the back buffer,
    // so the user never sees the black startup framebuffer (see ctor: SDL_WINDOW_HIDDEN).
    if (!shown_)
    {
        shown_ = true;
        SDL_ShowWindow(window_);
    }
}

void SdlAppWindow::SetVSync(const bool enabled) noexcept
{
    // 1 = sync presentation to the display refresh (tear-free); 0 = present immediately.
    SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}

bool SdlAppWindow::IsRenderable() const noexcept
{
    // No point painting (and no vsync pacing) when the window can't be seen.
    constexpr SDL_WindowFlags hidden = SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN | SDL_WINDOW_OCCLUDED;
    return (SDL_GetWindowFlags(window_) & hidden) == 0;
}

// ── Events ────────────────────────────────────────────────────────────────────

bool SdlAppWindow::WaitNextEvent(AppEvent& out, const int timeoutMs) noexcept
{
    SDL_Event e{};
    const bool got = SDL_WaitEventTimeout(&e, timeoutMs);
    out = got ? TranslateEvent(e) : AppEvent{};
    return got;
}

bool SdlAppWindow::PollNextEvent(AppEvent& out) noexcept
{
    SDL_Event e{};
    if (!SDL_PollEvent(&e))
    {
        return false;
    }
    out = TranslateEvent(e);
    return true;
}

uint32_t SdlAppWindow::RegisterCustomEventType() noexcept
{
    return SDL_RegisterEvents(1);
}

void SdlAppWindow::PushCustomEvent(const uint32_t eventType, void* data1) noexcept
{
    SDL_Event e{};
    e.type = eventType;
    e.user.data1 = data1;
    SDL_PushEvent(&e);
}

void SdlAppWindow::PushRenderUpdate() noexcept
{
    SDL_Event e{};
    e.type = renderUpdateEventType_;
    SDL_PushEvent(&e);
}

void SdlAppWindow::PushPlayerWakeup() noexcept
{
    SDL_Event e{};
    e.type = playerWakeupEventType_;
    SDL_PushEvent(&e);
}

void SdlAppWindow::PushQuitEvent() noexcept
{
    SDL_Event e{};
    e.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&e);
}

// ── Platform ──────────────────────────────────────────────────────────────────

int SdlAppWindow::GetPrefPath(const char* org, const char* app, char* buf, int cap) const noexcept
{
    char* raw = SDL_GetPrefPath(org, app);
    if (!raw)
    {
        if (buf && cap > 0)
        {
            buf[0] = '\0';
        }
        return 0;
    }
    const int len = static_cast<int>(std::strlen(raw));
    if (buf && cap > 0)
    {
        const int n = len < cap - 1 ? len : cap - 1;
        std::memcpy(buf, raw, static_cast<std::size_t>(n));
        buf[n] = '\0';
    }
    SDL_free(raw);
    return len;
}

int SdlAppWindow::GetBasePath(char* buf, int cap) const noexcept
{
    const char* raw = SDL_GetBasePath();
    if (!raw)
    {
        if (buf && cap > 0)
        {
            buf[0] = '\0';
        }
        return 0;
    }
    const int len = static_cast<int>(std::strlen(raw));
    if (buf && cap > 0)
    {
        const int n = len < cap - 1 ? len : cap - 1;
        std::memcpy(buf, raw, static_cast<std::size_t>(n));
        buf[n] = '\0';
    }
    return len;
}

// ── ImGui ─────────────────────────────────────────────────────────────────────

void SdlAppWindow::SetImGuiIniPath(const char* path) noexcept
{
    iniPath_ = path ? path : "";
}

void SdlAppWindow::ImGuiInit() noexcept
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = iniPath_.empty() ? nullptr : iniPath_.c_str();

    // Multi-viewport: lets plugin panels "pop out" into real OS windows that can
    // be dragged onto a second monitor. The SDL3 backend creates the shared GL
    // contexts for secondary windows itself; see the epilogue in UIEndFrame().
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 1.f;
    style.FrameRounding = 3.f;

    ImGui_ImplSDL3_InitForOpenGL(window_, glContext_);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

void SdlAppWindow::ImGuiShutdown() noexcept
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void SdlAppWindow::UIBeginFrame() noexcept
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void SdlAppWindow::UIEndFrame() noexcept
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Render any popped-out panels into their own OS windows, then restore the
    // main window's GL context so the caller's SwapBuffers() targets it.
    const ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        SDL_Window* backupWin = SDL_GL_GetCurrentWindow();
        SDL_GLContext backupCtx = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent(backupWin, backupCtx);
    }
}

void SdlAppWindow::ImGuiProcessEvent(const AppEvent& event) noexcept
{
    static_assert(sizeof(SDL_Event) <= sizeof(event.nativeStorage), "nativeStorage is too small for SDL_Event");
    const auto* sdlEvent = reinterpret_cast<const SDL_Event*>(event.nativeStorage);
    ImGui_ImplSDL3_ProcessEvent(sdlEvent);
}

// ── Event translation ─────────────────────────────────────────────────────────

AppEvent SdlAppWindow::TranslateEvent(SDL_Event e) const
{
    AppEvent out{};
    static_assert(sizeof(SDL_Event) <= sizeof(out.nativeStorage));
    std::memcpy(out.nativeStorage, &e, sizeof(SDL_Event));

    if (e.type == renderUpdateEventType_)
    {
        out.type = AppEventType::RenderUpdate;
        return out;
    }
    if (e.type == playerWakeupEventType_)
    {
        out.type = AppEventType::PlayerWakeup;
        return out;
    }

    switch (e.type)
    {
    case SDL_EVENT_QUIT:
        out.type = AppEventType::Quit;
        break;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        // With multi-viewport enabled, popped-out panels are separate OS windows,
        // so SDL no longer emits SDL_EVENT_QUIT when the main window closes (it is
        // not the last window). Treat closing the main window as a quit; let ImGui
        // handle close requests for its secondary (popped) windows.
        if (e.window.windowID == SDL_GetWindowID(window_))
        {
            out.type = AppEventType::Quit;
        }
        else
        {
            out.type = AppEventType::Custom;
            out.custom = {e.type, nullptr};
        }
        break;
    case SDL_EVENT_WINDOW_EXPOSED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        // All three mean the window content must be repainted; the event wakes the
        // loop from its idle WaitNextEvent so the next frame is painted promptly.
        out.type = AppEventType::WindowExposed;
        break;
    case SDL_EVENT_KEY_DOWN:
        out.type = AppEventType::KeyDown;
        out.key = {TranslateKey(e.key.key), TranslateMods(e.key.mod)};
        break;
    case SDL_EVENT_KEY_UP:
        out.type = AppEventType::KeyUp;
        out.key = {TranslateKey(e.key.key), TranslateMods(e.key.mod)};
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        out.type = AppEventType::MouseButtonDown;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        out.type = AppEventType::MouseMotion;
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        out.type = AppEventType::MouseWheel;
        break;
    case SDL_EVENT_DROP_FILE:
        out.type = AppEventType::DropFile;
        out.file = {e.drop.data}; // host-owned, valid for the OnEvent call duration
        break;
    default:
        out.type = AppEventType::Custom;
        out.custom = {
            e.type,
            e.type >= SDL_EVENT_USER ? e.user.data1 : nullptr,
        };
        break;
    }
    return out;
}

Key SdlAppWindow::TranslateKey(const SDL_Keycode k)
{
    // SDL_Keycode values match our Key constants exactly:
    //   printable chars  → Unicode codepoints (same as Keys::A … Keys::Z etc.)
    //   scancode-based   → SDLK_SCANCODE_MASK | SDL_Scancode (same as Keys::ScancodeBase | n)
    // A plain cast is therefore correct for every key — no switch needed.
    return k;
}

Mod SdlAppWindow::TranslateMods(SDL_Keymod m)
{
    // Strip lock keys so Caps/Num/Scroll don't interfere with modifier matching
    m = static_cast<SDL_Keymod>(m & ~(SDL_KMOD_CAPS | SDL_KMOD_NUM | SDL_KMOD_SCROLL));
    uint32_t r = 0;
    if (m & SDL_KMOD_CTRL)
    {
        r |= static_cast<uint32_t>(Mod::Ctrl);
    }
    if (m & SDL_KMOD_SHIFT)
    {
        r |= static_cast<uint32_t>(Mod::Shift);
    }
    if (m & SDL_KMOD_ALT)
    {
        r |= static_cast<uint32_t>(Mod::Alt);
    }
    return static_cast<Mod>(r);
}
