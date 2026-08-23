#pragma once

#include "pra32-u2-common.h"

class PRA32_U2_Amp {
  int16_t m_gain_target;
  int16_t m_gain_current;
  int16_t m_expression_target;
  int16_t m_expression_current;
  int32_t m_gain_linear;
  int32_t m_gain_mod_input;
  uint8_t m_breath_mod;
  uint8_t m_breath_controller;
  int32_t m_breath_gain_linear;

public:
PRA32_U2_Amp()
  : m_gain_target(127)
  , m_gain_current(0)
  , m_expression_target(127)
  , m_expression_current(0)
  , m_gain_linear()
  , m_gain_mod_input(0)
  , m_breath_mod()
  , m_breath_controller()
  , m_breath_gain_linear()
  {
  }

  INLINE void set_gain(uint8_t controller_value) {
    m_gain_target = controller_value;
  }

  INLINE void set_expression(uint8_t controller_value) {
    m_expression_target = controller_value;
  }

  INLINE void set_breath_mod(uint8_t controller_value) {
    m_breath_mod = (controller_value >= 32) + (controller_value >= 96);
  }

  INLINE void set_breath_controller(uint8_t controller_value) {
    m_breath_controller = controller_value;
  }

  INLINE void reset() {
    m_gain_mod_input = 0;
  }

  INLINE void process_at_low_rate(int16_t gain_mod_input) {
    update_gain_current();
    m_gain_mod_input = gain_mod_input << 2;
    update_breath_controller_effective();
  }

  INLINE int32_t process(int32_t audio_input_int24) {
    int32_t audio_output = audio_input_int24;
    audio_output = multiply_shift_right(audio_output, m_gain_mod_input,     16);
    audio_output = multiply_shift_right(audio_output, m_gain_linear,        16);
    audio_output = multiply_shift_right(audio_output, m_breath_gain_linear, 16);
    return audio_output;
  }

private:
  INLINE void update_gain_current() {
    m_gain_current = approach_exp(m_gain_current, m_gain_target, 2048);
    m_expression_current = approach_exp(m_expression_current, m_expression_target, 2048);
    m_gain_linear = ((((m_gain_current * m_gain_current) * 16384) / 16129) *
                     (((m_expression_current * m_expression_current) * 16384) / 16129)) >> (14 - 2);
  }

  INLINE void update_breath_controller_effective() {
    const int32_t val_mod_2 = (m_breath_controller * 16384) / 127;
    const int32_t val_mod_1 = ((m_breath_controller * m_breath_controller) * 16384) / 16129;
    const int32_t val_mod_0 = 16384;

    m_breath_gain_linear = ((val_mod_2 * (m_breath_mod == 2)) + 
                            (val_mod_1 * (m_breath_mod == 1)) + 
                            (val_mod_0 * (m_breath_mod == 0))) << 2;
  }
};
