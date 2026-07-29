#pragma once

#include <algorithm>
#include "pra32-u2-constants.h"

#define INLINE inline __attribute__((always_inline))

static INLINE uint8_t low_byte(uint16_t x) {
  return x & 0xFF;
}

static INLINE uint8_t high_byte(uint16_t x) {
  return (x >> 8) & 0xFF;
}

static INLINE int32_t multiply_shift_right(int32_t x, int32_t y, uint8_t z) {
  return static_cast<int32_t>((static_cast<int64_t>(x) * y) >> z);
}

static INLINE int32_t minimum(int32_t value_0, int32_t value_1) {
  return std::min(value_0, value_1);
}

static INLINE int32_t maximum(int32_t value_0, int32_t value_1) {
  return std::max(value_0, value_1);
}

static INLINE int32_t clamp(int32_t value, int32_t minimum_value, int32_t maximum_value) {
  return std::clamp(value, minimum_value, maximum_value);
}

static INLINE int32_t approach(int32_t current_value, int32_t target_value, int32_t delta) {
  return std::clamp(target_value, current_value - delta, current_value + delta);
}

template <typename T>
T branchless_conditional(bool condition, T a, T b) {
  return (condition ? a : b);
}
