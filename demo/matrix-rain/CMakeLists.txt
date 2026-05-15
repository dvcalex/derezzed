add_executable(matrix-rain main.cpp)

target_link_libraries(matrix-rain PRIVATE drz SDL3::SDL3 OpenGL::GL)

drz_copy_res(matrix-rain)
