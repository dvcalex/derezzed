#pragma once

#include <cstdlib>

#if defined(_MSC_VER)

#include <intrin.h>

#define DEBUG_BREAK() __debugbreak()

#elif defined(__clang__) || defined(__GNUC__)

#define DEBUG_BREAK() __builtin_trap()

#else

// Portable fallback: terminate the program.
#define DEBUG_BREAK() std::abort()

#endif