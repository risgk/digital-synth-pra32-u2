#pragma once

#include "pra32-u2-constants.h"

#define INLINE inline __attribute__((always_inline))

static INLINE uint8_t low_byte(uint16_t x) {
  return x & 0xFF;
}

static INLINE uint8_t high_byte(uint16_t x) {
  return (x >> 8) & 0xFF;
}

static INLINE int32_t mul_s32_s32_shift_right(int32_t x, int32_t y, uint8_t z) {
  return static_cast<int32_t>((static_cast<int64_t>(x) * y) >> z);
}

static INLINE int32_t clamp(int32_t value, int32_t minimum, int32_t maximum) {
    volatile int32_t clamped =
         (value >  maximum)                        * maximum
      +                        (value <  minimum)  * minimum
      + ((value <= maximum) && (value >= minimum)) * value;
    return clamped;
}
