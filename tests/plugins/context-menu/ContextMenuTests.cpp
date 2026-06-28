#include "ContextMenuModule.h"

#include <QtCore/QVariantMap>

#include <gtest/gtest.h>

// ContextMenuModule is exercised without a host context: with no services wired
// up the audio/subtitle flags report false and Assemble() builds only the core
// items (which QmlExtraItems filters out). These cover the ContextMenu service
// storage ABI and the QML projection.

namespace
{
int g_invoked = 0;
int g_cleaned = 0;
} // namespace

TEST(ContextMenuTest, CoreItemsAreExcludedFromExtraItems)
{
    ContextMenuModule m;
    // First read assembles the core items (Open File, Play / Pause, Quit, …);
    // all of them are filtered from the plugin "extra items" list.
    EXPECT_TRUE(m.QmlExtraItems().isEmpty());
}

TEST(ContextMenuTest, CustomItemAppearsAndInvokes)
{
    g_invoked = 0;
    ContextMenuModule m;
    m.AddItemRaw(
        "Custom Action", [](void*) { ++g_invoked; }, nullptr, nullptr
    );

    const QVariantList items = m.QmlExtraItems();
    ASSERT_EQ(items.size(), 1);
    const QVariantMap row = items.front().toMap();
    EXPECT_EQ(row.value(QStringLiteral("label")).toString(), QStringLiteral("Custom Action"));

    m.invokeExtra(row.value(QStringLiteral("index")).toInt());
    EXPECT_EQ(g_invoked, 1);
}

TEST(ContextMenuTest, SeparatorsAreNotExtraItems)
{
    ContextMenuModule m;
    m.AddSeparator(); // empty label + null action — never an extra item
    EXPECT_TRUE(m.QmlExtraItems().isEmpty());
}

TEST(ContextMenuTest, ClearRunsItemCleanup)
{
    g_cleaned = 0;
    ContextMenuModule m;
    m.AddItemRaw(
        "X", nullptr, nullptr, [](void*) { ++g_cleaned; }
    );
    m.Clear();
    EXPECT_EQ(g_cleaned, 1);
}

TEST(ContextMenuTest, AudioFlagsAreFalseWithoutServices)
{
    const ContextMenuModule m;
    EXPECT_FALSE(m.Muted());
    EXPECT_FALSE(m.NormalizeEnabled());
    EXPECT_FALSE(m.SubtitlesEnabled());
}
