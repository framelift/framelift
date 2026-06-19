#include "PluginResolver.h"
#include <string>
#include <unordered_set>

namespace
{
bool Contains(const FrameLiftStringList& list, std::string_view value)
{
    for (int i = 0; i < list.count; ++i)
    {
        const char* item = list.items ? list.items[i] : nullptr;
        if (item && std::string_view(item) == value)
        {
            return true;
        }
    }
    return false;
}

void AddAll(std::unordered_set<std::string>& out, const FrameLiftStringList& list)
{
    for (int i = 0; i < list.count; ++i)
    {
        const char* item = list.items ? list.items[i] : nullptr;
        if (item && item[0])
        {
            out.emplace(item);
        }
    }
}

bool PlatformSupported(const FrameLiftModuleInfo& module, std::string_view platformId)
{
    return module.platforms.count <= 0 || Contains(module.platforms, platformId);
}

std::string PackageId(const FrameLiftPluginInfo* info)
{
    return info && info->packageId ? info->packageId : "<unknown>";
}
} // namespace

const char* FrameLiftCurrentPlatformId() noexcept
{
#ifdef _WIN32
    return "windows";
#elif defined(__linux__)
    return "linux";
#elif defined(__APPLE__)
    return "macos";
#else
    return "unknown";
#endif
}

std::vector<PluginResolveDecision> ResolvePluginPackages(
    const std::vector<PluginResolveCandidate>& candidates, std::string_view platformId
)
{
    std::vector<PluginResolveDecision> decisions(candidates.size());

    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        const FrameLiftPluginInfo* info = candidates[i].info;
        if (!info)
        {
            decisions[i].reason = "missing plugin metadata";
            continue;
        }
        if (!info->packageId || !info->packageId[0])
        {
            decisions[i].reason = "missing package id";
            continue;
        }
        if (!info->modules || info->moduleCount <= 0)
        {
            decisions[i].reason = "package has no enabled modules";
            continue;
        }

        decisions[i].accepted = true;
        for (int m = 0; m < info->moduleCount; ++m)
        {
            const FrameLiftModuleInfo& module = info->modules[m];
            if (!module.id || !module.id[0])
            {
                decisions[i].accepted = false;
                decisions[i].reason = "module is missing id";
                break;
            }
            if (!PlatformSupported(module, platformId))
            {
                decisions[i].accepted = false;
                decisions[i].reason = "module '" + std::string(module.id) + "' does not support platform '" +
                                      std::string(platformId) + "'";
                break;
            }
        }
    }

    bool changed = true;
    while (changed)
    {
        changed = false;

        std::unordered_set<std::string> providedModules;
        std::unordered_set<std::string> providedFeatures;
        for (std::size_t i = 0; i < candidates.size(); ++i)
        {
            if (!decisions[i].accepted)
            {
                continue;
            }
            const FrameLiftPluginInfo* info = candidates[i].info;
            for (int m = 0; m < info->moduleCount; ++m)
            {
                const FrameLiftModuleInfo& module = info->modules[m];
                providedModules.emplace(module.id);
                AddAll(providedFeatures, module.providesFeatures);
            }
        }

        for (std::size_t i = 0; i < candidates.size(); ++i)
        {
            if (!decisions[i].accepted)
            {
                continue;
            }

            const FrameLiftPluginInfo* info = candidates[i].info;
            for (int m = 0; m < info->moduleCount && decisions[i].accepted; ++m)
            {
                const FrameLiftModuleInfo& module = info->modules[m];
                for (int r = 0; r < module.requiresModules.count; ++r)
                {
                    const char* required = module.requiresModules.items ? module.requiresModules.items[r] : nullptr;
                    if (required && !providedModules.contains(required))
                    {
                        decisions[i].accepted = false;
                        decisions[i].reason = PackageId(info) + " requires missing module '" + required + "'";
                        changed = true;
                        break;
                    }
                }
                for (int r = 0; r < module.requiresFeatures.count && decisions[i].accepted; ++r)
                {
                    const char* required = module.requiresFeatures.items ? module.requiresFeatures.items[r] : nullptr;
                    if (required && !providedFeatures.contains(required))
                    {
                        decisions[i].accepted = false;
                        decisions[i].reason = PackageId(info) + " requires missing feature '" + required + "'";
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    return decisions;
}
