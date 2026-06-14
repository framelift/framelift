# FrameLift release-build optimizations.
#
# framelift_apply_release_opts(<target>) layers size/speed optimizations onto a
# target's *Release* configuration only, so Debug builds stay fast to compile
# and easy to debug. Applied from the root CMake to the host exe and each
# shipped plugin DLL; intentionally NOT part of add_framelift_plugin() so the public
# SDK module stays dependency-clean.
#
# What it enables (Release only):
#   • IPO/LTO            — cross-TU inlining and pruning (per binary; does not
#                          cross the exe⇄plugin DLL boundary).
#   • -ffunction/-fdata-sections + --gc-sections — drop unreferenced code/data.
#   • -s                 — strip the symbol table from the linked binary.
# The optimization level itself is left at the CMake Release default (-O3 for
# GCC). Switch to -O2 here if binary size ever wins over micro-speed.

include(CheckIPOSupported)

# Probe IPO support once; cache so each target call is cheap.
if (NOT DEFINED FRAMELIFT_IPO_SUPPORTED)
    check_ipo_supported(RESULT _framelift_ipo_ok OUTPUT _framelift_ipo_msg)
    set(FRAMELIFT_IPO_SUPPORTED ${_framelift_ipo_ok} CACHE INTERNAL "IPO/LTO availability")
    if (NOT _framelift_ipo_ok)
        message(STATUS "FrameLift: IPO/LTO unavailable, skipping: ${_framelift_ipo_msg}")
    endif ()
endif ()

function(framelift_apply_release_opts target)
    if (FRAMELIFT_IPO_SUPPORTED)
        set_property(TARGET ${target}
                PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-ffunction-sections;-fdata-sections>)
        target_link_options(${target} PRIVATE
                $<$<CONFIG:Release>:-Wl,--gc-sections;-s>)
        # CMake's IPO property emits plain -flto, which runs the LTRANS phase
        # serially. -flto=auto parallelizes it across cores (faster links) and
        # silences GCC's "using serial compilation" note. GCC-only; Clang's
        # -flto is already parallel via ThinLTO.
        if (FRAMELIFT_IPO_SUPPORTED AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE $<$<CONFIG:Release>:-flto=auto>)
            target_link_options(${target} PRIVATE $<$<CONFIG:Release>:-flto=auto>)
        endif ()
    elseif (MSVC)
        # /Gy: function-level linking, /Gw: optimize global data (both enable
        # the linker's dead-code/data removal below).
        target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:/Gy;/Gw>)
        # /OPT:REF drops unused code; /OPT:ICF folds identical sections.
        target_link_options(${target} PRIVATE
                $<$<CONFIG:Release>:/OPT:REF;/OPT:ICF>)
    endif ()
endfunction()
