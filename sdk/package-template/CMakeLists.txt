# Standalone build root for the packaged FrameLift plugin SDK.
#
# This file sits at the root of the extracted framelift-sdk-<ver>.zip. It locates the
# SDK shipped alongside it and builds the bundled example plugin — the same path
# an external plugin author follows, and the clean-room test CI runs to prove the
# package is self-contained (no third-party packages required).
cmake_minimum_required(VERSION 3.28)
project(FrameLiftPluginSDK LANGUAGES CXX)

find_package(FrameLiftSdk REQUIRED PATHS "${CMAKE_CURRENT_SOURCE_DIR}/cmake" NO_DEFAULT_PATH)

add_subdirectory(examples/HelloPlugin)
