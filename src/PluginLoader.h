#pragma once
#include <framelift/IModule.h>
#include <framelift/IRenderable.h>
#include <framelift/PluginABI.h>
#include <string>
#include <vector>

// Loads module DLLs from a directory, filtered by an enabled package-id list.
// Each DLL must export the four framelift_* C symbols below.
//
// extern "C" {
//   IModule*    framelift_create();
//   void         framelift_destroy(IModule*);
//   IRenderable* framelift_get_renderable(IModule*);   // nullptr if not renderable
//   int          framelift_render_order();               // z-order for renderables
// }
class PluginLoader
{
public:
    struct LoadedPlugin
    {
        std::string name;          // package id / enabled-list entry
        std::string moduleFile; // shipped module binary filename.
        void* handle; // HMODULE on Windows, dlopen handle on POSIX
        IModule* module;
        IRenderable* renderable; // may be nullptr
        int renderOrder;
        void (*destroyFn)(IModule*);
        const FrameLiftPluginInfo* info; // identity descriptor (points into the loaded DLL)
    };

    struct AvailablePlugin
    {
        std::string packageId;
        std::string moduleFile;
    };

    // Scans Modules/ for shared libraries and loads packages listed in `enabled`.
    // Dependencies are resolved
    // from embedded package/module metadata before any module object is created.
    // Does NOT call Install(); the caller does that via Registry().Add(p, ctx).
    void LoadAll(const std::string& modulesDir, const std::vector<std::string>& enabled);

    const std::vector<LoadedPlugin>& Plugins() const
    {
        return plugins_;
    }

    // Discover every module binary present in modulesDir, so the settings UI can
    // list and re-enable packages that are currently disabled.
    static std::vector<AvailablePlugin> DiscoverAvailable(const std::string& modulesDir);

    // Calls framelift_destroy and FreeLibrary for every loaded plugin.
    ~PluginLoader();

private:
    std::vector<LoadedPlugin> plugins_;
};
