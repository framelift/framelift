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

    // Draw UI for this frame. windowW/windowH are the current client area in pixels.
    virtual void Render(int windowW, int windowH, UIContext& ctx) noexcept = 0;

    // Return true to request a render pass this frame even when no events are pending.
    // Returning false prevents unnecessary GPU work while the UI is static.
    [[nodiscard]] virtual bool NeedsRedraw() const noexcept
    {
        return false;
    }
};