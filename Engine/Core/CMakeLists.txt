# ============================================================
# vx-core
# ============================================================

add_library(vx-core STATIC)

target_sources(vx-core
    PRIVATE
        src/Logger.cpp
        src/Platform.cpp
        src/Timer.cpp
 "include/vx/Core/Core.h" "include/vx/Core/ILogSink.h" "src/ConsoleSink.cpp")

target_include_directories(vx-core
    PUBLIC
        include
)

target_compile_features(vx-core
    PUBLIC
        cxx_std_20
)