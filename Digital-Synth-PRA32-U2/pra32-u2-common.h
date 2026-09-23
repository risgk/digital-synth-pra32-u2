#pragma once

#include <algorithm>
#include "pra32-u2-constants.h"

#define INLINE inline __attribute__((always_inline))

struct PRA32_U2_StereoSample {
  int32_t left;
  int32_t right;
};

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

// Soft clipping for the final output (1.0 = 1 << 23): linear up to +-0.5, then
// a quadratic knee that reaches +-1.0 with slope 0 at +-1.5
//   y = a - (a - 0.5)^2 / 2  (a = |x|, 0.5 <= a <= 1.5)
// The result always fits in 24 bits
static INLINE int32_t soft_clip_output(int32_t value) {
  const int32_t ONE = 1 << 23;
  int32_t abs_value = minimum((value < 0) ? -value : value, ONE + (ONE >> 1));
  int32_t over      = maximum(abs_value - (ONE >> 1), 0);
  int32_t result    = minimum(abs_value - multiply_shift_right(over, over, 24), ONE - 1);
  return (value < 0) ? -result : result;
}

static INLINE int32_t approach(int32_t current_value, int32_t target_value, int32_t delta) {
  return std::clamp(target_value, current_value - delta, current_value + delta);
}

static INLINE int32_t approach_exp(int32_t current_value, int32_t target_value, int32_t rate) {
  return target_value - (((target_value - current_value) * (65536 - rate)) / 65536);
}

// Same result as approach_exp, for value ranges where the 32-bit product would overflow
static INLINE int32_t approach_exp_wide(int32_t current_value, int32_t target_value, int32_t rate) {
  int64_t delta = static_cast<int64_t>(target_value - current_value) * (65536 - rate);
  return target_value - static_cast<int32_t>((delta + ((delta >> 63) & 0xFFFF)) >> 16);
}

template <typename T>
T branchless_conditional(bool condition, T a, T b) {
  return (condition ? a : b);
}
