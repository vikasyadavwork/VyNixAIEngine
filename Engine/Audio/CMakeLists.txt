add_library(vx-audio STATIC
    src/Audio.cpp
    include/vx/Audio/Audio.h
)
target_include_directories(vx-audio PUBLIC include)
target_compile_features(vx-audio PUBLIC cxx_std_20)
target_compile_definitions(vx-audio PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(vx-audio PUBLIC vx-core PRIVATE winmm)

if(VX_BUILD_TESTS)
    add_executable(vx-audio-tests tests/AudioTests.cpp)
    target_link_libraries(vx-audio-tests PRIVATE vx-audio)
    add_test(NAME vx-audio-tests COMMAND vx-audio-tests)
endif()
