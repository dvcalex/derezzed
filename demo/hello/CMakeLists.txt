add_executable(hello main.cpp)

target_link_libraries(hello PRIVATE drz SDL3::SDL3 OpenGL::GL)

drz_copy_res(hello)
