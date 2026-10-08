cmake_minimum_required(VERSION 3.21)
project(drz VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Make compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# executables go to build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Vendoring all deps
set(SDL_SHARED  OFF CACHE BOOL "" FORCE)
set(SDL_STATIC  ON  CACHE BOOL "" FORCE)
set(SDL_TEST    OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendor/stb_image EXCLUDE_FROM_ALL)

# Build engine as a static library
add_library(${PROJECT_NAME} STATIC
    src/core/Engine.cpp
    src/util/Logger.cpp
    src/gfx/gl/Shader.cpp
    src/gfx/gl/Renderer.cpp
    src/gfx/gl/FrameRingBuffer.cpp
    src/gfx/gl/MeshPool.cpp
    src/gfx/gl/IndirectRingBuffer.cpp
    src/gfx/gl/Texture.cpp

    vendor/glad/src/gl.c
) 

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    vendor/glad/include
)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    vendor
)

target_compile_definitions(${PROJECT_NAME} PUBLIC DRZ_BACKEND_GL)

target_link_libraries(${PROJECT_NAME}
    PUBLIC  SDL3::SDL3-static
    PRIVATE stb::image
)

function(drz_add_demo name)
    set(dir "${CMAKE_CURRENT_SOURCE_DIR}/demo/${name}")
    set(res_dir "${dir}/res")

    add_executable(${name} "${dir}/main.cpp")
    target_link_libraries(${name} PRIVATE ${PROJECT_NAME})

    # Generator expression prevents Visual Studio from appending Debug/Release.
    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}/$<0:>"
    )

    if(IS_DIRECTORY "${res_dir}")
        add_custom_command(
            TARGET ${name}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf
                    "$<TARGET_FILE_DIR:${name}>/res"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${res_dir}"
                    "$<TARGET_FILE_DIR:${name}>/res"
            COMMENT "Copying resources for ${name}"
            VERBATIM
        )
    endif()
endfunction()


# Demos
foreach(demo Hello MatrixRain Stress)
  drz_add_demo(${demo})
endforeach()
