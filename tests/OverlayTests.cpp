#include "Overlay.h"

#include <gtest/gtest.h>

namespace
{
MediaEvent IdleEvent(const bool idle)
{
    MediaEvent e;
    e.type = MediaEventType::PropertyChange;
    e.property.prop = PlayerProperty::IdleActive;
    e.property.type = PropertyType::Flag;
    e.property.value.flag = idle ? 1 : 0;
    return e;
}
} // namespace

TEST(OverlayTest, StartsIdleAndStatic)
{
    const Overlay o;
    EXPECT_TRUE(o.IsIdle());
    EXPECT_FALSE(o.NeedsRedraw()); // idle screen is static
}

TEST(OverlayTest, IdleActivePropertyTogglesIdleState)
{
    Overlay o;
    o.OnMediaEvent(IdleEvent(false));
    EXPECT_FALSE(o.IsIdle());
    o.OnMediaEvent(IdleEvent(true));
    EXPECT_TRUE(o.IsIdle());
}

TEST(OverlayTest, ShowCommandRequestsRedrawWhilePlaying)
{
    Overlay o;
    o.OnMediaEvent(IdleEvent(false)); // playing
    o.ShowCommand("Mute: On");
    EXPECT_TRUE(o.NeedsRedraw()); // HUD fade window is active
}

TEST(OverlayTest, ShowCommandRequestsRedrawWhileIdle)
{
    Overlay o;
    ASSERT_TRUE(o.IsIdle());       // no file loaded
    o.ShowCommand("File not found"); // e.g. selecting a missing history item
    EXPECT_TRUE(o.NeedsRedraw());  // notification must repaint over the idle screen
}

TEST(OverlayTest, NonPropertyEventsAreIgnored)
{
    Overlay o;
    MediaEvent e;
    e.type = MediaEventType::EndFile; // not a PropertyChange
    o.OnMediaEvent(e);
    EXPECT_TRUE(o.IsIdle()); // unchanged
}
