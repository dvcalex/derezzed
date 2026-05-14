cmake_minimum_required(VERSION 3.21)
project(Derezzed VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL3 REQUIRED CONFIG)

add_library(${PROJECT_NAME} STATIC
    src/engine.cpp
    src/logger.cpp
    src/gl/shader.cpp
    src/gl/mesh.cpp
    src/gl/renderer.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  include
    SYSTEM PUBLIC vendor
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SDL3::SDL3
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

option(DRZ_BACKEND_GL "" ON)

if(DRZ_BACKEND_GL)
  find_package(OpenGL REQUIRED)

  target_sources(${PROJECT_NAME} PRIVATE vendor/glad/src/gl.c)
  target_include_directories(${PROJECT_NAME} PUBLIC vendor/glad/include)
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
  target_compile_definitions(${PROJECT_NAME} PUBLIC DRZ_BACKEND_GL)
endif()

add_subdirectory(demo/hello)
add_subdirectory(demo/matrix-rain)
