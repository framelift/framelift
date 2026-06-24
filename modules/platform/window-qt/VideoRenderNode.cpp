#include "VideoRenderNode.h"

#include <QtQuick/QQuickWindow>

void VideoRenderNode::render(const RenderState* /*state*/)
{
    if (!renderCb_ || !window_)
    {
        return;
    }

    // The item fills the window, so the target framebuffer rect is the item's logical
    // size scaled by the device pixel ratio. The host video renderer sets its own
    // viewport and clears + letterboxes within this rect.
    const qreal dpr = window_->effectiveDevicePixelRatio();
    const int fbW = static_cast<int>(itemW_ * dpr);
    const int fbH = static_cast<int>(itemH_ * dpr);
    if (fbW <= 0 || fbH <= 0)
    {
        return;
    }

    // Tell Qt's RHI we are issuing raw GL commands outside its tracking, so it can flush
    // pending state and restore afterwards. changedStates() lists what we touch.
    window_->beginExternalCommands();
    renderCb_(fbW, fbH);
    window_->endExternalCommands();
}

QSGRenderNode::StateFlags VideoRenderNode::changedStates() const
{
    // The raw-GL video draw touches these; report them so the SG restores its own state.
    return {DepthState | StencilState | ScissorState | ColorState | BlendState | CullState | ViewportState};
}

QSGRenderNode::RenderingFlags VideoRenderNode::flags() const
{
    // Video fills the item opaquely; it draws in the item's coordinate space.
    return {BoundedRectRendering | OpaqueRendering};
}
