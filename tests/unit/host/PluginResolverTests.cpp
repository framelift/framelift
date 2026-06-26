#include "PluginResolver.h"

#include <framelift/ModuleABI.h>

#include <gtest/gtest.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::vector<std::string> EmptyList()
{
    return {};
}

std::vector<std::string> List(std::initializer_list<const char*> items)
{
    return {items.begin(), items.end()};
}

struct PluginFixture
{
    PluginMetadata info{};

    PluginFixture(
        std::string pluginId, std::vector<std::string> provides, std::vector<std::string> reqPlugins,
        std::vector<std::string> reqFeatures, std::vector<std::string> optional, std::vector<std::string> platformList
    )
    {
        info.valid = true;
        info.abiVersion = FRAMELIFT_ABI_VERSION;
        info.pluginId = std::move(pluginId);
        info.name = info.pluginId;
        info.providesFeatures = std::move(provides);
        info.requiresPlugins = std::move(reqPlugins);
        info.requiresFeatures = std::move(reqFeatures);
        info.optionalFeatures = std::move(optional);
        info.platforms = std::move(platformList);
    }
};
} // namespace

TEST(PluginResolverTest, AcceptsValidDependencyGraph)
{
    PluginFixture provider{"framelift.history", List({"history.service"}), EmptyList(), EmptyList(), EmptyList(),
                           EmptyList()};
    PluginFixture consumer{"framelift.playlist", EmptyList(), EmptyList(), List({"history.service"}), EmptyList(),
                           EmptyList()};

    const auto decisions = ResolvePlugins({{&provider.info}, {&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_TRUE(decisions[0].accepted);
    EXPECT_TRUE(decisions[1].accepted);
}

TEST(PluginResolverTest, RejectsMissingRequiredPlugin)
{
    PluginFixture consumer{"framelift.playlist", EmptyList(), List({"framelift.history"}), EmptyList(), EmptyList(),
                           EmptyList()};

    const auto decisions = ResolvePlugins({{&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("framelift.history"), std::string::npos);
}

TEST(PluginResolverTest, RejectsMissingRequiredFeature)
{
    PluginFixture consumer{"framelift.playlist", EmptyList(), EmptyList(), List({"history.service"}), EmptyList(),
                           EmptyList()};

    const auto decisions = ResolvePlugins({{&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_NE(decisions[0].reason.find("history.service"), std::string::npos);
}

TEST(PluginResolverTest, CascadesRejectedDependencies)
{
    PluginFixture provider{"framelift.provider", EmptyList(), EmptyList(), EmptyList(), EmptyList(), List({"windows"})};
    PluginFixture consumer{"framelift.consumer", EmptyList(), List({"framelift.provider"}), EmptyList(), EmptyList(),
                           EmptyList()};

    const auto decisions = ResolvePlugins({{&provider.info}, {&consumer.info}}, "linux");

    ASSERT_EQ(decisions.size(), 2u);
    EXPECT_FALSE(decisions[0].accepted);
    EXPECT_FALSE(decisions[1].accepted);
    EXPECT_NE(decisions[1].reason.find("framelift.provider"), std::string::npos);
}

TEST(PluginResolverTest, OptionalFeaturesDoNotGateLoading)
{
    PluginFixture package{"framelift.optional", EmptyList(), EmptyList(), EmptyList(), List({"missing.optional"}),
                          EmptyList()};

    const auto decisions = ResolvePlugins({{&package.info}}, "linux");

    ASSERT_EQ(decisions.size(), 1u);
    EXPECT_TRUE(decisions[0].accepted);
}

TEST(PluginResolverTest, OrdersProviderBeforeOptionalConsumer)
{
    PluginFixture provider{"framelift.context_menu", List({"ui.context_menu"}), EmptyList(), EmptyList(), EmptyList(),
                           EmptyList()};
    PluginFixture consumer{"framelift.playlist", EmptyList(), EmptyList(), EmptyList(), List({"ui.context_menu"}),
                           EmptyList()};

    // Consumer listed first in the input; ordering must still load the provider first.
    const auto order = OrderPlugins({{&consumer.info}, {&provider.info}});
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1u); // provider (input index 1)
    EXPECT_EQ(order[1], 0u); // consumer (input index 0)
}

TEST(PluginResolverTest, OrdersIndependentPackagesByPackageId)
{
    PluginFixture zzz{"framelift.zzz", EmptyList(), EmptyList(), EmptyList(), EmptyList(), EmptyList()};
    PluginFixture aaa{"framelift.aaa", EmptyList(), EmptyList(), EmptyList(), EmptyList(), EmptyList()};

    const auto order = OrderPlugins({{&zzz.info}, {&aaa.info}});
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 1u); // framelift.aaa sorts first
    EXPECT_EQ(order[1], 0u);
}
