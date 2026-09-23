#pragma once

#include "pra32-u2-common.h"
#include "pra32-u2-lfo-table.h"
#include "pra32-u2-osc-table.h"

class PRA32_U2_LFO {
  static const uint8_t LFO_WAVEFORM_TRIANGLE  = 0;
  static const uint8_t LFO_WAVEFORM_SINE      = 1;
  static const uint8_t LFO_WAVEFORM_SAW_DOWN  = 2;
  static const uint8_t LFO_WAVEFORM_RANDOM    = 3;
  static const uint8_t LFO_WAVEFORM_SQUARE    = 4;
  static const uint8_t LFO_WAVEFORM_RED_NOISE = 5;

  static const uint8_t LFO_FADE_COEF_OFF      = 1;

  static const uint8_t LFO_FADE_LEVEL_MAX     = 128;

  static const int32_t SMOOTH_RATE            = 2048;

  uint32_t m_lfo_phase;
  int32_t  m_lfo_wave_level;
  uint32_t m_lfo_rate;
  uint8_t  m_lfo_depth_target[2];
  uint8_t  m_lfo_depth_result_current[4];
  uint8_t  m_lfo_waveform;
  uint16_t m_lfo_fade_coef;
  uint16_t m_lfo_fade_cnt;
  uint8_t  m_lfo_fade_level;
  int32_t  m_noise_int23;
  int32_t  m_sampled_noise_int23;
  int32_t  m_red_noise_acc;
  int32_t  m_red_noise_level;
  uint16_t m_red_noise_gain;
  int32_t  m_red_noise_coef;
  uint8_t  m_pressure_amt;
  uint8_t  m_pressure[4];

public:
  PRA32_U2_LFO()
  : m_lfo_phase()
  , m_lfo_wave_level()
  , m_lfo_rate()
  , m_lfo_depth_target()
  , m_lfo_depth_result_current()
  , m_lfo_waveform()
  , m_lfo_fade_coef()
  , m_lfo_fade_cnt()
  , m_lfo_fade_level()
  , m_noise_int23()
  , m_sampled_noise_int23()
  , m_red_noise_acc()
  , m_red_noise_level()
  , m_red_noise_gain()
  , m_red_noise_coef()
  , m_pressure_amt()
  , m_pressure()
  {
    m_lfo_waveform = LFO_WAVEFORM_TRIANGLE;
    m_lfo_fade_coef = LFO_FADE_COEF_OFF;
    m_lfo_fade_cnt = m_lfo_fade_coef;
    m_lfo_fade_level = LFO_FADE_LEVEL_MAX;
    m_noise_int23 = 0;
    m_sampled_noise_int23 = m_noise_int23;
  }

  INLINE void set_lfo_waveform(uint8_t controller_value) {
    static uint8_t lfo_waveform_table[6] = {
      LFO_WAVEFORM_TRIANGLE,
      LFO_WAVEFORM_SINE,
      LFO_WAVEFORM_RED_NOISE,
      LFO_WAVEFORM_SAW_DOWN,
      LFO_WAVEFORM_RANDOM,
      LFO_WAVEFORM_SQUARE,
    };

    int32_t index = ((controller_value * 10) + 128) >> 8;

    m_lfo_waveform = lfo_waveform_table[index];
  }

  template <uint32_t LFO_RATE_OFFSET = 0>
  INLINE void set_lfo_rate(uint8_t controller_value) {
    m_lfo_rate = g_lfo_rate_table[controller_value];
    m_lfo_rate += (m_lfo_rate > 0) * LFO_RATE_OFFSET;

    m_red_noise_coef = g_lfo_red_noise_coef_table[controller_value];
    m_red_noise_gain = g_lfo_red_noise_gain_table[controller_value];
  }

  template <uint8_t N>
  INLINE void set_lfo_depth(uint8_t controller_value) {
    m_lfo_depth_target[N] = (controller_value == 127) ? 128 : controller_value;
  }

  INLINE void set_lfo_fade_time(uint8_t controller_value) {
    m_lfo_fade_coef = g_lfo_fade_coef_table[controller_value];
  }

  INLINE void set_pressure_amt(uint8_t controller_value) {
    m_pressure_amt = ((controller_value + 1) >> 1) << 1;
  }

  template <uint8_t N>
  INLINE void set_pressure(uint8_t pressure) {
    m_pressure[N] = (pressure == 127) ? 128 : pressure;
  }

  INLINE void trigger_lfo() {
    const int32_t is_matched = (m_lfo_waveform == LFO_WAVEFORM_SAW_DOWN) | 
                               (m_lfo_waveform == LFO_WAVEFORM_RANDOM)   | 
                               (m_lfo_waveform == LFO_WAVEFORM_SQUARE);
    const uint32_t match_mask = -is_matched; 
    m_lfo_phase &= ~match_mask;
    m_sampled_noise_int23 ^= (m_sampled_noise_int23 ^ m_noise_int23) & match_mask;

    m_lfo_fade_level *= (m_lfo_fade_coef <= LFO_FADE_COEF_OFF);
  }

