add_library(stb_image STATIC
    stb_image.cpp
    stb_image_write.cpp
)

# Consumers write #include <stb_image/stb_image.h>
# SYSTEM hides warnings from stb headers in your code
target_include_directories(stb_image SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_library(stb::image ALIAS stb_image)