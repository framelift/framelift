#include <framelift/PluginExports.h>

#include <gtest/gtest.h>

// Exercises FRAMELIFT_PLUGIN_EXPORT with .render = false on a plugin that draws
// nothing. Lives in its own binary because the macro's extern "C" symbols
// would clash with PluginExportTests.cpp's. The inverse misuse — a
// non-renderable type with render left true — is a static_assert and cannot
// be runtime-tested.

namespace
{
class NonRenderingDummy final : public IPlugin
{
};
} // namespace

FRAMELIFT_PLUGIN_EXPORT(NonRenderingDummy, {
                   .name = "NonRenderingDummy",
                   .version = {1, 0, 0},
                   .render = false,
                   })

TEST(PluginExportNoRenderTest, OptionalMetadataDefaultsToNull)
{
    const FrameLiftPluginInfo* info = framelift_plugin_info();
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->name, "NonRenderingDummy");
    EXPECT_EQ(info->publisher, nullptr);
    EXPECT_EQ(info->description, nullptr);
}

TEST(PluginExportNoRenderTest, RenderableIsNullAndOrderZero)
{
    IPlugin* plugin = framelift_create();
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(framelift_get_renderable(plugin), nullptr);
    EXPECT_EQ(framelift_render_order(), 0);
    framelift_destroy(plugin);
}

TEST(PluginExportNoRenderTest, GetRenderableHelperHandlesBothKinds)
{
    NonRenderingDummy dummy;
    EXPECT_EQ(framelift::detail::GetRenderable<NonRenderingDummy>(&dummy), nullptr);
}