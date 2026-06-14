#pragma once

#include <framelift/AppEvent.h>
#include <framelift/platform/IMediaPlayer.h>

class Hotkeys;
class IPluginContext;

// Mixin interface for subsystems that participate in event dispatch.
// Does NOT inherit IRenderable — plugins that also render push themselves
// into App::renderables_ separately.
// All hooks are noexcept: they are called across the DLL boundary, where a
// propagating exception is undefined behavior.
class IPlugin
{
public:
    static constexpr const char* InterfaceId = "framelift.IPlugin";
    virtual ~IPlugin() = default;

    // Perform one-time wiring. Called once after all platform services are
    // registered in ctx. Use ctx.GetService<T>() for dependencies.
    virtual void Install(IPluginContext& /*ctx*/) noexcept
    {
    }

    // Register hotkey bindings. Called once by App after all plugins are
    // installed; Hotkeys is injected — read settings via the context getters.
    virtual void BindHotkeys(Hotkeys& /*keys*/) noexcept
    {
    }

    // Handle a platform event. Return true to consume and stop further dispatch.
    // Dispatches to OnKeyDownEvent() for KeyDown events; override that instead
    // when only keyboard navigation is needed.
    virtual bool OnEvent(const AppEvent& e) noexcept
    {
        if (e.type == AppEventType::KeyDown)
        {
            return OnKeyDownEvent(e);
        }
        return false;
    }

    // Called by OnEvent() when a KeyDown event arrives. Override this rather
    // than OnEvent() when the plugin only needs to respond to key presses.
    virtual bool OnKeyDownEvent(const AppEvent&) noexcept
    {
        return false;
    }

    // Receive a decoded media-player event (property change, end-of-file, etc.).
    virtual void OnMediaEvent(const MediaEvent&) noexcept
    {
    }

    // Called once after the main loop exits. Use for teardown that must happen
    // after all events are drained (e.g. applying a pending update).
    virtual void OnShutdown() noexcept
    {
    }
};