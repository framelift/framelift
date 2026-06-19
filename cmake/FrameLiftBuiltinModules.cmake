# Built-in compile-time module declarations for the host build.
#
# These options are scaffold-only for #34: they document and report module state,
# but source/dependency gating is introduced by later module pilot work.

function(framelift_declare_builtin_module)
    cmake_parse_arguments(_FL_MODULE "" "NAME;DESCRIPTION;DEFAULT" "" ${ARGN})

    if (NOT _FL_MODULE_NAME)
        message(FATAL_ERROR "framelift_declare_builtin_module requires NAME <name>")
    endif ()
    if (NOT DEFINED _FL_MODULE_DEFAULT)
        set(_FL_MODULE_DEFAULT ON)
    endif ()
    if (NOT _FL_MODULE_DEFAULT MATCHES "^(ON|OFF)$")
        message(FATAL_ERROR
                "framelift_declare_builtin_module(${_FL_MODULE_NAME}) DEFAULT must be ON or OFF")
    endif ()

    string(TOUPPER "${_FL_MODULE_NAME}" _FL_MODULE_OPTION_SUFFIX)
    string(REGEX REPLACE "[^A-Z0-9_]" "_" _FL_MODULE_OPTION_SUFFIX "${_FL_MODULE_OPTION_SUFFIX}")
    set(_FL_MODULE_OPTION "FRAMELIFT_MODULE_${_FL_MODULE_OPTION_SUFFIX}")

    set(_FL_MODULE_HELP "Build the ${_FL_MODULE_NAME} built-in FrameLift module.")
    if (_FL_MODULE_DESCRIPTION)
        set(_FL_MODULE_HELP "${_FL_MODULE_DESCRIPTION}")
    endif ()

    option(${_FL_MODULE_OPTION} "${_FL_MODULE_HELP}" ${_FL_MODULE_DEFAULT})

    get_property(_FL_MODULES GLOBAL PROPERTY FRAMELIFT_BUILTIN_MODULES)
    if (NOT _FL_MODULE_NAME IN_LIST _FL_MODULES)
        set_property(GLOBAL APPEND PROPERTY FRAMELIFT_BUILTIN_MODULES "${_FL_MODULE_NAME}")
    endif ()
    set_property(GLOBAL PROPERTY "FRAMELIFT_BUILTIN_MODULE_${_FL_MODULE_NAME}_OPTION" "${_FL_MODULE_OPTION}")
    set_property(GLOBAL PROPERTY "FRAMELIFT_BUILTIN_MODULE_${_FL_MODULE_NAME}_DESCRIPTION" "${_FL_MODULE_HELP}")
endfunction()

function(framelift_report_builtin_modules)
    get_property(_FL_MODULES GLOBAL PROPERTY FRAMELIFT_BUILTIN_MODULES)
    if (NOT _FL_MODULES)
        return()
    endif ()

    message(STATUS "FrameLift built-in modules:")
    foreach (_FL_MODULE IN LISTS _FL_MODULES)
        get_property(_FL_MODULE_OPTION GLOBAL PROPERTY "FRAMELIFT_BUILTIN_MODULE_${_FL_MODULE}_OPTION")
        get_property(_FL_MODULE_DESCRIPTION GLOBAL PROPERTY "FRAMELIFT_BUILTIN_MODULE_${_FL_MODULE}_DESCRIPTION")
        if (${_FL_MODULE_OPTION})
            set(_FL_MODULE_STATE ON)
        else ()
            set(_FL_MODULE_STATE OFF)
        endif ()
        message(STATUS "  ${_FL_MODULE}: ${_FL_MODULE_STATE} (${_FL_MODULE_OPTION}) - ${_FL_MODULE_DESCRIPTION}")
    endforeach ()
endfunction()
