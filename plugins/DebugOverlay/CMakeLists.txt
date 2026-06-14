add_framelift_plugin(DebugOverlay
    "${CMAKE_CURRENT_SOURCE_DIR}/DebugOverlay.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/DebugOverlay.h"
    ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(DebugOverlay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
