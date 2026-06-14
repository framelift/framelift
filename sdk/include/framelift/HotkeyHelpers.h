#pragma once
#include <cstddef>
#include <functional>
#include <framelift/Guard.h>
#include <framelift/Hotkeys.h>
#include <string>
#include <utility>

// ── Lambda/std::function convenience wrappers for Hotkeys ────────────────────
// All heap-allocate a closure and pass cleanup so the memory is freed on Clear().
// They compile into the plugin (no boundary crossing) and can safely use std::function.

namespace framelift
{

// Unnamed binding: fires when `key` + `mods` is pressed.
template <typename Fn>
inline void Bind(Hotkeys& keys, Key key, Mod mods, Fn&& fn)
{
    struct C
    {
        std::function<void()> fn;

        static void call(void* ud)
        {
            Guard(
                "hotkey action",
                [&]
                {
                    static_cast<C*>(ud)->fn();
                }
            );
        }

        static void cleanup(void* ud)
        {
            delete static_cast<C*>(ud);
        }
    };

    keys.BindRaw(key, mods, C::call, new C{std::forward<Fn>(fn)}, C::cleanup);
}

// Unnamed binding with no modifier.
template <typename Fn>
inline void Bind(Hotkeys& keys, Key key, Fn&& fn)
{
    Bind(keys, key, Mod::None, std::forward<Fn>(fn));
}

// Named binding from a semicolon-separated bind-list string (e.g. "Ctrl+F;F2").
// The first entry is named and rebindable; extras are unnamed aliases.
template <typename Fn>
inline void Bind(Hotkeys& keys, const char* name, const char* bindList, Fn&& fn)
{
    struct C
    {
        std::function<void()> fn;

        static void call(void* ud)
        {
            Guard(
                "hotkey action",
                [&]
                {
                    static_cast<C*>(ud)->fn();
                }
            );
        }

        static void cleanup(void* ud)
        {
            delete static_cast<C*>(ud);
        }
    };

    keys.BindNamedRaw(name, bindList, C::call, new C{std::forward<Fn>(fn)}, C::cleanup);
}

// Named binding from a std::string bindList (convenience overload).
template <typename Fn>
inline void Bind(Hotkeys& keys, const char* name, const std::string& bindList, Fn&& fn)
{
    Bind(keys, name, bindList.c_str(), std::forward<Fn>(fn));
}

// Get shortcut string as std::string (allocates, plugin-side only).
inline std::string GetShortcutString(const Hotkeys& keys, const char* name)
{
    const int len = keys.GetShortcutString(name, nullptr, 0);
    if (len <= 0)
    {
        return {};
    }
    std::string s(static_cast<std::size_t>(len), '\0');
    keys.GetShortcutString(name, s.data(), len + 1);
    return s;
}

} // namespace framelift
