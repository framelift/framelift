add_framelift_plugin(SettingsMenu
        "${CMAKE_CURRENT_SOURCE_DIR}/SettingsMenu.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/SettingsMenu.h"
        "${CMAKE_SOURCE_DIR}/src/Settings.cpp"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(SettingsMenu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_SOURCE_DIR}/src"
)
