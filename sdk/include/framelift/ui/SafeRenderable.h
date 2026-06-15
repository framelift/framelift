#pragma once

#include <framelift/Guard.h>
#include <framelift/IRenderable.h>

// IRenderable with the ABI boundary sealed behind plugin-side guards: a throw
// from OnRender() is caught and logged (framelift::Guard) instead of terminating
// via the noexcept boundary. Plugins that render directly (without Panel) should
// derive this instead of raw IRenderable.
class SafeRenderable : public IRenderable
{
public:
    void Render(UIContext& ctx) noexcept final
    {
        framelift::Guard(
            "Render",
            [&]
            {
                OnRender(ctx);
            }
        );
    }

protected:
    // Draw UI for this frame; same contract as IRenderable::Render.
    virtual void OnRender(UIContext& ctx) = 0;
};
