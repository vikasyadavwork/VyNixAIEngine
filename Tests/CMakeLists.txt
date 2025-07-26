add_executable(vx-scene-tests SceneTests.cpp)
target_link_libraries(vx-scene-tests PRIVATE vx-scene)
add_test(NAME SceneSystems COMMAND vx-scene-tests)

add_executable(vx-skybound-tests SkyboundTests.cpp ../Sandbox/SkyboundGame.cpp)
target_include_directories(vx-skybound-tests PRIVATE ../Sandbox)
target_link_libraries(vx-skybound-tests PRIVATE vx-scene)
add_test(NAME SkyboundGameplay COMMAND vx-skybound-tests)
