# Explicit host source ownership for FrameLift.exe.
#
# Built-in modules are JSON-authored and reported by FrameLiftBuiltinModules.cmake.
# Source groups are appended by module state so lean builds do not compile code
# for disabled backends.

set(_FRAMELIFT_HOST_CORE_SOURCES
        "${CMAKE_SOURCE_DIR}/src/App.cpp"
        "${CMAKE_SOURCE_DIR}/src/App.h"
        "${CMAKE_SOURCE_DIR}/src/Cli.h"
        "${CMAKE_SOURCE_DIR}/src/FileDialogServiceImpl.cpp"
        "${CMAKE_SOURCE_DIR}/src/FileDialogServiceImpl.h"
        "${CMAKE_SOURCE_DIR}/src/FocusManagerImpl.h"
        "${CMAKE_SOURCE_DIR}/src/FontScan.cpp"
        "${CMAKE_SOURCE_DIR}/src/FontScan.h"
        "${CMAKE_SOURCE_DIR}/src/HotkeysImpl.cpp"
        "${CMAKE_SOURCE_DIR}/src/HotkeysImpl.h"
        "${CMAKE_SOURCE_DIR}/src/Log.cpp"
        "${CMAKE_SOURCE_DIR}/src/main.cpp"
        "${CMAKE_SOURCE_DIR}/src/PluginContext.cpp"
        "${CMAKE_SOURCE_DIR}/src/PluginContext.h"
        "${CMAKE_SOURCE_DIR}/src/PluginLoader.cpp"
        "${CMAKE_SOURCE_DIR}/src/PluginLoader.h"
        "${CMAKE_SOURCE_DIR}/src/PluginRegistry.h"
        "${CMAKE_SOURCE_DIR}/src/PluginResolver.cpp"
        "${CMAKE_SOURCE_DIR}/src/PluginResolver.h"
        "${CMAKE_SOURCE_DIR}/src/PluginSettingsImpl.h"
        "${CMAKE_SOURCE_DIR}/src/ReadAheadCache.h"
        "${CMAKE_SOURCE_DIR}/src/Services.h"
        "${CMAKE_SOURCE_DIR}/src/Settings.cpp"
        "${CMAKE_SOURCE_DIR}/src/Settings.h"
        "${CMAKE_SOURCE_DIR}/src/SettingsMapping.h"
        "${CMAKE_SOURCE_DIR}/src/ThemeUtil.h"
        "${CMAKE_SOURCE_DIR}/src/util.h"
)

set(_FRAMELIFT_HOST_UI_SOURCES
        "${CMAKE_SOURCE_DIR}/src/ui/ContextMenuImpl.cpp"
        "${CMAKE_SOURCE_DIR}/src/ui/ContextMenuImpl.h"
        "${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp"
        "${CMAKE_SOURCE_DIR}/src/ui/Theme.h"
        "${CMAKE_SOURCE_DIR}/src/ui/UIContextImpl.cpp"
        "${CMAKE_SOURCE_DIR}/src/ui/UIContextImpl.h"
)

set(_FRAMELIFT_HOST_WINDOW_SOURCES
        "${CMAKE_SOURCE_DIR}/src/platform/window/SdlAppWindow.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/window/SdlAppWindow.h"
)

set(_FRAMELIFT_HOST_WATCH_SOURCES
        "${CMAKE_SOURCE_DIR}/src/platform/watch/DirWatcher.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/watch/DirWatcher.h"
        "${CMAKE_SOURCE_DIR}/src/platform/watch/LinuxDirWatcher.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/watch/LinuxDirWatcher.h"
        "${CMAKE_SOURCE_DIR}/src/platform/watch/Win32DirWatcher.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/watch/Win32DirWatcher.h"
)

set(_FRAMELIFT_HOST_FFMPEG_SOURCES
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegAudioFilter.cpp"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegAudioFilter.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegAudioOptions.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegAudioOutput.cpp"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegAudioOutput.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegClock.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegFilters.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegHwDecode.cpp"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegHwDecode.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegLetterbox.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegPacketQueue.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegPlayer.cpp"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegPlayer.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegSubtitleBlend.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegSubtitles.cpp"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegSubtitles.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegTrackLabel.h"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/VideoDecodeMode.h"
)

set(_FRAMELIFT_HOST_FFMPEG_VULKAN_SOURCES
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegVulkanDevice.cpp"
        "${CMAKE_SOURCE_DIR}/modules/media/ffmpeg/FFmpegVulkanDevice.h"
)

set(_FRAMELIFT_HOST_GRAPHICS_SOURCES
        "${CMAKE_SOURCE_DIR}/modules/gfx/graphics-core/GraphicsApi.h"
        "${CMAKE_SOURCE_DIR}/modules/gfx/graphics-core/GraphicsBackendFactory.cpp"
        "${CMAKE_SOURCE_DIR}/modules/gfx/graphics-core/IGraphicsBackend.h"
        "${CMAKE_SOURCE_DIR}/modules/gfx/graphics-core/IVideoRenderer.h"
)

set(_FRAMELIFT_HOST_GRAPHICS_OPENGL_SOURCES
        "${CMAKE_SOURCE_DIR}/modules/gfx/opengl/GlGraphicsBackend.cpp"
        "${CMAKE_SOURCE_DIR}/modules/gfx/opengl/GlGraphicsBackend.h"
        "${CMAKE_SOURCE_DIR}/modules/gfx/opengl/GlVideoRenderer.cpp"
        "${CMAKE_SOURCE_DIR}/modules/gfx/opengl/GlVideoRenderer.h"
)

set(_FRAMELIFT_HOST_GRAPHICS_VULKAN_SOURCES
        "${CMAKE_SOURCE_DIR}/modules/gfx/vulkan/VulkanDeviceInfo.h"
        "${CMAKE_SOURCE_DIR}/modules/gfx/vulkan/VulkanGraphicsBackend.cpp"
        "${CMAKE_SOURCE_DIR}/modules/gfx/vulkan/VulkanGraphicsBackend.h"
        "${CMAKE_SOURCE_DIR}/modules/gfx/vulkan/VulkanQueueLock.h"
        "${CMAKE_SOURCE_DIR}/modules/gfx/vulkan/VulkanVideoRenderer.cpp"
        "${CMAKE_SOURCE_DIR}/modules/gfx/vulkan/VulkanVideoRenderer.h"
)

set(FRAMELIFT_HOST_SOURCES
        ${_FRAMELIFT_HOST_CORE_SOURCES}
        ${_FRAMELIFT_HOST_UI_SOURCES}
        ${_FRAMELIFT_HOST_WINDOW_SOURCES}
        ${_FRAMELIFT_HOST_WATCH_SOURCES}
        ${_FRAMELIFT_HOST_FFMPEG_SOURCES}
        ${_FRAMELIFT_HOST_GRAPHICS_SOURCES}
        ${_FRAMELIFT_HOST_GRAPHICS_OPENGL_SOURCES}
)

if (FRAMELIFT_MODULE_GRAPHICS_VULKAN)
    list(APPEND FRAMELIFT_HOST_SOURCES
            ${_FRAMELIFT_HOST_FFMPEG_VULKAN_SOURCES}
            ${_FRAMELIFT_HOST_GRAPHICS_VULKAN_SOURCES}
    )
endif ()

source_group(TREE "${CMAKE_SOURCE_DIR}" FILES ${FRAMELIFT_HOST_SOURCES})
