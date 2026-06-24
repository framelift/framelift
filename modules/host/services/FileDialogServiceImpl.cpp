#include "FileDialogServiceImpl.h"
#include <framelift/platform/IAppWindow.h>

#include <memory>
#include <string>

// MIGRATION (Phase 1 / Step 5 TODO): the SDL3 native picker (SDL_ShowOpenFileDialog) has
// been removed. The real Qt picker needs a dependency decision (QtWidgets QFileDialog vs a
// QML FileDialog) and is not needed for the CLI-driven Phase 1 milestone, so OpenFile is a
// deferred no-op: it posts the result back through the queued custom-event round-trip with
// an empty path (ok=false), exercising the same delivery path the real dialog will use.

// ── Internal payload ──────────────────────────────────────────────────────────
// Allocated per OpenFile call, ownership transferred to the event, reclaimed in
// HandleEvent. Carries the result-delivery callback and the routing event type.

namespace
{
struct Payload
{
    void (*cb)(const char* path, bool ok, void* ud) = nullptr;
    void* ud = nullptr;
    std::string path;
};
} // namespace

// ── Public API ────────────────────────────────────────────────────────────────

void FileDialogServiceImpl::Init(IAppWindow* appWindow, IEventPump* events) noexcept
{
    appWindow_ = appWindow;
    events_ = events;
    eventType_ = events->RegisterCustomEventType();
}

void FileDialogServiceImpl::OpenFile(void (*cb)(const char* path, bool ok, void* ud), void* ud) noexcept
{
    if (!appWindow_ || !events_)
    {
        return;
    }

    // Deferred cancel until the Qt picker lands (Step 5): hand an empty-path payload back
    // through the event loop so the caller gets a clean ok=false next turn (no reentrancy).
    auto* p = new Payload{cb, ud, {}};
    events_->PushCustomEvent(eventType_, p);
}

bool FileDialogServiceImpl::HandleEvent(const AppEvent& e) noexcept
{
    if (e.type != AppEventType::Custom)
    {
        return false;
    }
    const AppEvent::CustomPayload& cp = e.AsCustom();
    if (cp.eventType != eventType_)
    {
        return false;
    }

    const std::unique_ptr<Payload> p(static_cast<Payload*>(cp.userData1));
    if (p->cb)
    {
        p->cb(p->path.c_str(), !p->path.empty(), p->ud);
    }
    return true;
}
