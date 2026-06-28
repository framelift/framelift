#include "LogViewer.h"
#include "LogViewerSettings.h" // complete type for LogViewer's unique_ptr member

#include <gtest/gtest.h>

// LogViewer is exercised without a host context: with no ILogBuffer wired up the
// ring buffer stays empty, so these cover the context-free public surface
// (visibility, filter state, line projection).

TEST(LogViewerTest, StartsClosed)
{
    const LogViewer v;
    EXPECT_FALSE(v.IsOpen());
}

TEST(LogViewerTest, ToggleAndCloseAreIdempotent)
{
    LogViewer v;
    v.Toggle();
    EXPECT_TRUE(v.IsOpen());
    v.close();
    EXPECT_FALSE(v.IsOpen());
    v.close(); // closing an already-closed viewer is a no-op
    EXPECT_FALSE(v.IsOpen());
}

TEST(LogViewerTest, FilterTextRoundTrips)
{
    LogViewer v;
    v.SetFilterText(QStringLiteral("error"));
    EXPECT_EQ(v.FilterText(), QStringLiteral("error"));
}

TEST(LogViewerTest, PerfOnlyRoundTrips)
{
    LogViewer v;
    EXPECT_FALSE(v.PerfOnly());
    v.SetPerfOnly(true);
    EXPECT_TRUE(v.PerfOnly());
}

TEST(LogViewerTest, LinesEmptyWithoutLogBuffer)
{
    LogViewer v;
    EXPECT_TRUE(v.QmlLines().isEmpty());
    v.clearLines(); // safe with nothing buffered
    EXPECT_TRUE(v.QmlLines().isEmpty());
}
