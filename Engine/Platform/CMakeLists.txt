add_library(vx-platform STATIC)

target_sources(vx-platform
    PRIVATE
        src/WindowFactory.cpp
        src/Win32Window.cpp

        include/vx/Platform/Windows/Win32Window.h
 "include/vx/Platform/Windows/WGLContext.h" "src/WGLContext.cpp")

target_include_directories(vx-platform
    PUBLIC
        include
)

target_compile_features(vx-platform
    PUBLIC
        cxx_std_20
)

target_link_libraries(vx-platform
    PUBLIC
        vx-core
        vx-renderer

    PRIVATE
        opengl32
)

if(VX_BUILD_TESTS)
    add_executable(vx-platform-tests tests/PlatformTests.cpp)
    target_link_libraries(vx-platform-tests PRIVATE vx-platform opengl32)
    add_test(NAME vx-platform-tests COMMAND vx-platform-tests)
endif()
