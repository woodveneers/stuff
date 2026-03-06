cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set architecture for cross-platform builds
if("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(GDCreatorStudio VERSION 1.0.0)

# Use CPM to fetch Geode SDK
include(cmake/CPM.cmake)
CPMAddPackage("gh:geode-sdk/geode#v5.3.0")

# Set up the mod target
setup_geode_mod(${PROJECT_NAME})
