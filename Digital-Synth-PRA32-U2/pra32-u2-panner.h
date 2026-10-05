#pragma once

#include <cmath>
#include "pra32-u2-common.h"

#ifndef PI
#define PI (3.1415926535897932384626433832795)
#endif

class PRA32_U2_Panner {
  static const uint8_t OSC_PAN_TABLE_LENGTH = 129;

  int32_t m_pan_table[OSC_PAN_TABLE_LENGTH];
  int16_t m_pan_target;
  int32_t m_pan_stage_1;
  int32_t m_pan_current;   // Q16
  int32_t m_gain_linear_l;
  int32_t m_gain_linear_r;

public:
PRA32_U2_Panner()
  : m_pan_table()
  , m_pan_target(64)
  , m_pan_stage_1(64 << 16)
  , m_pan_current(64 << 16)
  , m_gain_linear_l(16384 << 2)
  , m_gain_linear_r(16384 << 2)
  {
    for (uint8_t i = 1; i < OSC_PAN_TABLE_LENGTH - 1; ++i) {
      m_pan_table[i] = static_cast<int16_t>(std::sqrt(2.0) * std::sin((PI * (i - 1)) / (2 * (OSC_PAN_TABLE_LENGTH - 1))) * (1 << 14)) << 2;
    }

    m_pan_table[0]                        = m_pan_table[1];
    m_pan_table[OSC_PAN_TABLE_LENGTH - 1] = m_pan_table[OSC_PAN_TABLE_LENGTH - 2];
  }

  INLINE void set_pan(uint8_t controller_value) {
    m_pan_target = controller_value;
  }

  INLINE void process_at_low_rate(uint8_t count) {
    if (is_balance_smoothing_period(count)) {
      update_gain_current();
    }
  }

  INLINE PRA32_U2_StereoSample process(int32_t audio_input_int24) {
    PRA32_U2_StereoSample audio_output_int24;

    audio_output_int24.left  = multiply_shift_right(audio_input_int24, m_gain_linear_l, 16);
    audio_output_int24.right = multiply_shift_right(audio_input_int24, m_gain_linear_r, 16);

    return audio_output_int24;
  }

private:
  INLINE void update_gain_current() {
    // In Q16, with the table interpolated, so that the gains do not move in steps while smoothed
    approach_exp_slow(m_pan_stage_1, m_pan_current, static_cast<int32_t>(m_pan_target) << 16);
    m_gain_linear_l = interpolate_table_q16(m_pan_table, OSC_PAN_TABLE_LENGTH - 1, (128 << 16) - m_pan_current);
    m_gain_linear_r = interpolate_table_q16(m_pan_table, OSC_PAN_TABLE_LENGTH - 1, m_pan_current);
  }
};
