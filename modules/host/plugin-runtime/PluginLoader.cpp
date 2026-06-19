#include "PluginLoader.h"
#include "PluginResolver.h"
#include <chrono>
#include <filesystem>
#include <framelift/Log.h>
#include <framelift/PluginABI.h>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

Log::SinkFn HostLogSink();

namespace
{
#ifdef _WIN32
constexpr const char* kPluginExt = ".dll";

void* OpenLib(const char* path)
{
    return LoadLibraryA(path);
}

void CloseLib(void* h)
{
    FreeLibrary(static_cast<HMODULE>(h));
}

void* FindSym(void* h, const char* name)
{
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(h), name));
}

std::string LastLoadError()
{
    return std::to_string(GetLastError());
}
#else
constexpr const char* kPluginExt = ".so";

void* OpenLib(const char* path)
{
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
}

void CloseLib(void* h)
{
    dlclose(h);
}

void* FindSym(void* h, const char* name)
{
    return dlsym(h, name);
}

std::string LastLoadError()
{
    const char* err = dlerror();
    return err ? err : "unknown error";
}
#endif

template <typename Fn>
Fn LoadSym(void* mod, const char* name)
{
    return reinterpret_cast<Fn>(FindSym(mod, name));
}

struct PackageBinary
{
    std::string moduleFile;
    std::string path;
};

struct PackageCandidate
{
    PackageBinary binary;
    void* handle = nullptr;
    const FrameLiftPluginInfo* info = nullptr;
};

std::vector<PackageBinary> DiscoverPackageBinaries(const std::string& modulesDir)
{
    namespace fs = std::filesystem;
    std::vector<PackageBinary> out;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(modulesDir, ec))
    {
        std::error_code fec;
        if (!entry.is_regular_file(fec) || entry.path().extension() != kPluginExt)
        {
            continue;
        }

        out.push_back({entry.path().filename().string(), entry.path().string()});
    }

    return out;
}

const FrameLiftPluginInfo* ReadPluginInfo(void* handle)
{
    using PluginInfoFn = const FrameLiftPluginInfo* (*)();
    const auto pluginInfoFn = LoadSym<PluginInfoFn>(handle, "framelift_plugin_info");
    return pluginInfoFn ? pluginInfoFn() : nullptr;
}

bool AbiCompatible(const FrameLiftPluginInfo* info)
{
    return info &&
           FrameLiftAbiCompatible(info->abiMajor, info->abiMinor, FRAMELIFT_PLUGIN_ABI_MAJOR, FRAMELIFT_PLUGIN_ABI_MINOR);
}

std::string PackageId(const FrameLiftPluginInfo* info)
{
    return info && info->packageId ? info->packageId : "<unknown>";
}
} // namespace

