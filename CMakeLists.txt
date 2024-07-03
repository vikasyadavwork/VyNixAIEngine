cmake_minimum_required(VERSION 3.25)

project(
    VyNixAIEngine
    VERSION 0.1.0
    DESCRIPTION "VyNix AI Engine"
    LANGUAGES CXX
)

# ------------------------------------------------------------
# C++ Standard
# ------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------
# Output Directories
# ------------------------------------------------------------

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ------------------------------------------------------------
# Build Options
# ------------------------------------------------------------

option(VX_BUILD_TESTS "Build engine tests" ON)
option(VX_BUILD_SANDBOX "Build sandbox application" ON)

# ------------------------------------------------------------
# Engine Modules
# ------------------------------------------------------------

add_subdirectory(Engine/Core)
add_subdirectory(Engine/Memory)

# ------------------------------------------------------------
# Applications
# ------------------------------------------------------------

if(VX_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif()

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

# if(VX_BUILD_TESTS)
#     add_subdirectory(Tests)
# endif()