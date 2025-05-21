add_library(vx-scene STATIC src/Scene.cpp)
target_include_directories(vx-scene PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(vx-scene PUBLIC cxx_std_20)
set_target_properties(vx-scene PROPERTIES FOLDER "Engine")
