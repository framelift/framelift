add_framelift_plugin(Overlay
        "${CMAKE_CURRENT_SOURCE_DIR}/Overlay.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Overlay.h"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(Overlay PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
