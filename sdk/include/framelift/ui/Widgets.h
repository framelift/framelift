#pragma once

#include <cstdint>
#include <string>

class UIContext;

// ── Compound settings-UI widgets ──────────────────────────────────────────────
// All control functions return true when the value changed this frame.
// They share one layout convention: the label sits on its own line above a
// full-width control, with an optional muted description line beneath. Pass a
// non-empty description string to show always-visible help text under the control.
namespace Widgets
{
// Outcome of a KeybindRow interaction in the current frame.
enum class KeybindAction : std::uint8_t
{
    None,          // no user interaction
    StartCapture,  // user clicked "Rebind"
    CancelCapture, // user clicked "Cancel" while capture was active
    Clear,         // user clicked "Clear"
};

// Float slider: label above, full-width slider, optional description beneath.
bool SliderFloat(UIContext& ctx, const char* label, const char* description, float& value, float min, float max);

// Int slider: label above, full-width slider, optional description beneath.
bool SliderInt(UIContext& ctx, const char* label, const char* description, int& value, int min, int max);

// Checkbox: inline "[x] label", optional description line beneath.
bool Checkbox(UIContext& ctx, const char* label, const char* description, bool& value);

// Single-line text input: label above, full-width field, optional description beneath.
bool InputText(UIContext& ctx, const char* label, const char* description, std::string& value, int maxLen = 512);

// Styled section heading with a separator line below.
void SectionHeader(UIContext& ctx, const char* label);

// Dropdown: label above, full-width combo, optional description beneath.
// items: array of `count` C strings; index: in/out selected index (clamped).
bool Combo(UIContext& ctx, const char* label, const char* description, const char* const* items, int count, int& index);

// Color picker row (RGB, no alpha): label above, optional description beneath.
// rgb: in/out array of 3 floats in 0–1.
bool ColorEdit(UIContext& ctx, const char* label, const char* description, float rgb[3]);

// One row of the keybinds page: label | binding string | Rebind | Clear.
// When isCapturing is true, shows "Press key..." and a Cancel button instead.
// The caller must wrap the call in PushID/PopID for button uniqueness.
KeybindAction KeybindRow(UIContext& ctx, const char* label, const std::string& binding, bool isCapturing);
} // namespace Widgets