cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(levelthumbs VERSION 1.5.0)

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD OFF)

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

set(GEODE_DISABLE_PRECOMPILED_HEADERS ON)
add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Read version from mod.json for runtime metadata/user-agent
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/mod.json" MOD_JSON_CONTENTS)
string(JSON MOD_VERSION GET "${MOD_JSON_CONTENTS}" version)
target_compile_definitions(${PROJECT_NAME} PRIVATE MOD_VERSION="${MOD_VERSION}")

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})
