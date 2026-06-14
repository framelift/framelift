if (FRAMELIFT_ENABLE_UPDATER)
    add_framelift_plugin(Updater
            "${CMAKE_CURRENT_SOURCE_DIR}/Updater.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/Updater.h"
            ${FRAMELIFT_SDK_SOURCES}
    )
    target_include_directories(Updater PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(Updater PRIVATE FRAMELIFT_ENABLE_UPDATER)
    # Updater parses GitHub release JSON — bring its own json dependency.
    target_link_libraries(Updater PRIVATE winhttp nlohmann_json::nlohmann_json)
endif ()
