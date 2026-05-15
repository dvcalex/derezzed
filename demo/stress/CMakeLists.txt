add_executable(stress main.cpp)

target_link_libraries(stress PRIVATE drz SDL3::SDL3 OpenGL::GL)

drz_copy_res(stress)
