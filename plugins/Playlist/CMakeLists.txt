add_framelift_plugin(Playlist
        "${CMAKE_CURRENT_SOURCE_DIR}/Playlist.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Playlist.h"
        ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(Playlist PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Playlist PRIVATE SDL3::SDL3)
