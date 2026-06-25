add_framelift_plugin(Benchmark
        PLUGIN_JSON "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark.Plugin.json"
        QML_URI FrameLift.Plugins.Benchmark
        QML_ENTRY "${CMAKE_CURRENT_SOURCE_DIR}/core/Benchmark.qml"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Benchmark.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/Benchmark.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/SysStats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/SysStats.h"
    ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(Benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/core")
if (WIN32)
    # psapi → GetProcessMemoryInfo; pdh → GPU Engine performance counters.
    target_link_libraries(Benchmark PRIVATE psapi pdh)
endif ()
