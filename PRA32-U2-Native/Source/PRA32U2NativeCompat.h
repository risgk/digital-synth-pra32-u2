#pragma once

// Compatibility definitions for building the PRA32-U2 core (Digital-Synth-PRA32-U2/*.h)
// on PCs (MSVC, GCC, Clang), without modifying the core.
// Include this only in PRA32U2Engine.cpp, before the core headers
// ("boolean" may conflict with the Windows headers, which JUCE may include)

#include <cstdint>
#include <cstdio>

typedef signed char boolean;

// RP2040: places the function in RAM; not needed on PCs
#define __not_in_flash_func(func) (func)

// MSVC does not support "__attribute__((...))" (always_inline, noclone), so it is removed
#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(x)
#endif  // defined(_MSC_VER) && !defined(__clang__)

// MSVC does not have "__builtin_clz" (the Output Compressor), so it is defined with
// "_BitScanReverse"; the result is undefined for 0, as with GCC (never 0 in the core)
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>

static inline int __builtin_clz(unsigned int x) {
  unsigned long index;
  _BitScanReverse(&index, x);
  return 31 - static_cast<int>(index);
}
#endif  // defined(_MSC_VER) && !defined(__clang__)

// MSVC does not have the float builtins of the Distortion FX either ("__builtin_fabsf",
// "__builtin_fminf", "__builtin_fmaxf", and "__builtin_copysignf"), so they are defined with <cmath>
#if defined(_MSC_VER) && !defined(__clang__)
#include <cmath>

static inline float __builtin_fabsf(float x) { return std::fabs(x); }
static inline float __builtin_fminf(float x, float y) { return std::fmin(x, y); }
static inline float __builtin_fmaxf(float x, float y) { return std::fmax(x, y); }
static inline float __builtin_copysignf(float x, float y) { return std::copysign(x, y); }
#endif  // defined(_MSC_VER) && !defined(__clang__)
