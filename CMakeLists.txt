cmake_minimum_required(VERSION 3.14)
project(artboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ── Platform-free core (no adapter, no OS calls) ──
file(GLOB_RECURSE ARTBOARD_CORE CONFIGURE_DEPENDS
     src/core/*.cpp src/anim/*.cpp src/render/*.cpp src/scene/*.cpp src/input/*.cpp)

add_library(artboard_core STATIC ${ARTBOARD_CORE})
target_include_directories(artboard_core PUBLIC src include)

# ── Unit tests (100% core coverage) ──
enable_testing()
add_executable(artboard_tests tests/coreTests.cpp)
target_link_libraries(artboard_tests PRIVATE artboard_core)
add_test(NAME core COMMAND artboard_tests)

# Adapters live under src/adapter/<device>/ and are compiled by the platform
# build (e.g. the arstro umbrella's web target builds src/adapter/web with emcc).
