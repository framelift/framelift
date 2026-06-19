#pragma once

// Plugin ABI version (major.minor.patch).
//
// The host loads a plugin iff:
//     plugin.abiMajor == host.abiMajor && plugin.abiMinor <= host.abiMinor
// Patch never affects the decision. The check runs before any vtable is touched.
//
// MAJOR: bump on any breaking change: removing/reordering virtual methods,
// changing a signature, appending to a host-called plugin interface
// (IModule, IRenderable), changing a framelift_* export signature, or changing
// FrameLiftPluginInfo layout. Reset MINOR and PATCH to 0.
// MINOR: bump on backward-compatible additions to host-provided surface:
// appending a method to IPluginContext, adding a new service interface, or adding
// a new optional export. Reset PATCH to 0.
// PATCH: ABI-neutral fixes. Carried and logged but not gated.
//
// MAJOR 2: generalized the host/player/window render hand-off for Vulkan.
// MINOR 1: appended IMediaPlayer::SetSubtitleStyle.
// MINOR 2: appended audio output enumeration and preferences.
// MAJOR 3: plugin identity moved to JSON-authored embedded package/module metadata.
#define FRAMELIFT_PLUGIN_ABI_MAJOR 3
#define FRAMELIFT_PLUGIN_ABI_MINOR 0
#define FRAMELIFT_PLUGIN_ABI_PATCH 0

struct FrameLiftStringList
{
    const char* const* items;
    int count;
};

struct FrameLiftModuleInfo
{
    const char* id;          // Stable dotted module id, e.g. "framelift.playlist.core".
    const char* name;        // Human-readable module name.
    const char* description; // One-line summary, nullptr if unset.
    FrameLiftStringList providesFeatures;
    FrameLiftStringList requiresModules;
    FrameLiftStringList requiresFeatures;
    FrameLiftStringList optionalModules;
    FrameLiftStringList optionalFeatures;
    FrameLiftStringList platforms; // Empty means every platform.
};

// POD identity + ABI descriptor exported by framelift_plugin_info(). The host
// reads it before constructing the plugin. The ABI fields come from the
// JSON-authored abi value and appear first so the loader can read them before
// any later major-gated additions.
struct FrameLiftPluginInfo
{
    int abiMajor;
    int abiMinor;
    int abiPatch;
    const char* packageId;   // Stable dotted package id, e.g. "framelift.playlist".
    const char* moduleFile;  // Shipped module binary basename, e.g. "FrameLift.Playlist.Core".
    const char* name;        // Human-readable package name.
    int version[3];          // Package version from JSON: {major, minor, patch}.
    const char* publisher;   // Optional author/vendor name, nullptr if unset.
    const char* description; // One-line summary, nullptr if unset.
    const FrameLiftModuleInfo* modules;
    int moduleCount;
};

// Load-time compatibility predicate. Header-only and POD so both the host loader
// and the unit tests share one source of truth. Patch is intentionally absent:
// a patch bump is non-breaking by definition and never changes the decision.
inline bool FrameLiftAbiCompatible(int pluginMajor, int pluginMinor, int hostMajor, int hostMinor) noexcept
{
    return pluginMajor == hostMajor && pluginMinor <= hostMinor;
}
