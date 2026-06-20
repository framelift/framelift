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

// Return indices into `candidates` in a deterministic load order: any package that
// provides a module/feature listed in another package's requires/optional is ordered
// before that consumer (so service providers Install first). Ties — and any nodes in
// a dependency cycle — fall back to ascending package-id order.
[[nodiscard]] std::vector<std::size_t> OrderPluginPackages(const std::vector<PluginResolveCandidate>& candidates);
