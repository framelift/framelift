#include "PackageResolver.h"
#include <algorithm>
#include <cstddef>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
bool Contains(const std::vector<std::string>& list, std::string_view value)
{
    return std::ranges::find(list, value) != list.end();
}

void AddAll(std::unordered_set<std::string>& out, const std::vector<std::string>& list)
{
    for (const std::string& item : list)
    {
        if (!item.empty())
        {
            out.emplace(item);
        }
    }
}

bool PlatformSupported(const ModuleMetadata& module, std::string_view platformId)
{
    return module.platforms.empty() || Contains(module.platforms, platformId);
}

std::string PackageId(const PackageMetadata* meta)
{
    return meta && !meta->packageId.empty() ? meta->packageId : "<unknown>";
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

std::vector<PackageResolveDecision> ResolvePackages(
    const std::vector<PackageResolveCandidate>& candidates, std::string_view platformId
)
{
    std::vector<PackageResolveDecision> decisions(candidates.size());

    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        const PackageMetadata* meta = candidates[i].meta;
        if (!meta || !meta->valid)
        {
            decisions[i].reason = "missing package metadata";
            continue;
        }
        if (meta->packageId.empty())
        {
            decisions[i].reason = "missing package id";
            continue;
        }
        if (meta->modules.empty())
        {
            decisions[i].reason = "package has no enabled modules";
            continue;
        }

        decisions[i].accepted = true;
        for (const ModuleMetadata& module : meta->modules)
        {
            if (module.id.empty())
            {
                decisions[i].accepted = false;
                decisions[i].reason = "module is missing id";
                break;
            }
            if (!PlatformSupported(module, platformId))
            {
                decisions[i].accepted = false;
                decisions[i].reason =
                    "module '" + module.id + "' does not support platform '" + std::string(platformId) + "'";
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
            for (const ModuleMetadata& module : candidates[i].meta->modules)
            {
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

            const PackageMetadata* meta = candidates[i].meta;
            for (const ModuleMetadata& module : meta->modules)
            {
                if (!decisions[i].accepted)
                {
                    break;
                }
                for (const std::string& required : module.requiresModules)
                {
                    if (!required.empty() && !providedModules.contains(required))
                    {
                        decisions[i].accepted = false;
                        decisions[i].reason = PackageId(meta) + " requires missing module '" + required + "'";
                        changed = true;
                        break;
                    }
                }
                for (const std::string& required : module.requiresFeatures)
                {
                    if (!decisions[i].accepted)
                    {
                        break;
                    }
                    if (!required.empty() && !providedFeatures.contains(required))
                    {
                        decisions[i].accepted = false;
                        decisions[i].reason = PackageId(meta) + " requires missing feature '" + required + "'";
                        changed = true;
                        break;
                    }
                }
            }
        }
    }

    return decisions;
}

std::vector<std::size_t> OrderPackages(const std::vector<PackageResolveCandidate>& candidates)
{
    const std::size_t n = candidates.size();

    // Per package: the set of tokens it provides (module ids + features) and the set
    // it depends on (requires + optional, modules + features).
    std::vector<std::unordered_set<std::string>> provides(n);
    std::vector<std::unordered_set<std::string>> needs(n);
    std::vector<std::string> ids(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        const PackageMetadata* meta = candidates[i].meta;
        ids[i] = PackageId(meta);
        if (!meta)
        {
            continue;
        }
        for (const ModuleMetadata& module : meta->modules)
        {
            if (!module.id.empty())
            {
                provides[i].emplace(module.id);
            }
            AddAll(provides[i], module.providesFeatures);
            AddAll(needs[i], module.requiresModules);
            AddAll(needs[i], module.requiresFeatures);
            AddAll(needs[i], module.optionalModules);
            AddAll(needs[i], module.optionalFeatures);
        }
    }

    // Edge i → j when consumer j depends on a token provider i offers. Dedupe edges
    // so indegree counts distinct predecessors.
    std::vector<std::set<std::size_t>> succ(n);
    std::vector<int> indeg(n, 0);
    for (std::size_t j = 0; j < n; ++j)
    {
        for (std::size_t i = 0; i < n; ++i)
        {
            if (i == j)
            {
                continue;
            }
            bool dependsOnI = false;
            for (const auto& token : needs[j])
            {
                if (provides[i].contains(token))
                {
                    dependsOnI = true;
                    break;
                }
            }
            if (dependsOnI && succ[i].insert(j).second)
            {
                ++indeg[j];
            }
        }
    }

    // Kahn's algorithm, draining zero-indegree nodes in ascending package-id order
    // for determinism. Any nodes left in a cycle are appended by package-id order.
    std::vector<std::size_t> order;
    order.reserve(n);
    std::vector<bool> done(n, false);
    while (order.size() < n)
    {
        std::size_t pick = n;
        for (std::size_t i = 0; i < n; ++i)
        {
            if (done[i] || indeg[i] != 0)
            {
                continue;
            }
            if (pick == n || ids[i] < ids[pick])
            {
                pick = i;
            }
        }
        if (pick == n)
        {
            // Cycle: append the remaining nodes in package-id order and stop.
            std::vector<std::size_t> rest;
            for (std::size_t i = 0; i < n; ++i)
            {
                if (!done[i])
                {
                    rest.push_back(i);
                }
            }
            std::ranges::sort(rest, [&](std::size_t a, std::size_t b) { return ids[a] < ids[b]; });
            for (const std::size_t i : rest)
            {
                order.push_back(i);
            }
            break;
        }

        done[pick] = true;
        order.push_back(pick);
        for (const std::size_t s : succ[pick])
        {
            --indeg[s];
        }
    }

    return order;
}
