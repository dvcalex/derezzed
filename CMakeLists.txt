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

# Build engine as a static library
add_library(${PROJECT_NAME} STATIC
    src/core/Engine.cpp
    src/util/Logger.cpp
    src/gfx/gl/Shader.cpp
    src/gfx/gl/Renderer.cpp
    src/gfx/gl/FrameRingBuffer.cpp
    src/gfx/gl/MeshPool.cpp
    src/gfx/gl/IndirectRingBuffer.cpp

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

target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3-static)

function(drz_add_demo name)
  set(dir "${CMAKE_CURRENT_SOURCE_DIR}/demo/${name}")

  add_executable(${name} "${dir}/main.cpp")
  # Linking the engine pulls in SDL3 transitively
  target_link_libraries(${name} PRIVATE ${PROJECT_NAME})

  # Private output dir keeps each demo's res/ from clobbering others
  set(out_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}")
  set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${out_dir}")

  set(res_dir "${dir}/res")
  if(NOT IS_DIRECTORY "${res_dir}")
    return()
  endif()

  # Symlink res/ next to the binary so SDL_GetBasePath() finds it. Falls back to a copy where symlinks error.
  file(MAKE_DIRECTORY "${out_dir}")
  if(NOT EXISTS "${out_dir}/res")
    file(CREATE_LINK "${res_dir}" "${out_dir}/res" SYMBOLIC COPY_ON_ERROR)
  endif()
endfunction()

# Demos
foreach(demo Hello MatrixRain Stress)
  drz_add_demo(${demo})
endforeach()
