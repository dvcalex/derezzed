add_executable(hello main.cpp)

target_link_libraries(hello PRIVATE Derezzed SDL3::SDL3 OpenGL::GL)

# Copy shaders to build output
add_custom_command(TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:hello>/res
)
