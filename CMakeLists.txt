cmake_minimum_required(VERSION 3.20)
project(optimisation_project LANGUAGES CXX)

# C++20 is required (<format>, <numbers>).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to an optimised build with debug info for single-config generators.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(benchmark main.cpp ${PROJECT_SOURCES})

if(MSVC)
    target_compile_options(benchmark PRIVATE /W4 /utf-8)
else()
    target_compile_options(benchmark PRIVATE -Wall -Wextra)
endif()

# `cmake --build build --target run` builds and runs the benchmark from the project
# root, whatever the generator (the executable's path differs between them).
add_custom_target(run
    COMMAND $<TARGET_FILE:benchmark>
    DEPENDS benchmark
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "Running benchmark (writes benchmark_report.html in the project root)")
