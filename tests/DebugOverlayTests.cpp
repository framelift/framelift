#include "DebugOverlay.h"

#include <gtest/gtest.h>

TEST(DebugOverlayTest, TogglesVisibility)
{
    DebugOverlay d;
    EXPECT_FALSE(d.NeedsRedraw()); // closed by default
    d.Toggle();
    EXPECT_TRUE(d.NeedsRedraw());
    d.Toggle();
    EXPECT_FALSE(d.NeedsRedraw());
}

TEST(DebugOverlayTest, MediaEventsDoNotCrashWhileClosed)
{
    DebugOverlay d;
    MediaEvent e;
    e.type = MediaEventType::PropertyChange;
    e.property.prop = PlayerProperty::IdleActive;
    e.property.type = PropertyType::Flag;
    e.property.value.flag = 1;
    d.OnMediaEvent(e); // must be safe with no player/context
    SUCCEED();
}
