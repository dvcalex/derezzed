cmake_minimum_required(VERSION 3.21)
project(Derezzed VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(SDL3 REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_program(GLAD_EXECUTABLE glad REQUIRED)

set(GLAD_OUT_DIR "${CMAKE_BINARY_DIR}/glad")

execute_process(
    COMMAND ${GLAD_EXECUTABLE}
        --api gl:core=4.6
        --out-path ${GLAD_OUT_DIR}
        --reproducible
        --quiet
        c
    RESULT_VARIABLE GLAD_RESULT
)
if(NOT GLAD_RESULT EQUAL 0)
    message(FATAL_ERROR "glad generation failed")
endif()

add_library(${PROJECT_NAME} STATIC
    ${GLAD_OUT_DIR}/src/gl.c
    src/engine.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC  include
    PUBLIC  ${GLAD_OUT_DIR}/include
    SYSTEM PUBLIC vendor
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE SDL3::SDL3
    PRIVATE OpenGL::GL
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

add_subdirectory(demo/hello)
