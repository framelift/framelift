#pragma once

#include <framelift/core.h>

// Minimal FrameLift plugin: logs a line on install and registers no UI.
// Demonstrates that the SDK builds a working plugin DLL with ZERO third-party
// dependencies (no imgui/spdlog/stb/json needed).
class HelloPlugin : public PluginBase
{
protected:
    const char* PluginName() const override
    {
        return "HelloPlugin";
    }

    void OnInstall(IPluginContext& ctx) override;
};
