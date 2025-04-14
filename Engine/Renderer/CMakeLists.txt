add_library(vx-renderer STATIC)

target_sources(vx-renderer
    PRIVATE
        include/vx/Renderer/GraphicsContext.h
        src/GraphicsContext.cpp
        include/vx/Renderer/Renderer2D.h
        src/Renderer2D.cpp
)

target_include_directories(vx-renderer
    PUBLIC
        include
)

target_compile_features(vx-renderer
    PUBLIC
        cxx_std_20
)

target_link_libraries(vx-renderer
    PUBLIC
        vx-core
    PRIVATE
        opengl32
        windowscodecs
        ole32
        gdi32
)
