#pragma once

#include "pra32-u2-common.h"

class PRA32_U2_Amp {
  // The gain is recalculated once per control interval, and interpolated over
  // the samples in between; holding it would step the audio at the control
  // rate, which shows up as sidebands around every partial
  static const int32_t CONTROL_INTERVAL_BITS = 2;

  int16_t m_gain;
  int16_t m_expression;
  int32_t m_gain_mod_input;
  uint8_t m_breath_mod;
  uint8_t m_breath_controller;
  int32_t m_gain_linear_stage_1;
  int32_t m_gain_linear_current;        // Q24 (the gain in Q16, shifted left by 8)
  int32_t m_expression_breath_linear_current;  // Q24
  int32_t m_total_gain_linear_current;  // Q24 (the gain in Q16, shifted left by 8)
  int32_t m_output_gain_current;
  int32_t m_output_gain_next;
  int32_t m_output_gain_step;

public:
PRA32_U2_Amp()
  : m_gain(127)
  , m_expression(127)
  , m_gain_mod_input(0)
  , m_breath_mod()
  , m_breath_controller()
  , m_gain_linear_stage_1()
  , m_gain_linear_current()
  , m_expression_breath_linear_current()
  , m_total_gain_linear_current()
  , m_output_gain_current()
  , m_output_gain_next()
  , m_output_gain_step()
  {
  }

  INLINE void set_gain(uint8_t controller_value) {
    m_gain = controller_value;
  }

  INLINE void set_expression(uint8_t controller_value) {
    m_expression = controller_value;
  }

  INLINE void set_breath_mod(uint8_t controller_value) {
    m_breath_mod = (controller_value >= 32) + (controller_value >= 96);
  }

  INLINE void set_breath_controller(uint8_t controller_value) {
    m_breath_controller = controller_value;
  }

  INLINE void reset() {
    m_gain_mod_input = 0;
    m_output_gain_current = 0;
    m_output_gain_next = 0;
    m_output_gain_step = 0;
  }

  // The gain, the expression, and the breath are the same for all voices, so
  // they are smoothed by one Amp (m_amp[0]) only, which every Amp reads in
  // process_at_low_rate()
  INLINE void update_smoothing() {
    update_total_gain_current();
  }

  INLINE void process_at_low_rate(int32_t gain_mod_input, const PRA32_U2_Amp& smoothed) {
    m_gain_mod_input = gain_mod_input;
    m_output_gain_next = multiply_shift_right(m_gain_mod_input, smoothed.m_total_gain_linear_current >> 8, 16);

    // Rounded up, so that the target is reached by the end of the interval
    const int32_t delta = m_output_gain_next - m_output_gain_current;
    m_output_gain_step = (maximum(delta, -delta) +
                          ((1 << CONTROL_INTERVAL_BITS) - 1)) >> CONTROL_INTERVAL_BITS;
  }

  INLINE int32_t process(int32_t audio_input_int24) {
    m_output_gain_current = approach(m_output_gain_current, m_output_gain_next, m_output_gain_step);
    return multiply_shift_right(audio_input_int24, m_output_gain_current, 23);
  }

private:
  // Q24
  INLINE int32_t calc_gain_linear_target() {
    return (((m_gain * m_gain) * 16384) / 16129) << (2 + 8);
  }

  // Q24
  INLINE int32_t calc_expression_breath_linear_target() {
    return ((((m_expression * m_expression) * 16384) / 16129) * calc_breath_gain_linear_target()) >> (14 - 8);
  }

  INLINE int32_t calc_breath_gain_linear_target() {
    const int32_t val_mod_2 = (m_breath_controller * 16384) / 127;
    const int32_t val_mod_1 = ((m_breath_controller * m_breath_controller) * 16384) / 16129;
    const int32_t val_mod_0 = 16384;

    return ((val_mod_2 * (m_breath_mod == 2)) +
            (val_mod_1 * (m_breath_mod == 1)) +
            (val_mod_0 * (m_breath_mod == 0))) << 2;
  }

  // Combine the smoothed gain and the smoothed expression/breath into a single
  // multiplier, in Q24 so that the tail is not held to 1 step of Q16.
  // The Expression and the Breath Controller are smoothed fast (see
  // approach_exp_fast()), so that the attack of a wind controller is not softened
  INLINE void update_total_gain_current() {
    approach_exp_slow(m_gain_linear_stage_1, m_gain_linear_current, calc_gain_linear_target());
    m_expression_breath_linear_current = approach_exp_fast(m_expression_breath_linear_current, calc_expression_breath_linear_target());
    m_total_gain_linear_current = multiply_shift_right(m_gain_linear_current, m_expression_breath_linear_current, 24);
  }
};
