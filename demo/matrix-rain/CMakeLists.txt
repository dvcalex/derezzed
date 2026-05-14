add_executable(matrix-rain main.cpp)

target_link_libraries(matrix-rain PRIVATE Derezzed SDL3::SDL3 OpenGL::GL)

# Copy shaders to build output
add_custom_command(TARGET matrix-rain POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:matrix-rain>/res
)
