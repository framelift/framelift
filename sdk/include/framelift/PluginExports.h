#pragma once
#include <framelift/IPlugin.h>
#include <framelift/IRenderable.h>
#include <framelift/Log.h>
#include <framelift/PluginABI.h>

#include <exception>
#include <type_traits>

// Place FRAMELIFT_PLUGIN_EXPORT at file scope at the bottom of a plugin's .cpp,
// passing the plugin type and a braced FrameLiftPluginDesc initializer:
//
//   FRAMELIFT_PLUGIN_EXPORT(MyPanel, {
//       .name = "MyPanel",
//       .version = {1, 0, 0},
//       .renderOrder = 50,
//       .publisher = "Acme",
//       .description = "Does a thing",
//   })
//
// A plugin that draws nothing opts out of rendering explicitly:
//
//   FRAMELIFT_PLUGIN_EXPORT(MyService, {
//       .name = "MyService",
//       .version = {1, 0, 0},
//       .render = false,
//   })
//
// `.name` and `.version` are required; the rest default. `.render` defaults to
// true, so a type that does not implement IRenderable fails to compile until
// it either derives IRenderable or states `.render = false` — the intent is
// always visible at the export site.

// Compile-time descriptor consumed by FRAMELIFT_PLUGIN_EXPORT. SDK-side only: it
// never crosses the plugin/host boundary (the macro bakes its fields into the
// ABI-stable FrameLiftPluginInfo), so new optional fields may be appended freely
// without touching the ABI or existing call sites.
struct FrameLiftPluginDesc
{
    const char* name = nullptr;        // required — human-readable plugin name (string literal)
    int version[3] = {0, 0, 0};        // plugin's own product semver {major, minor, patch}
    bool render = true;                // set false for plugins that draw nothing
    int renderOrder = 0;               // draw order when render == true (lower draws first / further back)
    const char* publisher = nullptr;   // optional — author/vendor name
    const char* description = nullptr; // optional — one-line summary
};

// Export specifier for the C entry points. Windows uses dllexport; other
// platforms (e.g. a native Linux build, as used by the unit tests) use default
// ELF visibility. Keeps the export macros portable without changing the Windows ABI.
#ifdef _WIN32
#define FRAMELIFT_PLUGIN_API __declspec(dllexport)
#else
#define FRAMELIFT_PLUGIN_API __attribute__((visibility("default")))
#endif

namespace framelift::detail
{
// Resolves the plugin object's IRenderable face, or nullptr when the type has
// none. Must be a template: `if constexpr` only discards the dead branch in a
// dependent context, so a plain function would fail to compile the static_cast
// for non-renderable types.
template <typename T>
IRenderable* GetRenderable(IPlugin* p)
{
    if constexpr (std::is_base_of_v<IRenderable, T>)
    {
        return static_cast<T*>(p);
    }
    else
    {
        return nullptr;
    }
}
} // namespace framelift::detail

// The descriptor is taken through __VA_ARGS__ so the braced initializer's
// commas survive macro expansion (the preprocessor only protects commas inside
// parentheses, not braces).
//
// framelift_plugin_info() is the identity export: it reports the ABI the plugin was
// built against plus the descriptor's identity fields. The host reads it first,
// before any vtable is touched, to gate the load and log a meaningful identity.
#define FRAMELIFT_PLUGIN_EXPORT(Type, ...)                                                                                  \
    namespace framelift_plugin_detail                                                                                       \
    {                                                                                                                  \
    inline constexpr ::FrameLiftPluginDesc kDesc = __VA_ARGS__;                                                             \
    static_assert(kDesc.name != nullptr, "FRAMELIFT_PLUGIN_EXPORT: .name is required");                                     \
    static_assert(                                                                                                     \
        !kDesc.render || std::is_base_of_v<IRenderable, Type>,                                                         \
        "FRAMELIFT_PLUGIN_EXPORT: " #Type " does not implement IRenderable — "                                              \
        "add .render = false (or derive IRenderable)"                                                                  \
    );                                                                                                                 \
    }                                                                                                                  \
    extern "C"                                                                                                         \
    {                                                                                                                  \
        FRAMELIFT_PLUGIN_API const FrameLiftPluginInfo* framelift_plugin_info()                                                       \
        {                                                                                                              \
            static const FrameLiftPluginInfo info{                                                                          \
                FRAMELIFT_PLUGIN_ABI_MAJOR,                                                                                 \
                FRAMELIFT_PLUGIN_ABI_MINOR,                                                                                 \
                FRAMELIFT_PLUGIN_ABI_PATCH,                                                                                 \
                framelift_plugin_detail::kDesc.name,                                                                        \
                {framelift_plugin_detail::kDesc.version[0], framelift_plugin_detail::kDesc.version[1],                           \
                 framelift_plugin_detail::kDesc.version[2]},                                                                \
                framelift_plugin_detail::kDesc.publisher,                                                                   \
                framelift_plugin_detail::kDesc.description                                                                  \
            };                                                                                                         \
            return &info;                                                                                              \
        }                                                                                                              \
        FRAMELIFT_PLUGIN_API void framelift_set_log_sink(Log::SinkFn fn)                                                         \
        {                                                                                                              \
            Log::SetSink(fn);                                                                                          \
        }                                                                                                              \
        FRAMELIFT_PLUGIN_API IPlugin* framelift_create()                                                                         \
        {                                                                                                              \
            /* Plugin-side catch: a throwing constructor must not unwind into                                          \
               the host. nullptr makes the loader log and skip this plugin. */                                         \
            try                                                                                                        \
            {                                                                                                          \
                return new Type();                                                                                     \
            }                                                                                                          \
            catch (const std::exception& e)                                                                            \
            {                                                                                                          \
                Log::Error("{}: constructor threw: {}", framelift_plugin_detail::kDesc.name, e.what());                     \
            }                                                                                                          \
            catch (...)                                                                                                \
            {                                                                                                          \
                Log::Error("{}: constructor threw a non-standard exception", framelift_plugin_detail::kDesc.name);          \
            }                                                                                                          \
            return nullptr;                                                                                            \
        }                                                                                                              \
        FRAMELIFT_PLUGIN_API void framelift_destroy(IPlugin* f)                                                                  \
        {                                                                                                              \
            delete f;                                                                                                  \
        }                                                                                                              \
        FRAMELIFT_PLUGIN_API IRenderable* framelift_get_renderable(IPlugin* f)                                                   \
        {                                                                                                              \
            return framelift_plugin_detail::kDesc.render ? ::framelift::detail::GetRenderable<Type>(f) : nullptr;                \
        }                                                                                                              \
        FRAMELIFT_PLUGIN_API int framelift_render_order()                                                                        \
        {                                                                                                              \
            return framelift_plugin_detail::kDesc.render ? framelift_plugin_detail::kDesc.renderOrder : 0;                       \
        }                                                                                                              \
    }
