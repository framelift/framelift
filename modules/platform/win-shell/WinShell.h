#pragma once

#include <framelift/platform/IMediaPlayer.h>
#include <chrono>
#include <memory>
#include <string>

class IModuleContext;
class ISettingsStore;
class UIContext;
class ToastNotifier;

// Windows-only host integration: raises a Qt-backed system notification when a
// file fails to play. A plain host class (owned by App, like PlaybackControls) —
// NOT a plugin/ModuleBase — driven off the media event stream App already drains.
// This header is free of <windows.h>.
class WinShell
{
public:
    WinShell();
    ~WinShell();

    WinShell(const WinShell&) = delete;
    WinShell& operator=(const WinShell&) = delete;

    // Load the notifications toggle and register the "Notifications" settings page.
    void Connect(IModuleContext& ctx);

    // Called from App::DrainMediaEvents for every media event. Updates the snapshot
    // and raises an error notification on EndFile/Error.
    void OnMediaEvent(const MediaEvent& e);

    // Settings page hooks (registered via ISettingsRegistry).
    void RenderSettings(UIContext& ctx);
    void ApplySettings();

private:
    std::unique_ptr<ToastNotifier> toast_;
    ISettingsStore* store_ = nullptr;

    bool notifyEnabled_ = true; // mirror of settings key "winshell.notifications"

    // Latest opened file (from FileOpenedEvent), used as the toast body text.
    std::string currentFile_;

    // Error-toast debounce state.
    std::string lastErrorFile_;
    std::chrono::steady_clock::time_point lastErrorTime_{};
    bool hasLastError_ = false;
};
