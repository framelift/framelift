#pragma once
#include <framelift/PluginABI.h>
#include <string>
#include <string_view>
#include <vector>

struct PluginResolveCandidate
{
    const FrameLiftPluginInfo* info = nullptr;
};

struct PluginResolveDecision
{
    bool accepted = false;
    std::string reason;
};

[[nodiscard]] const char* FrameLiftCurrentPlatformId() noexcept;

[[nodiscard]] std::vector<PluginResolveDecision> ResolvePluginPackages(
    const std::vector<PluginResolveCandidate>& candidates, std::string_view platformId
);
