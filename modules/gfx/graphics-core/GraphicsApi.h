#pragma once

#include <string_view>

#ifndef FRAMELIFT_MODULE_GRAPHICS_VULKAN
#define FRAMELIFT_MODULE_GRAPHICS_VULKAN 1
#endif

// The graphics presentation backend the host renders through. Auto prefers Vulkan and
// falls back to OpenGL; explicit selections never switch APIs silently.
enum class GraphicsApi
{
    Auto,
    OpenGL,
    Vulkan,
};

// Parse a backend name (case-insensitive) from the settings file. Empty/auto
// selects auto mode when Vulkan is built; unknown values fall back to OpenGL.
inline GraphicsApi GraphicsApiFromString(std::string_view name)
{
    // Tiny case-insensitive compare against the known names; the set is fixed and small.
    auto iequals = [](std::string_view a, std::string_view b)
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i)
        {
            char ca = a[i], cb = b[i];
            if (ca >= 'A' && ca <= 'Z')
            {
                ca = static_cast<char>(ca - 'A' + 'a');
            }
            if (cb >= 'A' && cb <= 'Z')
            {
                cb = static_cast<char>(cb - 'A' + 'a');
            }
            if (ca != cb)
            {
                return false;
            }
        }
        return true;
    };

#if FRAMELIFT_MODULE_GRAPHICS_VULKAN
    if (name.empty() || iequals(name, "auto"))
    {
        return GraphicsApi::Auto;
    }
    if (iequals(name, "vulkan") || iequals(name, "vk"))
    {
        return GraphicsApi::Vulkan;
    }
#else
    if (name.empty() || iequals(name, "auto"))
    {
        return GraphicsApi::OpenGL;
    }
#endif
    return GraphicsApi::OpenGL;
}

// Canonical lowercase name for a backend (for logging / round-tripping).
inline const char* GraphicsApiName(GraphicsApi api)
{
    switch (api)
    {
    case GraphicsApi::Auto:
#if FRAMELIFT_MODULE_GRAPHICS_VULKAN
        return "auto";
#else
        break;
#endif
    case GraphicsApi::Vulkan:
#if FRAMELIFT_MODULE_GRAPHICS_VULKAN
        return "vulkan";
#else
        break;
#endif
    case GraphicsApi::OpenGL:
        break;
    }
    return "gl";
}
