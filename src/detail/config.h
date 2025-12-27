#pragma once

#include <cstddef>

#if defined(_WIN32) || defined(__CYGWIN__)
#ifdef JT_DLL_EXPORT
#define JT_API __declspec(dllexport)
#elif defined(JT_DLL_IMPORT)
#define JT_API __declspec(dllimport)
#else
#define JT_API
#endif
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER) || \
    defined(__EMSCRIPTEN__)
#ifdef JT_LIB_VISIBILITY
#define JT_API __attribute__((visibility("default")))
#else
#define JT_API
#endif
#else
#define JT_API
#endif
