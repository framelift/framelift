# Example FrameLift plugin. Built both in-tree (for bitrot protection) and standalone
# from the packaged SDK (the clean-room CI test). add_framelift_plugin + the
# FRAMELIFT_SDK_SOURCES variable come from the FrameLiftSdk CMake module / find_package(FrameLiftSdk).
add_framelift_plugin(HelloPlugin
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/HelloPlugin.Plugin.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/HelloPlugin.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/HelloPlugin.h"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(HelloPlugin PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
