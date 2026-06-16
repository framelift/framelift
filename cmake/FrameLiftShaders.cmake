# Compiles the host's GLSL shaders to embedded SPIR-V byte-array headers at build time
# using glslang, fetched and built in-tree (no Vulkan SDK required on dev machines or
# CI — same self-contained model as the other FetchContent deps). Each shader becomes
# a header declaring `const uint32_t <varname>[]`, which the Vulkan renderer includes
# directly — so, like the GL renderer, there are no shader files to ship at runtime.

include(FetchContent)

# glslang as a build-time SPIR-V compiler only. Disable the optimizer (avoids pulling
# in SPIRV-Tools), HLSL, the SPIR-V remapper, tests and install — all we need is the
# `glslang` CLI (the glslang-standalone target). ENABLE_GLSLANG_BINARIES builds it.
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang.git
        GIT_TAG 15.0.0
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glslang)

set(FRAMELIFT_SHADER_GEN_DIR "${CMAKE_BINARY_DIR}/generated/shaders")
file(MAKE_DIRECTORY "${FRAMELIFT_SHADER_GEN_DIR}")

# framelift_compile_shader(<src> <varname> <out-header-path-var>)
#   Compiles <src> (a .vert/.frag) to a SPIR-V C header declaring
#   `const uint32_t <varname>[]` and returns the generated header path.
function(framelift_compile_shader src varname out_var)
    get_filename_component(_name "${src}" NAME)
    set(_hdr "${FRAMELIFT_SHADER_GEN_DIR}/${_name}.spv.h")
    add_custom_command(
            OUTPUT "${_hdr}"
            # -V: Vulkan SPIR-V; --vn: emit a C array named <varname>; --quiet: no chatter.
            COMMAND glslang-standalone -V --quiet --vn ${varname} -o "${_hdr}" "${src}"
            DEPENDS "${src}" glslang-standalone
            COMMENT "Compiling shader ${_name} -> SPIR-V header"
            VERBATIM
    )
    set(${out_var} "${_hdr}" PARENT_SCOPE)
endfunction()
