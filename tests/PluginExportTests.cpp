#include <framelift/PluginExports.h>

#include <gtest/gtest.h>

// Exercises FRAMELIFT_PLUGIN_EXPORT on a rendering plugin: the macro emits the
// extern "C" entry points into this TU, so the tests call them directly. Only
// one macro invocation fits per binary (the C symbols would clash) — the
// .render = false path lives in PluginExportNoRenderTests.cpp.

namespace
{
class RenderingDummy final : public IPlugin, public IRenderable
{
public:
    void Render(int, int, UIContext&) noexcept override
    {
    }
};
} // namespace

FRAMELIFT_PLUGIN_EXPORT(RenderingDummy, {
                   .name = "RenderingDummy",
                   .version = {2, 5, 9},
                   .renderOrder = 7,
                   .publisher = "Acme",
                   .description = "Does a thing",
                   })

TEST(PluginExportTest, InfoCarriesAbiAndIdentity)
{
    const FrameLiftPluginInfo* info = framelift_plugin_info();
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->abiMajor, FRAMELIFT_PLUGIN_ABI_MAJOR);
    EXPECT_EQ(info->abiMinor, FRAMELIFT_PLUGIN_ABI_MINOR);
    EXPECT_EQ(info->abiPatch, FRAMELIFT_PLUGIN_ABI_PATCH);
    EXPECT_STREQ(info->name, "RenderingDummy");
    EXPECT_EQ(info->version[0], 2);
    EXPECT_EQ(info->version[1], 5);
    EXPECT_EQ(info->version[2], 9);
    EXPECT_STREQ(info->publisher, "Acme");
    EXPECT_STREQ(info->description, "Does a thing");
}

TEST(PluginExportTest, RenderOrderComesFromDescriptor)
{
    EXPECT_EQ(framelift_render_order(), 7);
}

TEST(PluginExportTest, RenderableRoundTrip)
{
    IPlugin* plugin = framelift_create();
    ASSERT_NE(plugin, nullptr);
    EXPECT_NE(framelift_get_renderable(plugin), nullptr);
    framelift_destroy(plugin);
}

TEST(PluginExportTest, DescriptorDefaults)
{
    constexpr FrameLiftPluginDesc desc{};
    EXPECT_EQ(desc.name, nullptr);
    EXPECT_EQ(desc.version[0], 0);
    EXPECT_EQ(desc.version[1], 0);
    EXPECT_EQ(desc.version[2], 0);
    EXPECT_TRUE(desc.render);
    EXPECT_EQ(desc.renderOrder, 0);
    EXPECT_EQ(desc.publisher, nullptr);
    EXPECT_EQ(desc.description, nullptr);
}