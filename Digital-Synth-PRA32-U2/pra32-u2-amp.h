#pragma once

#include "pra32-u2-common.h"

class PRA32_U2_Amp {
  int16_t m_gain_control;
  int16_t m_gain_control_effective;
  int16_t m_gain_linear;
  int16_t m_gain_mod_input;
  uint8_t m_breath_mod;
  uint8_t m_breath_controller;
  int16_t m_breath_gain_linear;

public:
PRA32_U2_Amp()
  : m_gain_control(127)
  , m_gain_control_effective(0)
  , m_gain_linear()
  , m_gain_mod_input(0)
  , m_breath_mod()
  , m_breath_controller()
  , m_breath_gain_linear()
  {
  }

  INLINE void set_gain(uint8_t controller_value) {
    m_gain_control = controller_value;
  }

  INLINE void set_breath_mod(uint8_t controller_value) {
    if (controller_value >= 96) {
      m_breath_mod = 2;
    } else if (controller_value >= 32) {
      m_breath_mod = 1;
    } else {
      m_breath_mod = 0;
    }
  }

  INLINE void set_breath_controller(uint8_t controller_value) {
    m_breath_controller = controller_value;
  }

  INLINE void process_at_low_rate(int16_t gain_mod_input) {
    update_gain_effective();
    m_gain_mod_input = gain_mod_input;
    update_breath_controller_effective();
  }

  INLINE int32_t process(int32_t audio_input_int24) {
    int32_t audio_output = audio_input_int24;
    audio_output = mul_s32_s32_h16(audio_output, m_gain_mod_input     << 2);
    audio_output = mul_s32_s32_h16(audio_output, m_gain_linear        << 2);
    audio_output = mul_s32_s32_h16(audio_output, m_breath_gain_linear << 2);
    audio_output = mul_s32_s32_h16(audio_output, 11585                << 2);
    return audio_output;
  }

private:
  INLINE void update_gain_effective() {
    m_gain_control_effective += (m_gain_control_effective < m_gain_control);
    m_gain_control_effective -= (m_gain_control_effective > m_gain_control);

    m_gain_linear = ((m_gain_control_effective * m_gain_control_effective) * 16384) / 16129;
  }

  INLINE void update_breath_controller_effective() {
    if (m_breath_mod == 2) {
      m_breath_gain_linear = (m_breath_controller * 16384) / 127;
    } else if (m_breath_mod == 1) {
      m_breath_gain_linear = ((m_breath_controller * m_breath_controller) * 16384) / 16129;
    } else {
      m_breath_gain_linear = 16384;
    }
  }
};