  template <uint8_t N>
  INLINE int32_t get_output() {
    int32_t lfo_level = (m_lfo_depth_result_current[N] * m_lfo_wave_level) >> 7;
    return lfo_level;
  }

  INLINE void reset() {
    m_lfo_phase = 0x00000000;
  }

  INLINE void process_at_low_rate(uint8_t count, int32_t noise_int23) {
    static_cast<void>(count);

    update_lfo_depth_current<0>();
    update_lfo_depth_current<1>();
    update_lfo_depth_current<2>();
    update_lfo_depth_current<3>();

    m_noise_int23 = noise_int23;
    update_red_noise_level(noise_int23);
    update_lfo_wave_level();
  }

private:
  template <uint8_t N>
  INLINE void update_lfo_depth_current() {
    int32_t lfo_depth_target = high_byte((m_lfo_depth_target[0] << 1) * m_lfo_fade_level) + m_lfo_depth_target[1]
                              + ((m_pressure_amt * m_pressure[N]) >> 7);
    lfo_depth_target = minimum(lfo_depth_target, 128);
    m_lfo_depth_result_current[N] = approach_exp(m_lfo_depth_result_current[N], lfo_depth_target, SMOOTH_RATE);
  }

  INLINE int32_t get_lfo_wave_level(uint32_t phase) {
    int32_t level = 0;

    switch (m_lfo_waveform) {
    case LFO_WAVEFORM_TRIANGLE:
      {
        level = (1 << 22) - std::abs(static_cast<int32_t>(phase) - (1 << 23));
      }
      break;
    case LFO_WAVEFORM_SINE:
      {
        uint32_t curr_index  = phase >> (24 - OSC_WAVE_TABLE_SAMPLES_BITS);
        int32_t  next_weight = phase & ((1 << (24 - OSC_WAVE_TABLE_SAMPLES_BITS)) - 1);
        // The table array points past the index-bits entry, but this one uses
        // the table directly, so it skips that entry itself
        const int16_t* sine_wave_table = g_osc_sine_wave_table_h1 + 1;
        int16_t  curr_data   = sine_wave_table[curr_index + 0];
        int16_t  next_data   = sine_wave_table[curr_index + 1];
        level                = (curr_data << 9) + ((((next_data - curr_data) * next_weight) + (1 << 5)) >> 6); // lerp
      }
      break;
    case LFO_WAVEFORM_SAW_DOWN:
      {
        level = (1 << 22) - static_cast<int32_t>((phase + 1) >> 1);
      }
      break;
    case LFO_WAVEFORM_RANDOM:
      {
        const int32_t mask = -(phase < m_lfo_rate);
        m_sampled_noise_int23 ^= (m_sampled_noise_int23 ^ m_noise_int23) & mask;
        level = m_sampled_noise_int23;
      }
      break;
    case LFO_WAVEFORM_SQUARE:
      level = (phase < 0x800000) << 23;
      break;
    case LFO_WAVEFORM_RED_NOISE:
      level = m_red_noise_level;
      break;
    }

    return level;
  }

  INLINE void update_red_noise_level(int32_t noise_int23) {
    int32_t red_noise_input = (((noise_int23 >> 8) * m_red_noise_gain) + (m_red_noise_gain >> 1)) >> 2;
    int32_t red_noise_delta = red_noise_input - m_red_noise_acc;
    m_red_noise_acc += static_cast<int32_t>(((static_cast<int64_t>(red_noise_delta) *
                                              m_red_noise_coef) + (1 << 30)) >> 31);

    int32_t red_noise_level = m_red_noise_acc << 3;
    red_noise_level -= (red_noise_level > +(1 << 22)) * (red_noise_level - (1 << 22));
    red_noise_level += (red_noise_level < -(1 << 22)) * (-(1 << 22) - red_noise_level);
    m_red_noise_level = red_noise_level;
  }

  INLINE void update_lfo_wave_level() {
    --m_lfo_fade_cnt;

    const int32_t is_zero = (m_lfo_fade_cnt == 0);
    m_lfo_fade_cnt = (m_lfo_fade_coef * is_zero) + (m_lfo_fade_cnt * (is_zero ^ 1));
    m_lfo_fade_level += (is_zero & (m_lfo_fade_level < LFO_FADE_LEVEL_MAX));

    m_lfo_phase += m_lfo_rate;
    m_lfo_phase &= 0x00FFFFFF;
    m_lfo_wave_level = get_lfo_wave_level(m_lfo_phase);
  }
};
