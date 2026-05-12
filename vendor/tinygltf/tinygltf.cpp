// tinygltf implementation unit — compiled separately to avoid duplicate definitions.
// We define TINYGLTF_IMPLEMENTATION here, plus stubs for the built-in image
// loader/writer so tinygltf doesn't try to compile its own stb copies (we
// already vendor stb_image separately in the engine).
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"
