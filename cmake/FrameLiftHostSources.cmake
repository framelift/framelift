# Explicit host source ownership for FrameLift.exe.
#
# The built-in module options declared in FrameLiftBuiltinModules.cmake are
# scaffold-only for #34. Keep all source groups in the host target for now so
# the default build stays behaviorally identical.

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
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegAudioFilter.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegAudioFilter.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegAudioOptions.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegAudioOutput.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegAudioOutput.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegClock.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegFilters.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegHwDecode.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegHwDecode.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegLetterbox.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegPacketQueue.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegPlayer.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegPlayer.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegSubtitleBlend.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegSubtitles.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegSubtitles.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegTrackLabel.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegVulkanDevice.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/FFmpegVulkanDevice.h"
        "${CMAKE_SOURCE_DIR}/src/platform/ffmpeg/VideoDecodeMode.h"
)

set(_FRAMELIFT_HOST_GRAPHICS_SOURCES
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/GraphicsApi.h"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/GraphicsBackendFactory.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/IGraphicsBackend.h"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/IVideoRenderer.h"
)

set(_FRAMELIFT_HOST_GRAPHICS_OPENGL_SOURCES
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/GlGraphicsBackend.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/GlGraphicsBackend.h"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/GlVideoRenderer.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/GlVideoRenderer.h"
)

set(_FRAMELIFT_HOST_GRAPHICS_VULKAN_SOURCES
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/VulkanDeviceInfo.h"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/VulkanGraphicsBackend.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/VulkanGraphicsBackend.h"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/VulkanQueueLock.h"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/VulkanVideoRenderer.cpp"
        "${CMAKE_SOURCE_DIR}/src/platform/gfx/VulkanVideoRenderer.h"
)

set(FRAMELIFT_HOST_SOURCES
        ${_FRAMELIFT_HOST_CORE_SOURCES}
        ${_FRAMELIFT_HOST_UI_SOURCES}
        ${_FRAMELIFT_HOST_WINDOW_SOURCES}
        ${_FRAMELIFT_HOST_WATCH_SOURCES}
        ${_FRAMELIFT_HOST_FFMPEG_SOURCES}
        ${_FRAMELIFT_HOST_GRAPHICS_SOURCES}
        ${_FRAMELIFT_HOST_GRAPHICS_OPENGL_SOURCES}
        ${_FRAMELIFT_HOST_GRAPHICS_VULKAN_SOURCES}
)

source_group(TREE "${CMAKE_SOURCE_DIR}/src" PREFIX "src" FILES ${FRAMELIFT_HOST_SOURCES})
