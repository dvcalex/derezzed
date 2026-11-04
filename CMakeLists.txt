cmake_minimum_required(VERSION 3.26)
project(drz VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DRZ_ENABLE_LOGGING "Keep DRZ logging enabled in Release builds" OFF)

# Make compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Executables go to build/bin
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
    src/core/Camera.cpp
    src/gfx/gl/Shader.cpp
    src/gfx/gl/Renderer.cpp
    src/gfx/gl/FrameRingBuffer.cpp
    src/gfx/gl/MeshPool.cpp
    src/gfx/gl/IndirectRingBuffer.cpp
    src/gfx/gl/Texture.cpp
    src/gfx/gl/ModelLoader.cpp
    src/util/Logger.cpp

    vendor/glad/src/gl.c
    vendor/tinygltf/tinygltf.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
    vendor/glad/include
)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    vendor
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    $<$<BOOL:${DRZ_ENABLE_LOGGING}>:DRZ_ENABLE_LOGGING>
    DRZ_BACKEND_GL
    TINYGLTF_NO_STB_IMAGE
    TINYGLTF_NO_STB_IMAGE_WRITE
    TINYGLTF_NO_EXTERNAL_IMAGE
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC  SDL3::SDL3-static
    PRIVATE stb::image
)

function(drz_add_demo name)
  set(dir "${CMAKE_CURRENT_SOURCE_DIR}/demo/${name}")

  add_executable(${name} "${dir}/main.cpp")
  target_link_libraries(${name} PRIVATE ${PROJECT_NAME})

  # Generator expression prevents Visual Studio from appending Debug/Release
  set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${name}/$<0:>"
    )

  set(res_dirs)
  foreach(res_dir IN ITEMS "${dir}/res" "${CMAKE_CURRENT_SOURCE_DIR}/res")
    if(IS_DIRECTORY "${res_dir}")
      list(APPEND res_dirs "${res_dir}")
    endif()
  endforeach()

  if(res_dirs)
    # Run every build
    add_custom_target(${name}_resources
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                    ${res_dirs}
                    "$<TARGET_FILE_DIR:${name}>/res"
            COMMENT "Updating resources for ${name}"
            VERBATIM
        )
    add_dependencies(${name} ${name}_resources)
  endif()
endfunction()


# Demos
foreach(demo Hello MatrixRain Stress Scene3D Pumpkin ManyCubes Lighting)
  drz_add_demo(${demo})
endforeach()
