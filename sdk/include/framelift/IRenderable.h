#pragma once

#include <framelift/ui/UIContext.h>

// Interface for objects that draw UI during a frame.
// Render() is called once per frame between IAppWindow::ImGuiBeginFrame()
// and IAppWindow::ImGuiEndFrame(), with an active ImGui frame.
class IRenderable
{
public:
    static constexpr const char* InterfaceId = "framelift.IRenderable";
    virtual ~IRenderable() = default;

    // Draw UI for this frame. The host renders every frame while the window is
    // visible; query the client-area size via ctx.GetMainWindowSize() if needed.
    virtual void Render(UIContext& ctx) noexcept = 0;
};