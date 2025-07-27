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

if(NOT WIN32)
    message(FATAL_ERROR "The current VyNix platform backend targets Windows desktop.")
endif()
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
# The portable Windows package does not require a separate MSVC runtime installer.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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
if(VX_BUILD_TESTS)
    enable_testing()
endif()

# ------------------------------------------------------------
# Engine Modules
# ------------------------------------------------------------

add_subdirectory(Engine/Core)
add_subdirectory(Engine/Memory)
add_subdirectory(Engine/Renderer)
add_subdirectory(Engine/Platform)
add_subdirectory(Engine/Scene)
add_subdirectory(Engine/Audio)

# ------------------------------------------------------------
# Applications
# ------------------------------------------------------------

if(VX_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif()

# ------------------------------------------------------------
# Tests
# ------------------------------------------------------------

if(VX_BUILD_TESTS)
    add_subdirectory(Tests)
endif()

install(DIRECTORY Assets/ DESTINATION Assets)
install(FILES README.md DESTINATION .)
install(FILES LICENSE DESTINATION .)
set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME VyNixAIEngine)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "VyNixAIEngine-${PROJECT_VERSION}-Windows-x64")
include(CPack)
