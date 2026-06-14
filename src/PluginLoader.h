#pragma once
#include <framelift/IPlugin.h>
#include <framelift/IRenderable.h>
#include <framelift/PluginABI.h>
#include <string>
#include <vector>

// Loads plugin DLLs from a directory, filtered by an enabled-names list.
// Each DLL must export the four framelift_* C symbols below.
//
// extern "C" {
//   IPlugin*    framelift_create();
//   void         framelift_destroy(IPlugin*);
//   IRenderable* framelift_get_renderable(IPlugin*);   // nullptr if not renderable
//   int          framelift_render_order();               // z-order for renderables
// }
class PluginLoader
{
public:
    struct LoadedPlugin
    {
        std::string name; // load key (file stem / enabled-list entry)
        void* handle;     // HMODULE on Windows, dlopen handle on POSIX
        IPlugin* plugin;
        IRenderable* renderable; // may be nullptr
        int renderOrder;
        void (*destroyFn)(IPlugin*);
        const FrameLiftPluginInfo* info; // identity descriptor (points into the loaded DLL)
    };

    // For each name in `enabled`, tries to load the platform plugin file
    // (`pluginsDir + name + ".dll"` on Windows, `+ ".so"` on Linux).
    // Logs and skips DLLs that are absent or missing required symbols.
    // Does NOT call Install() — the caller does that via Registry().Add(p, ctx).
    void LoadAll(const std::string& pluginsDir, const std::vector<std::string>& enabled);

    const std::vector<LoadedPlugin>& Plugins() const
    {
        return plugins_;
    }

    // Discover the base names of every plugin DLL present in pluginsDir
    // (regardless of the enabled list), so the settings UI can list and
    // re-enable plugins that are currently disabled. Returns file stems.
    static std::vector<std::string> DiscoverAvailable(const std::string& pluginsDir);

    // Calls framelift_destroy and FreeLibrary for every loaded plugin.
    ~PluginLoader();

private:
    std::vector<LoadedPlugin> plugins_;
};