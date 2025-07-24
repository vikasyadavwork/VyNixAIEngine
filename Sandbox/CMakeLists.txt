add_executable(Sandbox
    main.cpp
    SkyboundGame.cpp
    SkyboundLayer.cpp
)

target_link_libraries(Sandbox
    PRIVATE
        vx-core
        vx-platform
        vx-renderer
        vx-scene
        vx-audio
)

set_target_properties(Sandbox PROPERTIES OUTPUT_NAME Skybound)
if(MSVC)
    target_link_options(Sandbox PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
endif()
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/Assets" "$<TARGET_FILE_DIR:Sandbox>/Assets"
    VERBATIM
)
install(TARGETS Sandbox RUNTIME DESTINATION .)
