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
        src/Input.cpp
        include/vx/Core/Input.h

        include/vx/Core/Application.h
        include/vx/Core/Core.h
        include/vx/Core/ILogSink.h
        include/vx/Core/Log.h
        include/vx/Core/Window.h
 "include/vx/Core/Event.h" "include/vx/Core/Events/WindowEvent.h" "include/vx/Core/Events/KeyEvent.h" "include/vx/Core/Events/MouseEvent.h" "include/vx/Core/Layer.h" "include/vx/Core/LayerStack.h" "src/Layer.cpp" "src/LayerStack.cpp")

target_include_directories(vx-core
    PUBLIC
        include
)

target_compile_features(vx-core
    PUBLIC
        cxx_std_20
)

if(VX_BUILD_TESTS)
    add_executable(vx-core-tests tests/CoreTests.cpp)
    target_link_libraries(vx-core-tests PRIVATE vx-core)
    add_test(NAME vx-core-tests COMMAND vx-core-tests)
endif()
