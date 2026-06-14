#pragma once

#include <framelift/Guard.h>
#include <framelift/IRenderable.h>

// IRenderable with the ABI boundary sealed behind plugin-side guards: a throw
// from OnRender()/RedrawNeeded() is caught and logged (framelift::Guard) instead of
// terminating via the noexcept boundary. Plugins that render directly (without
// Panel) should derive this instead of raw IRenderable.
class SafeRenderable : public IRenderable
{
public:
    void Render(const int windowW, const int windowH, UIContext& ctx) noexcept final
    {
        framelift::Guard(
            "Render",
            [&]
            {
                OnRender(windowW, windowH, ctx);
            }
        );
    }

    [[nodiscard]] bool NeedsRedraw() const noexcept final
    {
        return framelift::Guard(
            "NeedsRedraw",
            [&]
            {
                return RedrawNeeded();
            }
        );
    }

protected:
    // Draw UI for this frame; same contract as IRenderable::Render.
    virtual void OnRender(int windowW, int windowH, UIContext& ctx) = 0;

    // Return true to request a render pass even when no events are pending.
    [[nodiscard]] virtual bool RedrawNeeded() const
    {
        return false;
    }
};
