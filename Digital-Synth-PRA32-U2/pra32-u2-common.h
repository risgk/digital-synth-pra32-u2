#pragma once

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
  int32_t mask = -static_cast<int32_t>(value_0 > value_1);
  int32_t result = value_0 ^ ((value_0 ^ value_1) & mask);
  return result;
}

static INLINE int32_t maximum(int32_t value_0, int32_t value_1) {
  int32_t mask = -static_cast<int32_t>(value_0 < value_1);
  int32_t result = value_0 ^ ((value_0 ^ value_1) & mask);
  return result;
}

static INLINE int32_t clamp(int32_t value, int32_t minimum_value, int32_t maximum_value) {
  int32_t max_mask = -static_cast<int32_t>(value > maximum_value);
  int32_t clamped_max = value ^ ((value ^ maximum_value) & max_mask);
  int32_t min_mask = -static_cast<int32_t>(clamped_max < minimum_value);
  int32_t result = clamped_max ^ ((clamped_max ^ minimum_value) & min_mask);
  return result;
}

template <typename T>
T branchless_conditional(bool condition, T a, T b) {
  return condition * a + (!condition) * b;
}
