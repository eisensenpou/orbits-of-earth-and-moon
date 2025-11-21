cmake_minimum_required(VERSION 3.14)
project(EarthAndMoonOrbits VERSION 1.0 LANGUAGES C CXX)

# Require modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put all executables in build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include project headers
include_directories(${PROJECT_SOURCE_DIR}/include)

# ============================================================
# GLAD
# ============================================================
add_library(glad STATIC
    ${PROJECT_SOURCE_DIR}/external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    ${PROJECT_SOURCE_DIR}/external/glad/include
)

# ============================================================
# OpenGL Viewer
# ============================================================
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(orbit-viewer
    ${PROJECT_SOURCE_DIR}/src/orbit_viewer.cpp
)

target_include_directories(orbit-viewer PRIVATE
    ${PROJECT_SOURCE_DIR}/external/glad/include
)

if(UNIX AND NOT APPLE)
    target_link_libraries(orbit-viewer PRIVATE
        glad
        glfw
        OpenGL::GL
        dl
        pthread
    )
else()
    target_link_libraries(orbit-viewer PRIVATE
        glad
        glfw
        OpenGL::GL
    )
endif()

# ============================================================
# CLI: orbit-sim
# ============================================================
file(GLOB SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

list(REMOVE_ITEM SRC_FILES
    ${PROJECT_SOURCE_DIR}/src/orbit_viewer.cpp
)

# Define executable
add_executable(orbit-sim ${SRC_FILES})

# Compiler warnings
if (MSVC)
    target_compile_options(orbit-sim PRIVATE /W4)
else()
    target_compile_options(orbit-sim PRIVATE -Wall -Wextra -pedantic)
endif()

# CURL for Horizons
find_package(CURL REQUIRED)
target_link_libraries(orbit-sim PRIVATE CURL::libcurl)