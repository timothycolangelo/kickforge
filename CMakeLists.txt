cmake_minimum_required(VERSION 3.25)

project(KICKFORGE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(KF_BUILD_TESTS "Build the kick_dsp unit tests" ON)

# Dependencies are pinned here and nowhere else.
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME JUCE
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.17-security.tar.gz
    VERSION 8.0.17)

if(KF_BUILD_TESTS)
    CPMAddPackage(
        NAME Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.9.1.tar.gz
        VERSION 3.9.1)
endif()

add_subdirectory(dsp)
add_subdirectory(plugin)
add_subdirectory(tools)

if(KF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
