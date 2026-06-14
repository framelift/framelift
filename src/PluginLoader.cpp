#include "PluginLoader.h"
#include <chrono>
#include <filesystem>
#include <framelift/Log.h>
#include <framelift/PluginABI.h>
#include <string>
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

// Defined in src/Log.cpp — the host's spdlog-backed sink, pushed into each plugin.
Log::SinkFn HostLogSink();

namespace
{
// Plugin file extension and the OS dynamic-loader primitives, abstracted so the
// load loop below stays platform-neutral. Plugins are emitted without the "lib"
// prefix (PREFIX "" in add_framelift_plugin), so the enabled-name matches the file
// stem on every platform: <Name>.dll / <Name>.so.
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
} // namespace

void PluginLoader::LoadAll(const std::string& pluginsDir, const std::vector<std::string>& enabled)
{
    using Clock = std::chrono::steady_clock;
    const auto loadStart = Clock::now();

    for (const auto& name : enabled)
    {
        const auto pluginStart = Clock::now();
        const std::string path = pluginsDir + name + kPluginExt;

        void* const handle = OpenLib(path.c_str());
        if (!handle)
        {
            Log::Warn("Plugin '{}': load failed ({})", name, LastLoadError());
            continue;
        }

        using CreateFn = IPlugin* (*)();
        using DestroyFn = void (*)(IPlugin*);
        using GetRenderFn = IRenderable* (*)(IPlugin*);
        using RenderOrderFn = int (*)();

        const auto createFn = LoadSym<CreateFn>(handle, "framelift_create");
        const auto destroyFn = LoadSym<DestroyFn>(handle, "framelift_destroy");
        const auto getRenderFn = LoadSym<GetRenderFn>(handle, "framelift_get_renderable");
        const auto renderOrderFn = LoadSym<RenderOrderFn>(handle, "framelift_render_order");

        if (!createFn || !destroyFn || !getRenderFn || !renderOrderFn)
        {
            Log::Warn("Plugin '{}': missing required exports — skipped", name);
            CloseLib(handle);
            continue;
        }

        using PluginInfoFn = const FrameLiftPluginInfo* (*)();
        const auto pluginInfoFn = LoadSym<PluginInfoFn>(handle, "framelift_plugin_info");
        const FrameLiftPluginInfo* const info = pluginInfoFn ? pluginInfoFn() : nullptr;
        if (!info)
        {
            Log::Warn("Plugin '{}': missing framelift_plugin_info — rebuild against current SDK", name);
            CloseLib(handle);
            continue;
        }
        if (!FrameLiftAbiCompatible(info->abiMajor, info->abiMinor, FRAMELIFT_PLUGIN_ABI_MAJOR, FRAMELIFT_PLUGIN_ABI_MINOR))
        {
            Log::Warn(
                "Plugin '{}' v{}.{}.{}: ABI {}.{}.{} incompatible with host {}.{}.{} — skipped", name, info->version[0],
                info->version[1], info->version[2], info->abiMajor, info->abiMinor, info->abiPatch,
                FRAMELIFT_PLUGIN_ABI_MAJOR, FRAMELIFT_PLUGIN_ABI_MINOR, FRAMELIFT_PLUGIN_ABI_PATCH
            );
            CloseLib(handle);
            continue;
        }

        // Route the plugin's Log::* output into the host logger. Optional — a
        // plugin without the export simply logs nowhere.
        using SetLogSinkFn = void (*)(Log::SinkFn);
        if (const auto setLogSinkFn = LoadSym<SetLogSinkFn>(handle, "framelift_set_log_sink"))
        {
            setLogSinkFn(HostLogSink());
        }

        IPlugin* plugin = createFn();
        if (!plugin)
        {
            Log::Warn("Plugin '{}': framelift_create() returned nullptr — skipped", name);
            CloseLib(handle);
            continue;
        }

        const int order = renderOrderFn();
        IRenderable* renderable = getRenderFn(plugin);

        plugins_.push_back({name, handle, plugin, renderable, order, destroyFn, info});
        // Optional publisher: " by <publisher>" when present, empty otherwise.
        const std::string by = info->publisher ? std::string(" by ") + info->publisher : std::string();
        const double pluginMs = std::chrono::duration<double, std::milli>(Clock::now() - pluginStart).count();
        Log::Info(
            "Plugin '{}' v{}.{}.{}{} loaded (abi {}.{}.{}, render order {}, {:.1f} ms)", name, info->version[0],
            info->version[1], info->version[2], by, info->abiMajor, info->abiMinor, info->abiPatch, order, pluginMs
        );
    }

    const double totalMs = std::chrono::duration<double, std::milli>(Clock::now() - loadStart).count();
    Log::Info("Loaded {} plugin(s) in {:.1f} ms", plugins_.size(), totalMs);
}

std::vector<std::string> PluginLoader::DiscoverAvailable(const std::string& pluginsDir)
{
    namespace fs = std::filesystem;
    std::vector<std::string> names;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(pluginsDir, ec))
    {
        std::error_code fec;
        if (!entry.is_regular_file(fec))
        {
            continue;
        }
        const fs::path& p = entry.path();
        if (p.extension() == kPluginExt)
        {
            names.push_back(p.stem().string());
        }
    }
    return names;
}

PluginLoader::~PluginLoader()
{
    for (const auto& p : plugins_)
    {
        p.destroyFn(p.plugin);
        CloseLib(p.handle);
    }
}