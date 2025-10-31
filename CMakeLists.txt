cmake_minimum_required(VERSION 3.14)
project(EarthAndMoonOrbits VERSION 1.0 LANGUAGES CXX)

# Require modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output all binaries to build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add include directory
include_directories(${PROJECT_SOURCE_DIR}/include)

# Gather all source files automatically
file(GLOB SRC_FILES src/*.cpp)

# Create executable
add_executable(earth_and_moon_orbits ${SRC_FILES})

# Enable compiler warnings (good for debugging)
if (MSVC)
    target_compile_options(earth_and_moon_orbits PRIVATE /W4)
else()
    target_compile_options(earth_and_moon_orbits PRIVATE -Wall -Wextra -pedantic)
endif()
