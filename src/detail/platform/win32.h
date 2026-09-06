#pragma once

#if defined(_WIN32)
#if defined(__MINGW32__) && defined(__GNUC__) && !defined(__clang__)
#define _X86INTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED
#define _MM_MALLOC_H_INCLUDED
#endif

#include <windows.h>

#if defined(__MINGW32__) && defined(__GNUC__) && !defined(__clang__)
#undef _X86INTRIN_H_INCLUDED
#undef _EMMINTRIN_H_INCLUDED
#undef _MM_MALLOC_H_INCLUDED
#endif
#endif
