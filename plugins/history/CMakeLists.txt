add_framelift_plugin(History
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/History.Plugin.json"
        QML_URI FrameLift.Plugins.History
        QML_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/History.qml"
        "${CMAKE_CURRENT_SOURCE_DIR}/History.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/History.h"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(History PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
# History persists its entries as JSON via the host IJson service (ctx.GetService<IJson>())
# — no JSON library is linked into the plugin.
