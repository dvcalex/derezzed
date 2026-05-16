cmake_minimum_required(VERSION 3.21)
project(drz VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Prefer a system-installed SDL3; fall back to fetching from source.
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
  message(STATUS "System SDL3 not found... fetching from source.")
  include(FetchContent)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON  CACHE BOOL "" FORCE)
  set(SDL_TEST   OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.2.0
        GIT_SHALLOW    TRUE
    )
  FetchContent_MakeAvailable(SDL3)
else()
  message(STATUS "Using system SDL3 (${SDL3_VERSION}).")
endif()

add_library(${PROJECT_NAME} STATIC
    src/core/engine.cpp
    src/util/logger.cpp
    src/gfx/gl/shader.cpp
    src/gfx/gl/renderer.cpp
    src/gfx/gl/frame_ring_buffer.cpp
    src/gfx/gl/mesh_pool.cpp
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

# Copies <caller_dir>/res next to the built target. Re-runs whenever any file under
# res/ changes; CONFIGURE_DEPENDS makes CMake re-glob to catch added/removed files.
function(drz_copy_res target)
  set(res_dir "${CMAKE_CURRENT_SOURCE_DIR}/res")
  if(NOT IS_DIRECTORY ${res_dir})
    return()
  endif()

  file(GLOB_RECURSE res_files CONFIGURE_DEPENDS "${res_dir}/*")
  set(stamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_res.stamp")

  add_custom_command(
        OUTPUT ${stamp}
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${res_dir} "$<TARGET_FILE_DIR:${target}>/res"
        COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
        DEPENDS ${res_files}
        COMMENT "Copying ${target} res/"
        VERBATIM
    )

  add_custom_target(${target}_res DEPENDS ${stamp})
  add_dependencies(${target} ${target}_res)
endfunction()

add_subdirectory(demo/hello)
add_subdirectory(demo/matrix-rain)
add_subdirectory(demo/stress)
