add_framelift_plugin(Overlay
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/Overlay.Plugin.json"
        QML_URI FrameLift.Plugins.Overlay
        QML_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/Overlay.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/Overlay.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Overlay.h"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(Overlay PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_source_files_properties("${CMAKE_SOURCE_DIR}/assets/img/Icon.png" PROPERTIES QT_RESOURCE_ALIAS Icon.png)
qt_add_resources(
        Overlay overlay_assets
        PREFIX "/qt/qml/FrameLift/Plugins/Overlay"
        FILES "${CMAKE_SOURCE_DIR}/assets/img/Icon.png")
