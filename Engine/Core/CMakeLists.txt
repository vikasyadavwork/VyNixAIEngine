# ============================================================
# vx-core
# ============================================================

add_library(vx-core STATIC)

target_sources(vx-core
    PRIVATE
        src/Application.cpp
        src/Logger.cpp
        src/ConsoleSink.cpp
        src/Platform.cpp
        src/Timer.cpp

        include/vx/Core/Application.h
        include/vx/Core/Core.h
        include/vx/Core/ILogSink.h
        include/vx/Core/Log.h
        include/vx/Core/Window.h
)

target_include_directories(vx-core
    PUBLIC
        include
)

target_compile_features(vx-core
    PUBLIC
        cxx_std_20
)