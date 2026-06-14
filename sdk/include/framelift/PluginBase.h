#pragma once
#include <framelift/IPlugin.h>
#include <framelift/IPluginContext.h>
#include <framelift/IPluginSettings.h>
#include <framelift/PluginFields.h>
#include <cctype>
#include <string>
#include <vector>

class Hotkeys;
class UIContext;

// Optional base class for plugins. Seals Install() and BindHotkeys() so that
// consistent scaffolding (ctx_ storage, ImGui context, settings load, keybind
// registration) always runs in the right order. Plugins override named hooks.
class PluginBase : public IPlugin
{
public:
    // Stores ctx_, sets the ImGui context, loads settings, registers keybinds,
    // then calls OnInstall for plugin-specific setup.
    // noexcept (ABI boundary): a throw from any hook below is caught
    // plugin-side by framelift::Guard and logged — it never crosses the boundary.
    void Install(IPluginContext& ctx) noexcept final;

    // Forwards to OnBindHotkeys(keys). Each plugin loads its own key strings
    // from IPluginSettings, so no settings argument is needed.
    void BindHotkeys(Hotkeys& keys) noexcept final;

    // Sealed IPlugin event entry points: each guards and forwards to the
    // matching Handle* hook below, so a throwing hook is logged instead of
    // terminating via the noexcept boundary.
    bool OnEvent(const AppEvent& e) noexcept final;
    bool OnKeyDownEvent(const AppEvent& e) noexcept final;
    void OnMediaEvent(const MediaEvent& e) noexcept final;
    void OnShutdown() noexcept final;

protected:
    // Required: name used as the INI section, settings page title, and log label.
    virtual const char* PluginName() const = 0;

    // ── Declarative tables ─────────────────────────────────────────────────────
    // Optional: return descriptor tables over plugin members. When provided, the
    // default LoadSettings/SaveSettings/LoadKeybinds/SaveKeybinds/RegisterKeybinds/
    // OnBindHotkeys implementations below consume them. Overriding one of those
    // hooks replaces the table-driven default for that leg only (call the
    // PluginBase:: version to keep it and add extras).
    virtual std::vector<framelift::SettingsField> SettingsFields()
    {
        return {};
    }

    virtual std::vector<framelift::Keybind> Keybinds()
    {
        return {};
    }

    // ── Event hooks (guarded) ──────────────────────────────────────────────────
    // Override these instead of the sealed IPlugin methods above.
    // Handle a platform event; return true to consume it. Default mirrors
    // IPlugin::OnEvent: dispatches KeyDown to HandleKeyDownEvent.
    virtual bool HandleEvent(const AppEvent& e);

    // Handle a KeyDown event; return true to consume it.
    virtual bool HandleKeyDownEvent(const AppEvent&)
    {
        return false;
    }

    // Receive a decoded media-player event.
    virtual void HandleMediaEvent(const MediaEvent&)
    {
    }

    // Called once after the main loop exits.
    virtual void HandleShutdown()
    {
    }

    // ── Hooks (table-driven defaults) ──────────────────────────────────────────
    // Override to read member fields from the plugin's INI section on startup.
    // Default: loads every SettingsFields() entry.
    virtual void LoadSettings(IPluginSettings& ps);

    // Override to call ctx.RegisterKeybindEntry(...) for each plugin keybind.
    // Default: registers every Keybinds() entry.
    virtual void RegisterKeybinds(IPluginContext& ctx);

    // Override for plugin-specific setup: RegisterService, Subscribe, context menu.
    // Called after settings are loaded and keybinds are registered.
    virtual void OnInstall(IPluginContext& ctx)
    {
    }

    // Override to render the settings page's ImGui widgets.
    virtual void RenderSettings(UIContext& ctx)
    {
    }

    // Override to write current member field values back to IPluginSettings on Apply.
    // Default: saves every SettingsFields() entry.
    virtual void SaveSettings(IPluginSettings& ps);

    // Override to read keybind strings from the plugin's own [<Plugin>.keybinds]
    // section, keyed by bare action name (e.g. "togglePlaylist").
    // Default: loads every Keybinds() entry.
    virtual void LoadKeybinds(IPluginSettings& kps);

    // Override to write current keybind strings back to [<Plugin>.keybinds] on Apply.
    // Default: saves every Keybinds() entry.
    virtual void SaveKeybinds(IPluginSettings& kps);

    // Override to call keys.Bind(...) for each keybind.
    // Default: binds every Keybinds() entry that has an onPress action.
    virtual void OnBindHotkeys(Hotkeys& keys);

    // Returns "PluginName.name" — the namespaced key for the shared keybinds section.
    [[nodiscard]] std::string PrefixedKey(const char* name) const
    {
        return std::string(PluginName()) + "." + name;
    }

    // Returns the plugin's INI settings section in camelCase — PluginName() with a
    // lowercased first letter — so plugin sections match the lowercase core sections
    // ([general], [playback], ...). PluginName() itself stays PascalCase for the
    // settings-page title, log label, and the [plugins] enabled list.
    [[nodiscard]] std::string SettingsSection() const
    {
        std::string s = PluginName();
        if (!s.empty())
        {
            s[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[0])));
        }
        return s;
    }

    // Returns "<settingsSection>.keybinds" — the INI section this plugin's keybinds
    // are stored under, so each plugin solely owns its keybind section (no collision
    // with the host-owned [keybinds] section).
    [[nodiscard]] std::string KeybindsSection() const
    {
        return SettingsSection() + ".keybinds";
    }

    // Helper: registers a settings page wired to RenderSettings() + SaveSettings() + SaveKeybinds().
    // Call from OnInstall() if the plugin has configurable settings.
    void SetupSettingsPage(IPluginContext& ctx, bool visible = true);

    IPluginContext* ctx_ = nullptr;

private:
    // Descriptor tables cached once at the start of Install().
    std::vector<framelift::SettingsField> fields_;
    std::vector<framelift::Keybind> keybinds_;
};
