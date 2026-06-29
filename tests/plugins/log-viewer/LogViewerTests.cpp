#include "LogViewer.h"
#include "LogViewerSettings.h" // complete type for LogViewer's unique_ptr member

#include <framelift/IModuleContext.h>
#include <framelift/Log.h>
#include <framelift/services/ILogBuffer.h>

#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <tuple>
#include <vector>

namespace
{
// Minimal in-test ring: hands back entries with seq>after, oldest first.
class FakeLogBuffer final : public ILogBuffer
{
public:
    void Push(const int level, const char* msg)
    {
        entries_.emplace_back(entries_.size() + 1, 0LL, level, std::string(msg));
    }
    [[nodiscard]] unsigned long long LatestSeq() const noexcept override
    {
        return entries_.empty() ? 0 : std::get<0>(entries_.back());
    }
    unsigned long long ReadSince(const unsigned long long after, const Visitor v, void* ud) const noexcept override
    {
        unsigned long long last = after;
        for (const auto& e : entries_)
        {
            if (std::get<0>(e) <= after)
            {
                continue;
            }
            v(ud, std::get<0>(e), std::get<1>(e), std::get<2>(e), std::get<3>(e).c_str());
            last = std::get<0>(e);
        }
        return last;
    }

private:
    std::vector<std::tuple<unsigned long long, long long, int, std::string>> entries_;
};

class FakeContext final : public IModuleContext
{
public:
    ILogBuffer* logs = nullptr;
    void* GetServiceRaw(const char* id) const noexcept override
    {
        if (logs && std::strcmp(id, ILogBuffer::InterfaceId) == 0)
        {
            return logs;
        }
        return nullptr;
    }
    void RegisterServiceRaw(const char*, void*) noexcept override {}
    void SubscribeRaw(const char*, void (*)(const void*, void*), void*, void (*)(void*)) noexcept override {}
    void PublishRaw(const char*, const void*) noexcept override {}
};
} // namespace

TEST(LogViewerTest, ShowsBacklogOnOpen)
{
    FakeLogBuffer buf;
    buf.Push(static_cast<int>(Log::Level::Info), "hello");
    buf.Push(static_cast<int>(Log::Level::Warn), "world");

    FakeContext ctx;
    ctx.logs = &buf;

    LogViewer v;
    v.Install(ctx);

    EXPECT_TRUE(v.QmlLines().isEmpty()); // closed: nothing pulled yet
    v.Toggle();                          // open → must drain backlog immediately
    EXPECT_TRUE(v.IsOpen());
    EXPECT_EQ(v.QmlLines().size(), 2);
}

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
