#pragma once
#include "FontScan.h"
#include "PluginSettingsImpl.h"
#include "SettingsRegistry.h"
#include <framelift/IPluginContext.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Settings;
class PluginConfig;
class UIContext;

// Host-side settings page and keybind entry (STL types OK — host-internal only).
struct SettingsPageEntry
{
    std::string title;
    void (*renderFn)(void*, UIContext&) = nullptr;
    void (*applyFn)(void*) = nullptr;
    void* ud = nullptr;
    bool visible = true;
    void (*cleanup)(void*) = nullptr;
};

struct KeybindEntryRec
{
    std::string label;
    std::string actionName;
    const char* (*getStr)(void*) = nullptr;
    void (*setStr)(void*, const char*) = nullptr;
    void* ud = nullptr;
};

struct SubscriptionRec
{
    std::string eventId;
    void (*cb)(const void*, void*) = nullptr;
    void* ud = nullptr;
    void (*cleanup)(void*) = nullptr;
};

struct ChangeCallbackRec
{
    void (*cb)(void*) = nullptr;
    void* ud = nullptr;
    void (*cleanup)(void*) = nullptr;
};

// Concrete IPluginContext implementation owned by App.
class PluginContext final : public IPluginContext
{
public:
    PluginContext(
        std::string prefPath, Settings* settings, const std::string& settingsPath,
        PluginConfig* pluginConfig = nullptr, std::string pluginsPath = {}
    );

    int GetPrefPath(char* buf, int cap) const noexcept override;

    void EnumeratePlugins(
        void (*visit)(const char*, const FrameLiftPluginInfo&, bool, bool, bool, void*), void* visitUd
    ) const noexcept override;

    void SetPluginEnabled(const char* name, bool enabled) noexcept override;

    void EnumerateSystemFonts(void (*visit)(const char*, const char*, void*), void* visitUd) const noexcept override;

    // Host feeds the plugin catalogue here after PluginLoader::LoadAll, so
    // SettingsMenu (and any consumer) can list and toggle plugins via the ABI.
    // info is the loaded descriptor, or nullptr for a present-but-disabled DLL.
    void AddPlugin(std::string name, bool enabled, const FrameLiftPluginInfo* info);

    // Settings getters
    float GetSettingFloat(const char* key) const noexcept override;
    bool GetSettingBool(const char* key) const noexcept override;
    int GetSettingInt(const char* key) const noexcept override;
    int GetSettingString(const char* key, char* buf, int cap) const noexcept override;

    // Settings setters (SettingsMenu only)
    void CommitSettingFloat(const char* key, float value) noexcept override;
    void CommitSettingBool(const char* key, bool value) noexcept override;
    void CommitSettingInt(const char* key, int value) noexcept override;
    void CommitSettingString(const char* key, const char* value) noexcept override;
    void SaveSettings() noexcept override;

    int GetSettingsFilePath(char* buf, int cap) const noexcept override;
    void ReloadSettings() noexcept override;

    void EnumerateSettings(
        void (*visit)(const FrameLiftSettingDesc*, void*), void* visitUd
    ) const noexcept override;

    void RegisterSettingsChangeCallback(void (*cb)(void*), void* ud, void (*cleanup)(void*)) noexcept override;

    IPluginSettings& GetPluginSettings(const char* sectionName) noexcept override;

    void RegisterSettingsPage(
        const char* title, void (*renderFn)(void*, UIContext&), void (*applyFn)(void*), void* ud, bool visible,
        void (*cleanup)(void*)
    ) noexcept override;

    void EnumerateSettingsPages(
        void (*visit)(const char*, void (*)(void*, UIContext&), void (*)(void*), void*, bool, void*), void* visitUd
    ) const noexcept override;

    void RegisterKeybindEntry(
        const char* label, const char* actionName, const char* (*getStr)(void*), void (*setStr)(void*, const char*),
        void* ud
    ) noexcept override;

    void EnumerateKeybindEntries(
        void (*visit)(const char*, const char*, const char* (*)(void*), void (*)(void*, const char*), void*, void*),
        void* visitUd
    ) const noexcept override;

    // Drop all subscriber/DLL-owned callbacks before FreeLibrary.
    void ClearSubscriptions();

    // Host-internal direct access to the live settings (avoids the string-key API).
    [[nodiscard]] Settings& GetSettingsDirect() const
    {
        return *settings_;
    }

protected:
    void* GetServiceRaw(const char* id) const noexcept override;
    void RegisterServiceRaw(const char* id, void* ptr) noexcept override;
    void SubscribeRaw(
        const char* id, void (*cb)(const void*, void*), void* ud, void (*cleanup)(void*)
    ) noexcept override;
    void PublishRaw(const char* id, const void* payload) noexcept override;

private:
    std::string prefPath_;
    std::string settingsPath_;
    Settings* settings_;

    // User plugin enablement manifest (plugins.ini) and its path. Null in contexts
    // that don't manage plugin enablement (e.g. unit tests).
    PluginConfig* pluginConfig_ = nullptr;
    std::string pluginsPath_;

    // Field registry bound to *settings_, plus the serialized defaults (for
    // EnumerateSettings' defaultValue). Built once in the constructor.
    SettingsRegistry settingsRegistry_;
    std::unordered_map<std::string, std::string> settingDefaults_;
    std::unordered_map<std::string, void*> registry_;
    std::vector<ChangeCallbackRec> changeCallbacks_;
    std::vector<SubscriptionRec> subscriptions_;
    std::unordered_map<std::string, std::unique_ptr<PluginSettingsImpl>> pluginSettings_;
    std::vector<SettingsPageEntry> settingsPages_;
    std::vector<KeybindEntryRec> keybindEntries_;

    // One catalogue entry per available plugin (loaded or merely present).
    struct PluginRec
    {
        std::string name;           // package id / load key — owns stable storage
        bool enabled;               // true once loaded (enablement is JSON/build-time driven)
        bool loadFailed;            // enabled at startup yet not loaded (fixed at build time)
        const FrameLiftPluginInfo* info; // loaded descriptor, or nullptr if not loaded
    };

    std::vector<PluginRec> pluginCatalog_;

    // System font catalogue — scanned lazily on first EnumerateSystemFonts call.
    mutable std::vector<FontEntry> fontCache_;
    mutable bool fontsScanned_ = false;
};
