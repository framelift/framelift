#pragma once
#include <framelift/Hotkeys.h>
#include <framelift/IPlugin.h>
#include <vector>

// Dynamic plugin registry. Add() installs the plugin immediately (services must
// be registered in ctx first). Remove() unregisters without calling any teardown.
// Provides event-dispatch helpers used by App's main loop.
class PluginRegistry
{
public:
    void Add(IPlugin* p, IPluginContext& ctx)
    {
        plugins_.push_back(p);
        p->Install(ctx);
    }

    void Remove(IPlugin* p)
    {
        std::erase(plugins_, p);
    }

    // Dispatch to each plugin until one consumes the event. Returns true if consumed.
    bool OnEvent(const AppEvent& e)
    {
        for (auto* p : plugins_)
        {
            if (p->OnEvent(e))
            {
                return true;
            }
        }
        return false;
    }

    // Broadcast a media event to all plugins.
    void OnMediaEvent(const MediaEvent& e)
    {
        for (auto* p : plugins_)
        {
            p->OnMediaEvent(e);
        }
    }

    // Call BindHotkeys on every registered plugin.
    void BindHotkeys(Hotkeys& keys)
    {
        for (auto* p : plugins_)
        {
            p->BindHotkeys(keys);
        }
    }

    // Call OnShutdown on every registered plugin (invoked after the main loop exits).
    void OnShutdown()
    {
        for (auto* p : plugins_)
        {
            p->OnShutdown();
        }
    }

private:
    std::vector<IPlugin*> plugins_;
};