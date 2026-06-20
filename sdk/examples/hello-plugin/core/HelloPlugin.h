#pragma once

#include <framelift/core.h>

// Minimal FrameLift plugin: logs a line on install and registers no UI.
// Demonstrates that the SDK builds a working plugin DLL with ZERO third-party
// dependencies (no imgui/spdlog/stb/json needed).
class HelloPlugin : public ModuleBase
{
protected:
    const char* ModuleName() const override
    {
        return "HelloPlugin";
    }

    void OnInstall(IModuleContext& ctx) override;
};

FRAMELIFT_MODULE_ENTRY(HelloPlugin, {
    .render = false,
})
