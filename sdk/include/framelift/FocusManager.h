#pragma once

class IPlugin;

// Tracks which plugin currently owns keyboard input.
// Plugins are pushed onto a stack — the most recently acquired sits at the
// back and receives keyboard events exclusively. Panels acquire focus on open
// and release on close; modal states (e.g. keybind capture) do the same.
// Pure abstract — implemented by the host (src/FocusManagerImpl).
class FocusManager
{
public:
    static constexpr const char* InterfaceId = "framelift.FocusManager";
    virtual ~FocusManager() = default;

    // Give `f` keyboard focus. If `f` is already in the stack it is moved to
    // the top, so re-opening an already-open panel is safe to call.
    virtual void Acquire(IPlugin* f) noexcept = 0;

    // Relinquish focus for `f`. The previous owner (if any) resumes focus.
    virtual void Release(IPlugin* f) noexcept = 0;

    // Returns the plugin that currently owns focus, or nullptr if none.
    [[nodiscard]] virtual IPlugin* Focused() const noexcept = 0;
};
