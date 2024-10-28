add_library(vx-platform STATIC)

target_sources(vx-platform
    PRIVATE
        src/WindowFactory.cpp
        src/Win32Window.cpp

        include/vx/Platform/Windows/Win32Window.h
)

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
)