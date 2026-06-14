#pragma once

class UIContext;
class Hotkeys;

// Right-click context menu for the main video area.
// A pure service interface — the host's concrete implementation also implements
// IRenderable (kept separate so ContextMenu carries a single, unambiguous InterfaceId).
// All virtual methods use a stable C-compatible ABI — no std::function.
// Use the framelift::AddItem() helpers in ContextMenuHelpers.h for lambda items.
class ContextMenu
{
public:
    static constexpr const char* InterfaceId = "framelift.ContextMenu";
    virtual ~ContextMenu() = default;

    // ── Raw item registration (fn-ptr + ud) ───────────────────────────────────
    // cleanup(ud) is called when Clear() is called.

    // Simple item: action(ud) is invoked when selected.
    virtual void AddItemRaw(
        const char* label, void (*action)(void* ud), void* ud, void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;

    // Item with a hotkey hint column. hotkeyName must match a named binding.
    virtual void AddItemWithHotkeyRaw(
        const char* label, const char* hotkeyName, void (*action)(void* ud), void* ud,
        void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;

    virtual void AddSeparator() noexcept = 0;

    // Dynamic sub-menu: builder(builderUd, ctx) is called each frame the sub-menu is open.
    virtual void AddDynamicSubMenuRaw(
        const char* label, void (*builder)(void* ud, UIContext& ctx), void* ud, void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;

    // Drop all items (calls cleanup callbacks). Call before FreeLibrary.
    virtual void Clear() noexcept = 0;

    // Provide the hotkey registry used to look up shortcut strings at render time.
    virtual void SetKeys(Hotkeys* keys) noexcept = 0;

    // Register a builder invoked once when the host assembles the menu. The builder
    // adds this plugin's items (via AddItem* / AddSeparator) at a host-controlled
    // position. cleanup(ud) runs on Clear(). Use framelift::AddSection() for lambdas.
    virtual void AddSectionRaw(
        void (*builder)(ContextMenu& menu, void* ud), void* ud, void (*cleanup)(void* ud) = nullptr
    ) noexcept = 0;
};
