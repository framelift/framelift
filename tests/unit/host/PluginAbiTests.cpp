#include "PluginMetadata.h"

#include <framelift/ModuleABI.h>

#include <QJsonArray>
#include <QJsonObject>

#include <gtest/gtest.h>

TEST(PluginAbiTest, MatchingEpochLoads)
{
    EXPECT_TRUE(FrameLiftAbiCompatible(FRAMELIFT_ABI_VERSION, FRAMELIFT_ABI_VERSION));
}

TEST(PluginAbiTest, MismatchedEpochRejectedEitherDirection)
{
    EXPECT_FALSE(FrameLiftAbiCompatible(FRAMELIFT_ABI_VERSION - 1, FRAMELIFT_ABI_VERSION));
    EXPECT_FALSE(FrameLiftAbiCompatible(FRAMELIFT_ABI_VERSION + 1, FRAMELIFT_ABI_VERSION));
}

TEST(PluginAbiTest, MetadataParsesCurrentPluginShape)
{
    const PluginMetadata info = ParsePluginMetadata(QJsonObject{
        {"abi", FRAMELIFT_ABI_VERSION},
        {"pluginId", "framelift.playlist"},
        {"pluginFile", "framelift.playlist"},
        {"name", "Playlist"},
        {"version", QJsonArray{1, 2, 3}},
        {"publisher", "FrameLift"},
        {"description", "Folder playlist"},
        {"providesFeatures", QJsonArray{"playlist.panel", "playlist.navigation"}},
        {"requiresPlugins", QJsonArray{"framelift.history"}},
        {"requiresFeatures", QJsonArray{"history.service"}},
        {"optionalPlugins", QJsonArray{"framelift.context-menu"}},
        {"optionalFeatures", QJsonArray{"ui.context_menu"}},
        {"platforms", QJsonArray{"linux", "windows"}},
    });

    ASSERT_TRUE(info.valid);
    EXPECT_EQ(info.abiVersion, FRAMELIFT_ABI_VERSION);
    EXPECT_EQ(info.pluginId, "framelift.playlist");
    EXPECT_EQ(info.pluginFile, "framelift.playlist");
    EXPECT_EQ(info.name, "Playlist");
    EXPECT_EQ(info.version[0], 1);
    EXPECT_EQ(info.version[1], 2);
    EXPECT_EQ(info.version[2], 3);
    EXPECT_EQ(info.publisher, "FrameLift");
    EXPECT_EQ(info.description, "Folder playlist");
    ASSERT_EQ(info.providesFeatures.size(), 2u);
    EXPECT_EQ(info.providesFeatures[1], "playlist.navigation");
    ASSERT_EQ(info.requiresPlugins.size(), 1u);
    EXPECT_EQ(info.requiresPlugins[0], "framelift.history");
    ASSERT_EQ(info.requiresFeatures.size(), 1u);
    EXPECT_EQ(info.requiresFeatures[0], "history.service");
    ASSERT_EQ(info.optionalPlugins.size(), 1u);
    EXPECT_EQ(info.optionalPlugins[0], "framelift.context-menu");
    ASSERT_EQ(info.optionalFeatures.size(), 1u);
    EXPECT_EQ(info.optionalFeatures[0], "ui.context_menu");
    ASSERT_EQ(info.platforms.size(), 2u);
    EXPECT_EQ(info.platforms[0], "linux");
}

TEST(PluginAbiTest, MetadataWithoutPluginIdIsInvalid)
{
    const PluginMetadata info = ParsePluginMetadata(QJsonObject{{"abi", FRAMELIFT_ABI_VERSION}});
    EXPECT_FALSE(info.valid);
}
