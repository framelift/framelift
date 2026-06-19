#pragma once
#include <framelift/Abi.h>
#include <framelift/IPluginSettings.h>
#include <framelift/PluginABI.h>
#include <type_traits>

class UIContext;

// Plugin dependency-injection context. Passed into IModule::Install().
// All virtual methods use a stable C-compatible ABI — no STL types cross the boundary.
// Use the non-virtual template helpers (GetService, RegisterService, Publish, Subscribe)
// for ergonomic access; they compile into the plugin and call the raw virtuals below.
class IPluginContext
{
public:
    static constexpr const char* InterfaceId = "framelift.IPluginContext";
    virtual ~IPluginContext() = default;

    // ── Service registry ───────────────────────────────────────────────────────
    // Keys are InterfaceId strings (e.g. T::InterfaceId).
    virtual void* GetServiceRaw(const char* id) const noexcept = 0;
    virtual void RegisterServiceRaw(const char* id, void* svc) noexcept = 0;

    // ── Pub/sub ────────────────────────────────────────────────────────────────
    // Events are keyed by EventId (e.g. TEvent::EventId).
    // cleanup(ud) is called when subscriptions are cleared (plugin unload).
    virtual void SubscribeRaw(
        const char* eventId, void (*cb)(const void* event, void* ud), void* ud, void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;
    virtual void PublishRaw(const char* eventId, const void* payload) noexcept = 0;

    // ── Settings (per-key typed getters) ─────────────────────────────────────
    // Keys are "section.name" (e.g. "playback.hwdec", "ui.panelWidth").
    virtual float GetSettingFloat(const char* key) const noexcept = 0;
    virtual bool GetSettingBool(const char* key) const noexcept = 0;
    virtual int GetSettingInt(const char* key) const noexcept = 0;
    // Writes up to cap-1 chars + NUL into buf; returns full length excl. NUL.
    // Pass buf=nullptr to query the required length.
    virtual int GetSettingString(const char* key, char* buf, int cap) const noexcept = 0;

    // ── Commit settings (SettingsMenu only) ───────────────────────────────────
    virtual void CommitSettingFloat(const char* key, float value) noexcept = 0;
    virtual void CommitSettingBool(const char* key, bool value) noexcept = 0;
    virtual void CommitSettingInt(const char* key, int value) noexcept = 0;
    virtual void CommitSettingString(const char* key, const char* value) noexcept = 0;
    // Persist all committed settings to disk and notify change subscribers.
    virtual void SaveSettings() noexcept = 0;

    // Register a callback invoked after settings are committed.
    // cleanup(ud) is called on plugin unload.
    virtual void RegisterSettingsChangeCallback(
        void (*cb)(void* ud), void* ud, void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;

    // ── Per-plugin settings (INI section) ──────────────────────────────────────
    virtual IPluginSettings& GetPluginSettings(const char* sectionName) noexcept = 0;

    // ── Settings page registration ─────────────────────────────────────────────
    // renderFn(ud, ctx): called each frame while the page is visible.
    // applyFn(ud):       called when the user presses Save.
    // cleanup(ud):       called on plugin unload (may be nullptr).
    // visible=false:     applyFn still runs on Save but the page is not shown.
    virtual void RegisterSettingsPage(
        const char* title, void (*renderFn)(void* ud, UIContext& ctx), void (*applyFn)(void* ud), void* ud,
        bool visible = true, void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;

    // Enumerate all registered settings pages. visit() receives page metadata.
    virtual void EnumerateSettingsPages(
        void (*visit)(
            const char* title, void (*renderFn)(void*, UIContext&), void (*applyFn)(void*), void* ud, bool visible,
            void* visitUd
        ),
        void* visitUd
    ) const noexcept = 0;

    // ── Keybind entry registration ──────────────────────────────────────────────
    // getStr(ud): returns a const char* to the current binding string (plugin-owned).
    // setStr(ud, val): updates the binding string in the plugin.
    virtual void RegisterKeybindEntry(
        const char* label, const char* actionName, const char* (*getStr)(void* ud),
        void (*setStr)(void* ud, const char* val), void* ud
    ) noexcept = 0;

    // Enumerate all registered keybind entries.
    virtual void EnumerateKeybindEntries(
        void (*visit)(
            const char* label, const char* actionName, const char* (*getStr)(void* ud),
            void (*setStr)(void* ud, const char* val), void* ud, void* visitUd
        ),
        void* visitUd
    ) const noexcept = 0;

    // ── Pref path ──────────────────────────────────────────────────────────────
    // Trailing-separator user config dir (e.g. "C:/Users/foo/AppData/Roaming/FrameLift/").
    // Returns full length excl. NUL; pass buf=nullptr to query required size.
    virtual int GetPrefPath(char* buf, int cap) const noexcept = 0;

    // ── Plugin catalogue ────────────────────────────────────────────────────────
    // Enumerate every plugin discovered in the Modules/ directory - both the
    // loaded ones and any present-but-disabled DLLs.
    //   name    — the load key (package id / [plugins] enabled entry); use it for
    //             SetPluginEnabled. Stable identity even when info is synthesized.
    //   info    — full descriptor when loaded == true; when loaded == false only
    //             info.name is meaningful (equals name), the rest is zero/null.
    //   enabled    — whether the plugin is in the persisted enabled list (live:
    //                 reflects SetPluginEnabled toggles made this session).
    //   loaded     — whether the plugin is currently loaded this session.
    //   loadFailed — true iff it was enabled at startup but did not load (a real
    //                error); false for a plugin merely toggled on this session,
    //                which is pending a restart rather than failed.
    // All pointers are valid only for the duration of the call.
    virtual void EnumeratePlugins(
        void (*visit)(
            const char* name, const FrameLiftPluginInfo& info, bool enabled, bool loaded, bool loadFailed, void* visitUd
        ),
        void* visitUd
    ) const noexcept = 0;

    // Add/remove a plugin from the persisted enabled list and save. Unknown names
    // are ignored. The change takes effect on the next application start.
    virtual void SetPluginEnabled(const char* name, bool enabled) noexcept = 0;

    // ── System fonts (ABI 1.1) ──────────────────────────────────────────────────
    // Enumerate installed .ttf/.otf fonts found in the OS font directories. The
    // scan runs lazily on the first call and is cached for the session.
    //   name — display name derived from the filename.
    //   path — absolute font file path.
    // Both pointers are valid only for the duration of each visit() call.
    virtual void EnumerateSystemFonts(
        void (*visit)(const char* name, const char* path, void* visitUd), void* visitUd
    ) const noexcept = 0;

    // ── Settings file (ABI 1.1) ─────────────────────────────────────────────────
    // Absolute path of the on-disk settings.ini. Returns full length excl. NUL;
    // pass buf=nullptr to query the required size.
    virtual int GetSettingsFilePath(char* buf, int cap) const noexcept = 0;
    // Re-read settings.ini from disk into the live settings and re-apply them
    // (fires the settings-change callbacks). Use after the file is edited directly.
    virtual void ReloadSettings() noexcept = 0;

    // ── Convenience templates (non-virtual — compiled into the plugin) ──────────

    template <typename T>
    T* GetService() const noexcept
    {
        return static_cast<T*>(GetServiceRaw(std::remove_const_t<T>::InterfaceId));
    }

    template <typename T, typename... Us>
    void RegisterService(T* svc) noexcept
    {
        RegisterServiceRaw(std::remove_const_t<T>::InterfaceId, const_cast<std::remove_const_t<T>*>(svc));
        (..., RegisterServiceRaw(std::remove_const_t<Us>::InterfaceId, static_cast<std::remove_const_t<Us>*>(svc)));
    }

    template <typename TEvent>
    void Publish(const TEvent& event) noexcept
    {
        PublishRaw(TEvent::EventId, &event);
    }
};
