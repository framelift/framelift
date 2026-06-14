# Third-party dependencies for the HOST build only.
#
# These are NOT part of the public plugin SDK — after the COM-like ABI redesign,
# nothing crosses the host↔plugin boundary as a third-party type, so plugins do
# not need imgui/spdlog/stb, and only plugins that opt into JSON link nlohmann.
# SDL3, FFmpeg, and libass are set up by FrameLiftPlatformLibs.cmake before this module,
# because the imgui static library links SDL3::SDL3.

include(FetchContent)

# ── spdlog ────────────────────────────────────────────────────────────────────
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(spdlog)

# ── Dear ImGui ────────────────────────────────────────────────────────────────
# The docking-branch tag is a superset of the like-versioned release: it adds
# multi-viewport (ImGuiConfigFlags_ViewportsEnable + UpdatePlatformWindows/
# RenderPlatformWindowsDefault), which the host uses to pop panels out into their
# own OS windows.
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.8-docking
        GIT_SHALLOW TRUE
)
# imgui ships no CMakeLists.txt; FetchContent_MakeAvailable silently skips
# add_subdirectory() in that case (CMake 3.28+), leaving imgui_SOURCE_DIR set.
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(imgui PUBLIC
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

# ── stb (header-only) ─────────────────────────────────────────────────────────
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# ── nlohmann/json (header-only) ───────────────────────────────────────────────
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)
