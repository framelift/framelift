add_framelift_plugin(RemoteStream
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/RemoteStream.Plugin.json"
        QML_URI FrameLift.Plugins.RemoteStream
        QML_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/core/RemoteStream.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/RemoteStream.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/core/RemoteStream.h"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(RemoteStream PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
