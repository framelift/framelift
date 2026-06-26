add_framelift_plugin(LogViewer
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/LogViewer.Plugin.json"
        QML_URI FrameLift.Plugins.LogViewer
        QML_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/LogViewer.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/LogViewer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/LogViewer.h"
    ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(LogViewer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
