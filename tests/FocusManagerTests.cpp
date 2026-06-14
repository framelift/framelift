#include "FocusManagerImpl.h"

#include <gtest/gtest.h>
#include <framelift/IPlugin.h>

// FocusManagerImpl is a header-only stack of IPlugin*; the tests only need
// distinct addresses, so trivial IPlugin instances suffice.

namespace
{
class DummyPlugin final : public IPlugin
{
};
} // namespace

TEST(FocusManagerTest, EmptyHasNoFocus)
{
    const FocusManagerImpl fm;
    EXPECT_EQ(fm.Focused(), nullptr);
}

TEST(FocusManagerTest, AcquireSetsFocus)
{
    FocusManagerImpl fm;
    DummyPlugin a;
    fm.Acquire(&a);
    EXPECT_EQ(fm.Focused(), &a);
}

TEST(FocusManagerTest, DoubleAcquireKeepsSingleEntry)
{
    FocusManagerImpl fm;
    DummyPlugin a;
    fm.Acquire(&a);
    fm.Acquire(&a);
    EXPECT_EQ(fm.Focused(), &a);
    fm.Release(&a);
    EXPECT_EQ(fm.Focused(), nullptr); // a single Release clears it — no duplicate left behind
}

TEST(FocusManagerTest, ReacquireMovesToTop)
{
    FocusManagerImpl fm;
    DummyPlugin a, b;
    fm.Acquire(&a);
    fm.Acquire(&b);
    EXPECT_EQ(fm.Focused(), &b);
    fm.Acquire(&a); // re-acquire moves a above b
    EXPECT_EQ(fm.Focused(), &a);
}

TEST(FocusManagerTest, ReleaseTopRevealsPrevious)
{
    FocusManagerImpl fm;
    DummyPlugin a, b;
    fm.Acquire(&a);
    fm.Acquire(&b);
    fm.Release(&b);
    EXPECT_EQ(fm.Focused(), &a);
}

TEST(FocusManagerTest, ReleaseNonAcquiredIsNoOp)
{
    FocusManagerImpl fm;
    DummyPlugin a, b;
    fm.Acquire(&a);
    fm.Release(&b);
    EXPECT_EQ(fm.Focused(), &a);
}

TEST(FocusManagerTest, ReleaseMiddleKeepsTop)
{
    FocusManagerImpl fm;
    DummyPlugin a, b, c;
    fm.Acquire(&a);
    fm.Acquire(&b);
    fm.Acquire(&c);
    fm.Release(&b);
    EXPECT_EQ(fm.Focused(), &c);
    fm.Release(&c);
    EXPECT_EQ(fm.Focused(), &a);
}
