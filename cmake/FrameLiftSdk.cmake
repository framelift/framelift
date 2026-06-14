# FrameLift plugin SDK CMake module — the single source of truth for building plugins.
#
# Two modes, selected by FRAMELIFT_SDK_STANDALONE:
#   • In-tree (default): included by the root CMakeLists while building FrameLift.
#     FrameLiftSdk points at sdk/include + the generated-header dir; plugins land next
#     to the host exe in <FrameLift>/Plugins.
#   • Standalone: included by FrameLiftSdkConfig.cmake from an installed SDK package.
#     FrameLiftSdk points at the packaged include/ + src/; no host target exists, so
#     plugins land in <build>/Plugins.
#
# The public SDK target (FrameLiftSdk) is dependency-clean: it exposes only the SDK
# include path. No imgui/spdlog/stb/json — the host owns those.

if (NOT DEFINED FRAMELIFT_SDK_STANDALONE)
    set(FRAMELIFT_SDK_STANDALONE OFF)
endif ()

if (FRAMELIFT_SDK_STANDALONE)
    # Installed layout: <prefix>/cmake/FrameLiftSdk.cmake, <prefix>/include, <prefix>/src
    get_filename_component(_framelift_sdk_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    set(_framelift_sdk_include "${_framelift_sdk_root}/include")
    set(_framelift_sdk_src "${_framelift_sdk_root}/src")
else ()
    set(_framelift_sdk_include "${CMAKE_SOURCE_DIR}/sdk/include")
    set(_framelift_sdk_src "${CMAKE_SOURCE_DIR}/sdk/src")
endif ()

# ── Public plugin SDK (headers only) ──────────────────────────────────────────
if (NOT TARGET FrameLiftSdk)
    add_library(FrameLiftSdk INTERFACE)
    target_include_directories(FrameLiftSdk INTERFACE "${_framelift_sdk_include}")
    if (NOT FRAMELIFT_SDK_STANDALONE)
        # In-tree plugins (Overlay, Updater) consume the generated Version.h.
        target_include_directories(FrameLiftSdk INTERFACE "${CMAKE_BINARY_DIR}")
    endif ()
endif ()

# ── SDK helper sources compiled into every plugin (and, in-tree, the host) ────
# MinGW static-runtime plugins need their own copy of each translation unit, so
# these are compiled in rather than shipped as a shared library.
set(FRAMELIFT_SDK_SOURCES
        "${_framelift_sdk_src}/PluginBase.cpp"
        "${_framelift_sdk_src}/Log.cpp"
        "${_framelift_sdk_src}/Panel.cpp"
        "${_framelift_sdk_src}/Widgets.cpp"
        "${_framelift_sdk_src}/KeyNames.cpp"
)

# ── Helper: add_framelift_plugin(NAME src1 src2 ...) ───────────────────────────────
# Creates a SHARED plugin library that links the SDK and outputs into Plugins/.
macro(add_framelift_plugin NAME)
    add_library(${NAME} SHARED ${ARGN})
    set_target_properties(${NAME} PROPERTIES
            CXX_STANDARD 23
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
            # The host loads plugins by exact name (<Name>.dll); MinGW would
            # otherwise emit lib<Name>.dll and the load would fail.
            PREFIX "")
    target_compile_definitions(${NAME} PRIVATE FRAMELIFT_BUILDING_PLUGIN)
    target_link_libraries(${NAME} PRIVATE FrameLiftSdk)
    if (MINGW)
        target_link_options(${NAME} PRIVATE
                -static-libgcc -static-libstdc++ -static)
    endif ()
    if (TARGET FrameLift)
        set(_framelift_plugin_out "$<TARGET_FILE_DIR:FrameLift>/Plugins")
    else ()
        set(_framelift_plugin_out "${CMAKE_BINARY_DIR}/Plugins")
    endif ()
    # RUNTIME covers the Windows .dll; LIBRARY covers the Linux/macOS .so/.dylib
    # (a SHARED lib is a LIBRARY artifact off-Windows). Set both so plugins always
    # land in Plugins/ regardless of platform.
    set_target_properties(${NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${_framelift_plugin_out}"
            LIBRARY_OUTPUT_DIRECTORY "${_framelift_plugin_out}")
    # Ensure the Plugins/ output directory exists before linking.
    add_custom_command(TARGET ${NAME} PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_framelift_plugin_out}")
endmacro()
