#pragma once

#include <algorithm>
#include "pra32-u2-constants.h"

#define INLINE inline __attribute__((always_inline))

struct PRA32_U2_StereoSample {
  int32_t left;
  int32_t right;
};

// The FX inputs, indexed by the FX Routing; each synth adds its output to one of them
const uint8_t FX_BUS_CHORUS = 0;
const uint8_t FX_BUS_DELAY  = 1;
const uint8_t FX_BUS_BYPASS = 2;

struct PRA32_U2_FxBusSample {
  PRA32_U2_StereoSample input[3];
};

// Not "= {}", which -Os compiles into a call to memset() in the flash (through a veneer)
static INLINE void clear_fx_bus(PRA32_U2_FxBusSample& fx_bus) {
  fx_bus.input[0].left  = 0;
  fx_bus.input[0].right = 0;
  fx_bus.input[1].left  = 0;
  fx_bus.input[1].right = 0;
  fx_bus.input[2].left  = 0;
  fx_bus.input[2].right = 0;
}

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

// Soft clipping for the final output (1.0 = 1 << 23): linear up to +-0.75, then
// a knee that reaches +-1.0 with slope 0 at +-1.25. The slope 1 - (3 t^2 - 2 t^3)
// falls smoothly at both ends of the knee (C2 continuous), so that the harmonics
// grow gradually as the input goes over 0.75
//   y = 0.75 + 0.5 * (t - t^3 + t^4 / 2)  (t = (|x| - 0.75) / 0.5, 0 <= t <= 1)
//     = |x| - t^3 * (2 - t) / 4
// The error is within about 1 LSB (near the ceiling, the output may step back
// by 1 LSB). The result always fits in 24 bits
static INLINE int32_t soft_clip_output(int32_t value) {
  const int32_t ONE = 1 << 23;
  int32_t abs_value   = minimum((value < 0) ? -value : value, ONE + (ONE >> 2));
  int32_t t           = maximum(abs_value - (ONE - (ONE >> 2)), 0) << 8;  // Q30
  int32_t t_2         = multiply_shift_right(t,   t, 32);                 // Q28
  int32_t t_3         = multiply_shift_right(t_2, t, 32);                 // Q26
  int32_t two_minus_t = (1 << 30) - (t >> 1);                             // Q29
  int32_t result      = minimum(abs_value - (multiply_shift_right(t_3, two_minus_t, 32) >> 2), ONE - 1);
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

// Parameter smoothing (see also "Parameter Smoothing" in README.md)
// The smoothing is updated at 6 kHz, i.e. every other control period of 12 kHz
// (once per 8 samples): the amounts in the even periods, and the balances in
// the odd periods, so that the load is spread (see is_balance_smoothing_period()).
// The EG and LFO modulations themselves are not smoothed, so they are not delayed.
// - Slow: approach_exp_slow(), 2 stages at the rate 4096 at 6 kHz (5.3 ms average delay, 99% in 18 ms),
//   for the amounts (of the tone, the level, or a modulation), whose steps are easily heard
//   - Filter: Cutoff, Resonance, Filter EG Amt, LFO Filter Amt, Breath Filter Amt,
//     EG/LFO Mod Amt (Dst: F)
//   - Osc: Osc 1 Shape, EG/LFO Mod Amt (Dst: 1S)
//   - Amp: Amp Gain
//   - LFO: LFO Depth (the sum with the LFO Fade, the Modulation, and the After Touch, per voice)
//   - Chorus FX: Chorus Level, Chorus Depth, the base delay time (internal)
//   - Delay FX: Delay Level, Delay Feedback (with the Reverb table interpolated)
// - Slow (in the odd periods): approach_exp_slow(), the same as above,
//   for the balances (between two sounds, or left and right), which keep the total amount about the same
//   - Osc: Osc 1 Morph (in Q16; rounded to the controller value for the Sine Wave and the
//     Wave Tables, which switch the ratio or the table step by step), Mixer Osc Mix (in Q16),
//     Mixer Noise/Sub Osc (in 1/16 steps)
//   - Panner: Pan (in Q16)
// - Fast: approach_exp_fast(), 1 stage at the rate 8192 at 6 kHz (1.2 ms time constant, 95% in 3.7 ms),
//   for the performance controllers, whose attack must not be softened, but whose steps must not click
//   - Filter: the Breath Controller (x Breath Filter Amt)
//   - Amp: Expression x Breath Controller (Breath Amp Mod)
// - Normal: 1 stage, for the Delay Time, which moves in its own way
//   - Delay FX: Delay Time (in Q8, at 6 kHz, i.e. 5.3 ms, and slew-limited)
// - Others
//   - The Amp gain and the Chorus delay time are interpolated linearly over the control interval
//   - The Delay Time and the Reverb size are also slew-limited (DELAY_TIME_SLEW, REVERB_LEN_SLEW)
//   - Not smoothed: the pitch parameters, so that the pitch follows right away (Pitch Bend,
//     EG/LFO Mod Amt (Dst: P, 2P)), and the parameters whose steps are part of the sound
//     (e.g. Osc 2 Coarse/Pitch, LFO Rate, EG times)

// The smoothing at 6 kHz takes half the load of the smoothing at 12 kHz, with the
// same time constants. count is the count of the control periods (12 kHz)
static INLINE bool is_slow_smoothing_period(uint8_t count) {
  return (count & 0x01) == 0;
}

static INLINE bool is_balance_smoothing_period(uint8_t count) {
  return (count & 0x01) != 0;
}

// Slower smoothing for the parameters whose steps are easily heard (e.g. the
// Filter Cutoff at a high Resonance, moved by a MIDI controller that sends
// sparse CCs), for a call at 6 kHz: two cascaded stages at the rate 4096
// (a time constant of 2.7 ms each) have the same average delay as one stage at
// the rate 2048 (5.3 ms), but start from slope 0, so that the corners of the
// steps of the target are rounded off, and settle sooner (99% in 18 ms).
// It is not slower than that, so that the Cutoff and the other parameters sent
// by the breath of some MIDI controllers (e.g. wind controllers) follow the breath.
// To use one stage at the rate 2048 instead, set SLOW_SMOOTH_TWO_STAGES to false
static const bool    SLOW_SMOOTH_TWO_STAGES = true;
static const uint8_t SLOW_SMOOTH_SHIFT      = SLOW_SMOOTH_TWO_STAGES ? 4 : 5;  // The rate 4096 or 2048

// Same result as approach_exp_wide() with the rate (65536 >> SHIFT), without
// the 64-bit product: the step is the difference divided by (1 << SHIFT),
// rounded away from zero
template <uint8_t SHIFT>
static INLINE int32_t approach_exp_shift(int32_t current_value, int32_t target_value) {
  int32_t delta = target_value - current_value;
  return current_value + ((delta + (((1 << SHIFT) - 1) & ~(delta >> 31))) >> SHIFT);
}

// 1 stage at the rate 8192 at 6 kHz (1.2 ms time constant, 95% in 3.7 ms),
// for the performance controllers (the Expression and the Breath Controller),
// whose attack must not be softened, but whose steps must not click (e.g. the
// 7-bit steps of the Breath Controller at a low breath, or in the Filter Cutoff
// at a high Resonance)
static INLINE int32_t approach_exp_fast(int32_t current_value, int32_t target_value) {
  return approach_exp_shift<3>(current_value, target_value);
}

static INLINE int32_t approach_exp_slow(int32_t& stage_1, int32_t& stage_2, int32_t target_value) {
  stage_1 = approach_exp_shift<SLOW_SMOOTH_SHIFT>(stage_1, target_value);
  if constexpr (SLOW_SMOOTH_TWO_STAGES) {
    stage_2 = approach_exp_shift<SLOW_SMOOTH_SHIFT>(stage_2, stage_1);
  } else {
    stage_2 = stage_1;
  }
  return stage_2;
}

// Linear interpolation of table[0 .. last_index] at index_q16 (0 to last_index << 16);
// the same as table[i] at an integer index i, and table[last_index + 1] is not read.
// The differences between adjacent entries times 65536 must fit in 31 bits
template <typename T>
static INLINE int32_t interpolate_table_q16(const T* table, int32_t last_index, int32_t index_q16) {
  int32_t index    = minimum(index_q16 >> 16, last_index - 1);
  int32_t fraction = index_q16 - (index << 16);  // 0 to 65536
  return table[index] + (((table[index + 1] - table[index]) * fraction) >> 16);
}

template <typename T>
T branchless_conditional(bool condition, T a, T b) {
  return (condition ? a : b);
}
