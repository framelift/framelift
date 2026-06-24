#include "VideoItem.h"
#include "VideoRenderNode.h"

#include <QtQuick/QQuickWindow>

VideoItem::VideoItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void VideoItem::SetRenderCallback(std::function<void(int, int)> cb)
{
    renderCb_ = std::move(cb);
    update();
}

QSGNode* VideoItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData* /*data*/)
{
    auto* node = static_cast<VideoRenderNode*>(old);
    if (!node)
    {
        node = new VideoRenderNode();
    }
    node->SetWindow(window());
    node->SetItemSize(static_cast<int>(width()), static_cast<int>(height()));
    node->SetRenderCallback(renderCb_);
    return node;
}
