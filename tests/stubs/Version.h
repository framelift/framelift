#pragma once

// Test stub for the CMake-generated <Version.h>. The real header is produced
// from src/Version.h.in into the build dir and is not on the unit-test include
// path; the plugin .cpp files compiled by the per-plugin suites only need the
// numeric version components (passed into FRAMELIFT_PLUGIN_EXPORT). Fixed dummy values.
#define FRAMELIFT_VERSION_MAJOR 0
#define FRAMELIFT_VERSION_MINOR 0
#define FRAMELIFT_VERSION_PATCH 0
#define FRAMELIFT_VERSION_STRING "0.0.0"
#define FRAMELIFT_VERSION_DISPLAY "test"
