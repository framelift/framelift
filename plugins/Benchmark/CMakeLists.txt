add_framelift_plugin(Benchmark
    "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Benchmark.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/SysStats.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/SysStats.h"
    ${FRAMELIFT_SDK_SOURCES}
)
target_include_directories(Benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if (WIN32)
    # psapi → GetProcessMemoryInfo; pdh → GPU Engine performance counters.
    target_link_libraries(Benchmark PRIVATE psapi pdh)
endif ()
