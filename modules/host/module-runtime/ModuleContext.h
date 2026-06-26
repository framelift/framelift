#pragma once
#include "ModuleSettingsImpl.h"
#include "SettingsRegistry.h"
#include <framelift/IModuleContext.h>
#include <framelift/services/IAppPaths.h>
#include <framelift/services/IPluginCatalog.h>
#include <framelift/services/ISettingsPageRegistry.h>
#include <framelift/services/ISettingsRegistry.h>
#include <framelift/services/ISettingsStore.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Settings;
class PluginConfig;
class QObject;

struct KeybindEntryRec
{
    std::string label;
    std::string actionName;
    const char* (*getStr)(void*) = nullptr;
    void (*setStr)(void*, const char*) = nullptr;
    void* ud = nullptr;
};

struct ModuleSettingRec
{
    std::string key;
    int type = 0;
    std::string desc;
    std::string defaultValue;
    const char* (*getValue)(void*) = nullptr;
    void (*setValue)(void*, const char*) = nullptr;
    void* ud = nullptr;
};

struct SettingsPageRec
{
    std::string id;
    std::string title;
    std::string qmlUrl;
    QObject* viewModel = nullptr;
    int order = 0;
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

// Concrete host context. Implements the bootstrap IModuleContext plus every host
// capability service, and registers itself under each service id at construction so
// plugins can ctx.GetService<ISettingsStore>() etc. Host code holding a ModuleContext
// directly can still call any method without going through the registry.
class ModuleContext final : public IModuleContext,
                            public ISettingsStore,
                            public ISettingsRegistry,
                            public ISettingsPageRegistry,
                            public IPluginCatalog,
                            public IAppPaths
{
public:
    ModuleContext(
        std::string prefPath, Settings* settings, const std::string& settingsPath,
        PluginConfig* pluginConfig = nullptr, std::string pluginsPath = {}
    );

    // Anchored out-of-line so the implicit virtual destructor (and its secondary-base
    // thunks) is emitted in exactly one TU. Without this, every TU that destroys a
    // ModuleContext (App's unique_ptr) emits its own linkonce copy, which mingw + LTO
    // fails to fold into one — a multiple-definition link error.
    ~ModuleContext() override;

    int GetPrefPath(char* buf, int cap) const noexcept override;

    void EnumeratePlugins(
        void (*visit)(const char*, const char*, const int*, const char*, const char*, bool, bool, bool, void*),
        void* visitUd
    ) const noexcept override;

    void SetPluginEnabled(const char* pluginId, bool enabled) noexcept override;

    // A plugin present in plugins/ (loaded or merely discovered). Built by the host
    // and handed to AddPlugin after PluginLoader::LoadAll so SettingsMenu (and any
    // consumer) can list and toggle plugins via the ABI.
    struct PluginCatalogEntry
    {
        std::string id;
        std::string displayName;
        int version[3] = {0, 0, 0};
        std::string publisher;
        std::string description;
        bool enabled = true;
        bool loaded = false;
    };

    void AddPlugin(PluginCatalogEntry entry);

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

    IModuleSettings& GetModuleSettings(const char* sectionName) noexcept override;

    void RegisterModuleSetting(const FrameLiftModuleSettingDesc* desc) noexcept override;

    void EnumerateModuleSettings(
        void (*visit)(const FrameLiftModuleSettingDesc*, void*), void* visitUd
    ) const noexcept override;

    void RegisterSettingsPage(
        const char* id, const char* title, const char* qmlUrl, QObject* viewModel, int order
    ) noexcept override;

    void EnumerateSettingsPages(
        void (*visit)(const FrameLiftSettingsPageDesc*, void*), void* visitUd
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
    std::unordered_map<std::string, std::unique_ptr<ModuleSettingsImpl>> moduleSettings_;
    std::vector<KeybindEntryRec> keybindEntries_;
    std::vector<ModuleSettingRec> moduleSettingEntries_;
    std::vector<SettingsPageRec> settingsPages_;

    // One catalogue entry per available plugin (loaded or merely present). loadFailed
    // is computed once in AddPlugin and snapshots startup state, so a freshly toggled
    // plugin reads as "pending restart", not "failed".
    struct PluginCatalogRec
    {
        std::string id;
        std::string displayName;
        int version[3];
        std::string publisher;
        std::string description;
        bool enabled;
        bool loaded;
        bool loadFailed;
    };

    std::vector<PluginCatalogRec> pluginCatalog_;

};
