#include "PluginResolver.h"

#include <gtest/gtest.h>
#include <string>

namespace
{
constexpr FrameLiftStringList EmptyList()
{
    return {nullptr, 0};
}

template <int N>
constexpr FrameLiftStringList List(const char* const (&items)[N])
{
    return {items, N};
}

struct PackageFixture
{
    FrameLiftModuleInfo module{};
    FrameLiftPluginInfo info{};

    PackageFixture(
        const char* packageId, const char* moduleId, FrameLiftStringList provides, FrameLiftStringList reqModules,
        FrameLiftStringList reqFeatures, FrameLiftStringList optional, FrameLiftStringList platformList
    )
        : module{moduleId, "Module", nullptr, provides, reqModules, reqFeatures, EmptyList(), optional, platformList},
          info{FRAMELIFT_PLUGIN_ABI_MAJOR,
               FRAMELIFT_PLUGIN_ABI_MINOR,
               FRAMELIFT_PLUGIN_ABI_PATCH,
               packageId,
               packageId,
               packageId,
               {1, 0, 0},
               nullptr,
               nullptr,
               &module,
               1}
    {
    }
};
} // namespace

TEST(PluginResolverTest, AcceptsValidDependencyGraph)
{
    static constexpr const char* const providerFeatures[] = {"history.service"};
    static constexpr const char* const consumerRequires[] = {"history.service"};
    PackageFixture provider{
        "framelift.history", "framelift.history.core", List(providerFeatures), EmptyList(), EmptyList(), EmptyList(),
        EmptyList()};
    PackageFixture consumer{
        "framelift.playlist", "framelift.playlist.core", EmptyList(), EmptyList(), List(consumerRequires), EmptyList(),
        EmptyList()};

    const auto decisions = ResolvePluginPackages({{&provider.info}, {&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
}

TEST(PluginResolverTest, RejectsMissingRequiredModule)
{
    static constexpr const char* const requires[] = {"framelift.history.core"};
    PackageFixture consumer{
        "framelift.playlist", "framelift.playlist.core", EmptyList(), List(requires), EmptyList(), EmptyList(),
        EmptyList()};

    const auto decisions = ResolvePluginPackages({{&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("framelift.history.core"), std::string::npos);
}

TEST(PluginResolverTest, RejectsMissingRequiredFeature)
{
    static constexpr const char* const requires[] = {"history.service"};
    PackageFixture consumer{
        "framelift.playlist", "framelift.playlist.core", EmptyList(), EmptyList(), List(requires), EmptyList(),
        EmptyList()};

    const auto decisions = ResolvePluginPackages({{&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("history.service"), std::string::npos);
}

TEST(PluginResolverTest, RejectsUnsupportedPlatform)
{
    static constexpr const char* const platforms[] = {"windows"};
    PackageFixture updater{
        "framelift.updater", "framelift.updater.core", EmptyList(), EmptyList(), EmptyList(), EmptyList(),
        List(platforms)};

    const auto decisions = ResolvePluginPackages({{&updater.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("platform"), std::string::npos);
}

TEST(PluginResolverTest, CascadesRejectedDependencies)
{
    static constexpr const char* const windowsOnly[] = {"windows"};
    static constexpr const char* const requiresModule[] = {"framelift.provider.core"};
    PackageFixture provider{
        "framelift.provider", "framelift.provider.core", EmptyList(), EmptyList(), EmptyList(), EmptyList(),
        List(windowsOnly)};
    PackageFixture consumer{
        "framelift.consumer", "framelift.consumer.core", EmptyList(), List(requiresModule), EmptyList(), EmptyList(),
        EmptyList()};

    const auto decisions = ResolvePluginPackages({{&provider.info}, {&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_FALSE(decisions[1].accepted);
    EXPECT_NE(decisions[1].reason.find("framelift.provider.core"), std::string::npos);
}

TEST(PluginResolverTest, OptionalFeaturesDoNotGateLoading)
{
    static constexpr const char* const optional[] = {"missing.optional"};
    PackageFixture package{
        "framelift.optional", "framelift.optional.core", EmptyList(), EmptyList(), EmptyList(), List(optional),
        EmptyList()};

    const auto decisions = ResolvePluginPackages({{&package.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].accepted);
}
