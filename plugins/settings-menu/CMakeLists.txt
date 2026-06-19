add_framelift_plugin(SettingsMenu
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/SettingsMenu.Plugin.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/SettingsMenu.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/SettingsMenu.h"
        "${CMAKE_SOURCE_DIR}/src/Settings.cpp"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(SettingsMenu PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/core"
        "${CMAKE_SOURCE_DIR}/src"
)