void PluginLoader::LoadAll(const std::string& modulesDir, const std::vector<std::string>& enabled)
{
    using Clock = std::chrono::steady_clock;
    const auto loadStart = Clock::now();

    std::unordered_map<std::string, PackageCandidate> available;
    for (const auto& binary : DiscoverPackageBinaries(modulesDir))
    {
        void* const handle = OpenLib(binary.path.c_str());
        if (!handle)
        {
            Log::Warn("Module '{}': metadata load failed ({})", binary.moduleFile, LastLoadError());
            continue;
        }

        const FrameLiftPluginInfo* const info = ReadPluginInfo(handle);
        if (!info)
        {
            Log::Warn("Module '{}': missing framelift_plugin_info - rebuild against current SDK", binary.moduleFile);
            CloseLib(handle);
            continue;
        }
        if (!AbiCompatible(info))
        {
            Log::Warn(
                "Plugin package '{}' v{}.{}.{}: ABI {}.{}.{} incompatible with host {}.{}.{} - skipped",
                PackageId(info), info->version[0], info->version[1], info->version[2], info->abiMajor, info->abiMinor,
                info->abiPatch, FRAMELIFT_PLUGIN_ABI_MAJOR, FRAMELIFT_PLUGIN_ABI_MINOR, FRAMELIFT_PLUGIN_ABI_PATCH
            );
            CloseLib(handle);
            continue;
        }

        const auto [it, inserted] = available.emplace(PackageId(info), PackageCandidate{binary, handle, info});
        if (!inserted)
        {
            Log::Warn("Plugin package '{}': duplicate package id - skipped", PackageId(info));
            CloseLib(handle);
        }
    }

    std::vector<PackageCandidate*> enabledCandidates;
    std::vector<PluginResolveCandidate> resolveCandidates;
    for (const auto& packageId : enabled)
    {
        const auto it = available.find(packageId);
        if (it == available.end())
        {
            Log::Warn("Plugin package '{}': not found", packageId);
            continue;
        }

        enabledCandidates.push_back(&it->second);
        resolveCandidates.push_back({it->second.info});
    }

    const std::vector<PluginResolveDecision> decisions =
        ResolvePluginPackages(resolveCandidates, FrameLiftCurrentPlatformId());

    for (std::size_t i = 0; i < enabledCandidates.size(); ++i)
    {
        PackageCandidate& candidate = *enabledCandidates[i];
        const FrameLiftPluginInfo* const info = candidate.info;
        if (!decisions[i].accepted)
        {
            Log::Warn("Plugin package '{}': {} - skipped", PackageId(info), decisions[i].reason);
            CloseLib(candidate.handle);
            candidate.handle = nullptr;
            continue;
        }

        const auto pluginStart = Clock::now();
        using CreateFn = IModule* (*)();
        using DestroyFn = void (*)(IModule*);
        using GetRenderFn = IRenderable* (*)(IModule*);
        using RenderOrderFn = int (*)();

        const auto createFn = LoadSym<CreateFn>(candidate.handle, "framelift_create");
        const auto destroyFn = LoadSym<DestroyFn>(candidate.handle, "framelift_destroy");
        const auto getRenderFn = LoadSym<GetRenderFn>(candidate.handle, "framelift_get_renderable");
        const auto renderOrderFn = LoadSym<RenderOrderFn>(candidate.handle, "framelift_render_order");

        if (!createFn || !destroyFn || !getRenderFn || !renderOrderFn)
        {
            Log::Warn("Plugin package '{}': missing required exports - skipped", PackageId(info));
            CloseLib(candidate.handle);
            candidate.handle = nullptr;
            continue;
        }

        using SetLogSinkFn = void (*)(Log::SinkFn);
        if (const auto setLogSinkFn = LoadSym<SetLogSinkFn>(candidate.handle, "framelift_set_log_sink"))
        {
            setLogSinkFn(HostLogSink());
        }

        IModule* module = createFn();
        if (!module)
        {
            Log::Warn("Plugin package '{}': framelift_create() returned nullptr - skipped", PackageId(info));
            CloseLib(candidate.handle);
            candidate.handle = nullptr;
            continue;
        }

        const int order = renderOrderFn();
        IRenderable* renderable = getRenderFn(module);

        plugins_.push_back(
            {PackageId(info), candidate.binary.moduleFile, candidate.handle, module, renderable, order, destroyFn, info}
        );
        candidate.handle = nullptr; // ownership moved to plugins_

        const std::string by = info->publisher ? std::string(" by ") + info->publisher : std::string();
        const double pluginMs = std::chrono::duration<double, std::milli>(Clock::now() - pluginStart).count();
        Log::Info(
            "Plugin package '{}' v{}.{}.{}{} loaded (abi {}.{}.{}, modules {}, render order {}, {:.1f} ms)",
            PackageId(info), info->version[0], info->version[1], info->version[2], by, info->abiMajor, info->abiMinor,
            info->abiPatch, info->moduleCount, order, pluginMs
        );
    }

    for (auto& [_, candidate] : available)
    {
        if (candidate.handle)
        {
            CloseLib(candidate.handle);
        }
    }

    const double totalMs = std::chrono::duration<double, std::milli>(Clock::now() - loadStart).count();
    Log::Info("Loaded {} plugin package(s) in {:.1f} ms", plugins_.size(), totalMs);
}

std::vector<PluginLoader::AvailablePlugin> PluginLoader::DiscoverAvailable(const std::string& modulesDir)
{
    std::vector<AvailablePlugin> out;
    for (const auto& binary : DiscoverPackageBinaries(modulesDir))
    {
        void* const handle = OpenLib(binary.path.c_str());
        if (!handle)
        {
            out.push_back({binary.moduleFile, binary.moduleFile});
            continue;
        }

        const FrameLiftPluginInfo* const info = ReadPluginInfo(handle);
        if (info && AbiCompatible(info) && info->packageId)
        {
            out.push_back({info->packageId, binary.moduleFile});
        }
        else
        {
            out.push_back({binary.moduleFile, binary.moduleFile});
        }
        CloseLib(handle);
    }
    return out;
}

PluginLoader::~PluginLoader()
{
    for (const auto& p : plugins_)
    {
        p.destroyFn(p.module);
        CloseLib(p.handle);
    }
}
