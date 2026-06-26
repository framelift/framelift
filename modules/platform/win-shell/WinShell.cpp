#include "WinShell.h"
#include "ToastNotifier.h"

#include <framelift/ContextHelpers.h>
#include <framelift/Events.h>
#include <framelift/IModuleSettings.h>
#include <framelift/services/ISettingsRegistry.h>
#include <framelift/services/ISettingsStore.h>
#include <framelift/ui/UIContext.h>

#include <filesystem>

namespace
{
constexpr const char* kSection = "winshell";
constexpr const char* kNotifyKey = "notifications";
constexpr double kErrorDebounceSeconds = 1.5;

std::string FilenameOf(const std::string& path)
{
    try
    {
        return std::filesystem::path(path).filename().string();
    }
    catch (...)
    {
        return path;
    }
}

bool ShouldNotifyError(bool sameFileAsLast, double secondsSinceLast) noexcept
{
    return !sameFileAsLast || secondsSinceLast >= kErrorDebounceSeconds;
}
} // namespace

WinShell::WinShell() : toast_(std::make_unique<ToastNotifier>()) {}

WinShell::~WinShell() = default;

void WinShell::Connect(IModuleContext& ctx)
{
    // Persisted enable/disable toggle in the [winshell] INI section (default on).
    store_ = ctx.GetService<ISettingsStore>();
    if (store_)
    {
        IModuleSettings& ps = store_->GetModuleSettings(kSection);
        notifyEnabled_ = ps.GetBool(kNotifyKey, true);
        // Seed the default to disk on first run so the section is hand-editable.
        if (!ps.WasLoaded())
        {
            ps.SetBool(kNotifyKey, notifyEnabled_);
            ps.Save();
        }
    }

    // Reflect the file currently playing in the error-toast text.
    framelift::Subscribe<FileOpenedEvent>(
        ctx,
        [this](const FileOpenedEvent& e)
        {
            currentFile_ = e.path ? e.path : "";
        }
    );

    // Register the "Notifications" settings page (status + toggle + register button).
    if (auto* reg = ctx.GetService<ISettingsRegistry>())
    {
        reg->RegisterSettingsPage(
            "Notifications",
            [](void* ud, UIContext& c)
            {
                static_cast<WinShell*>(ud)->RenderSettings(c);
            },
            [](void* ud)
            {
                static_cast<WinShell*>(ud)->ApplySettings();
            },
            this, /*visible=*/true, /*cleanup=*/nullptr
        );
    }
}

void WinShell::OnMediaEvent(const MediaEvent& e)
{
    switch (e.type)
    {
    case MediaEventType::StartFile:
    case MediaEventType::FileLoaded:
        break;

    case MediaEventType::EndFile:
        if (e.endReason == EndFileReason::Error)
        {
            if (notifyEnabled_ && toast_)
            {
                const auto now = std::chrono::steady_clock::now();
                const bool sameFile = hasLastError_ && currentFile_ == lastErrorFile_;
                const double sinceLast =
                    hasLastError_ ? std::chrono::duration<double>(now - lastErrorTime_).count() : 1.0e9;
                if (ShouldNotifyError(sameFile, sinceLast))
                {
                    toast_->NotifyError(FilenameOf(currentFile_).c_str());
                    lastErrorFile_ = currentFile_;
                    lastErrorTime_ = now;
                    hasLastError_ = true;
                }
            }
        }
        break;

    default:
        break;
    }
}

void WinShell::RenderSettings(UIContext& ctx)
{
    ctx.Checkbox("Show playback-error notifications", &notifyEnabled_);

    ctx.Separator();

    if (toast_ && toast_->IsAvailable())
    {
        ctx.Text("Notifications are available through Qt.");
        if (ctx.Button("Send test notification"))
        {
            toast_->Notify("FrameLift", "This is a test notification.");
        }
    }
    else
    {
        ctx.TextWrapped("System notifications are not available in this session.");
    }
}

void WinShell::ApplySettings()
{
    if (store_)
    {
        IModuleSettings& ps = store_->GetModuleSettings(kSection);
        ps.SetBool(kNotifyKey, notifyEnabled_);
        ps.Save();
    }
}
