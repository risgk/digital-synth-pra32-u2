#pragma once

#include "pra32-u2-common.h"

class PRA32_U2_Amp {
  static const int32_t SMOOTH_RATE        = 2048;

  int16_t m_gain;
  int16_t m_expression;
  int32_t m_gain_mod_input;
  uint8_t m_breath_mod;
  uint8_t m_breath_controller;
  int32_t m_total_gain_linear_current;
  int32_t m_output_gain_current;

public:
PRA32_U2_Amp()
  : m_gain(127)
  , m_expression(127)
  , m_gain_mod_input(0)
  , m_breath_mod()
  , m_breath_controller()
  , m_total_gain_linear_current()
  , m_output_gain_current()
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
  }

  INLINE void process_at_low_rate(int16_t gain_mod_input) {
    update_total_gain_current();
    m_gain_mod_input = gain_mod_input << 2;
    m_output_gain_current = multiply_shift_right(m_gain_mod_input, m_total_gain_linear_current, 16);
  }

  INLINE int32_t process(int32_t audio_input_int24) {
    return multiply_shift_right(audio_input_int24, m_output_gain_current, 16);
  }

private:
  INLINE int32_t calc_gain_linear_target() {
    return ((((m_gain * m_gain) * 16384) / 16129) *
            (((m_expression * m_expression) * 16384) / 16129)) >> (14 - 2);
  }

  INLINE int32_t calc_breath_gain_linear_target() {
    const int32_t val_mod_2 = (m_breath_controller * 16384) / 127;
    const int32_t val_mod_1 = ((m_breath_controller * m_breath_controller) * 16384) / 16129;
    const int32_t val_mod_0 = 16384;

    return ((val_mod_2 * (m_breath_mod == 2)) +
            (val_mod_1 * (m_breath_mod == 1)) +
            (val_mod_0 * (m_breath_mod == 0))) << 2;
  }

  // Combine gain/expression and breath into a single smoothed multiplier.
  INLINE void update_total_gain_current() {
    int32_t total_gain_linear_target = multiply_shift_right(calc_gain_linear_target(), calc_breath_gain_linear_target(), 16);
    m_total_gain_linear_current = approach_exp(m_total_gain_linear_current, total_gain_linear_target, SMOOTH_RATE);
  }
};
