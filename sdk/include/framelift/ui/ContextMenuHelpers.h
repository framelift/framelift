#pragma once
#include <functional>
#include <framelift/Guard.h>
#include <framelift/ui/ContextMenu.h>
#include <string>
#include <utility>

// ── Lambda/std::function convenience wrappers for ContextMenu ────────────────
// All heap-allocate closures with cleanup callbacks so memory is freed on Clear().

namespace framelift
{

template <typename Fn>
inline void AddItem(ContextMenu& menu, const char* label, Fn&& fn)
{
    struct C
    {
        std::function<void()> fn;

        static void call(void* ud)
        {
            Guard(
                "context-menu action",
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

    menu.AddItemRaw(label, C::call, new C{std::forward<Fn>(fn)}, C::cleanup);
}

template <typename Fn>
inline void AddItem(ContextMenu& menu, const char* label, const char* hotkey, Fn&& fn)
{
    struct C
    {
        std::function<void()> fn;

        static void call(void* ud)
        {
            Guard(
                "context-menu action",
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

    menu.AddItemWithHotkeyRaw(label, hotkey, C::call, new C{std::forward<Fn>(fn)}, C::cleanup);
}

template <typename Fn>
inline void AddItem(ContextMenu& menu, std::string label, Fn&& fn)
{
    AddItem(menu, label.c_str(), std::forward<Fn>(fn));
}

template <typename Fn>
inline void AddItem(ContextMenu& menu, std::string label, std::string hotkey, Fn&& fn)
{
    AddItem(menu, label.c_str(), hotkey.c_str(), std::forward<Fn>(fn));
}

template <typename BuilderFn>
inline void AddDynamicSubMenu(ContextMenu& menu, const char* label, BuilderFn&& builder)
{
    struct C
    {
        std::function<void(UIContext&)> fn;

        static void build(void* ud, UIContext& ctx)
        {
            Guard(
                "context-menu submenu",
                [&]
                {
                    static_cast<C*>(ud)->fn(ctx);
                }
            );
        }

        static void cleanup(void* ud)
        {
            delete static_cast<C*>(ud);
        }
    };

    menu.AddDynamicSubMenuRaw(label, C::build, new C{std::forward<BuilderFn>(builder)}, C::cleanup);
}

template <typename BuilderFn>
inline void AddDynamicSubMenu(ContextMenu& menu, std::string label, BuilderFn&& builder)
{
    AddDynamicSubMenu(menu, label.c_str(), std::forward<BuilderFn>(builder));
}

// Register a section builder: builder(menu) is invoked once when the host assembles
// the menu, letting a plugin add its items at a host-controlled position.
template <typename BuilderFn>
inline void AddSection(ContextMenu& menu, BuilderFn&& builder)
{
    struct C
    {
        std::function<void(ContextMenu&)> fn;

        static void build(ContextMenu& m, void* ud)
        {
            Guard(
                "context-menu section",
                [&]
                {
                    static_cast<C*>(ud)->fn(m);
                }
            );
        }

        static void cleanup(void* ud)
        {
            delete static_cast<C*>(ud);
        }
    };

    menu.AddSectionRaw(C::build, new C{std::forward<BuilderFn>(builder)}, C::cleanup);
}

} // namespace framelift
