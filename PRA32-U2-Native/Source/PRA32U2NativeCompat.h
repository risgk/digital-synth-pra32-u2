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
