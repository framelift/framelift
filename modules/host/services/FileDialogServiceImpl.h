#pragma once
#include <framelift/AppEvent.h>
#include <framelift/platform/IFileDialog.h>
#include <cstdint>

class IAppWindow;
class Settings;

// Host file picker — the single file-dialog abstraction for both the host and
// plugins. Implements the plugin-facing IFileDialog ABI and adds host-only
// Init/HandleEvent for the SDL async-result plumbing. The implementation
// (FileDialogServiceImpl.cpp) is SDL-aware: like SdlAppWindow, it is one of the
// few files allowed to touch SDL directly, since the native picker APIs are
// inherently backend-specific.
class FileDialogServiceImpl final : public IFileDialog
{
public:
    explicit FileDialogServiceImpl(const Settings* settings) : settings_(settings)
    {
    }

    // Call once after the IAppWindow is created — registers the custom event type
    // used to deliver async picker results back onto the main thread.
    void Init(IAppWindow* appWindow) noexcept;

    // Call once per frame from the event loop. Returns true if the event was a
    // picker result (consumed); fires the pending callback.
    bool HandleEvent(const AppEvent& e) noexcept;

    // IFileDialog — opens the native open-file picker with video+image filters
    // taken from settings. cb(path, ok, ud) fires on the main thread.
    void OpenFile(void (*cb)(const char* path, bool ok, void* ud), void* ud) noexcept override;

private:
    const Settings* settings_;
    IAppWindow* appWindow_ = nullptr;
    uint32_t eventType_ = 0;
};
