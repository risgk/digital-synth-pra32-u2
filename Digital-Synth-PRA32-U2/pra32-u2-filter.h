#pragma once

// refs https://www.discodsp.net/VAFilterDesign_2.1.2.pdf (The Art of VA Filter Design)
// refs https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt
// refs https://jatinchowdhury18.medium.com/complex-nonlinearities-episode-4-nonlinear-biquad-filters-ae6b3f23cb0e
// refs https://www.willpirkle.com/fx-book/project-gallery/
// refs https://www.willpirkle.com/synthlabdm/

#include "pra32-u2-common.h"
#include "pra32-u2-filter-table.h"

static const uint8_t FILTER_CALC_SCALING_BITS = 3;
static const int32_t FILTER_ONE               = (1 << 23) << FILTER_CALC_SCALING_BITS;

// Cubic soft clipping, realized by "gain prediction": instead of evaluating a
// waveshaper on the signal, the gain of the clipper is predicted from the
// signal and applied by a single multiplication
//   soft_clip(x) = gain(x) * c,  c = clamp(x, -1, +1),  gain(x) = 1 - c^2 / 3
static INLINE int32_t soft_clip(int32_t value) {
  // Note: Without anti-aliasing (oversampling)

  int32_t sign_mask = -static_cast<int32_t>(value < 0);
  int32_t abs_value = (value ^ sign_mask) - sign_mask;
  int32_t clamped_abs = minimum(abs_value, FILTER_ONE);
  int32_t squared = multiply_shift_right(clamped_abs, clamped_abs,
                                         ((23 + FILTER_CALC_SCALING_BITS) * 2) - FILTER_TABLE_FRACTION_BITS);
  int32_t gain = (1 << FILTER_TABLE_FRACTION_BITS) - (squared / 3);
  int32_t clamped_positive = multiply_shift_right(clamped_abs, gain, FILTER_TABLE_FRACTION_BITS);
  int32_t clamped = (clamped_positive ^ sign_mask) - sign_mask;

  return clamped;
}

// Linear interpolation between the coefficients of two adjacent controller
// values; controller_value_q8 is the controller value scaled by 256
static INLINE int32_t interpolate_filter_table(const int32_t* filter_table, int32_t controller_value_q8) {
  uint32_t index = static_cast<uint32_t>(controller_value_q8) >> 8;
  int32_t  fraction = controller_value_q8 & 0xFF;
  int32_t  value_0 = filter_table[index + 0];
  int32_t  value_1 = filter_table[index + 1];

  return value_0 + multiply_shift_right(value_1 - value_0, fraction, 8);
}

class PRA32_U2_Filter {
  static const int32_t SMOOTH_RATE = 2048;
  static const int32_t CONTROLLER_VALUE_Q8_MAX = static_cast<int32_t>(FILTER_TABLE_LENGTH - 2) << 8;

  int32_t m_g;                         // g = tan(pi * f_0 / f_s), Q26
  int32_t m_one_over_a_0;              // 1 / a_0, Q30, where a_0 = 1 + g * (g + k)
  int32_t m_g_plus_k_over_a_0;         // (g + k) / a_0, Q30
  int32_t m_s_1;                       // State of the band pass integrator
  int32_t m_s_2;                       // State of the low pass integrator
  uint8_t m_resonance_target;
  int32_t m_resonance_current;
  int32_t m_cutoff_current;
  int32_t m_cutoff_target;
  int16_t m_cutoff_eg_amt_target[2];
  int32_t m_cutoff_eg_amt_current[2];
  int16_t m_cutoff_lfo_amt[2];
  int16_t m_cutoff_lfo_amt_current[2];
  int16_t m_cutoff_pitch_amt;
  uint8_t m_filter_mode;
  int16_t m_cutoff_breath_amt;
  int16_t m_breath_controller;
  int32_t m_cutoff_base_current;       // Smooth state variable for the base cutoff and LFO/Pitch/Breath modulations

public:
  PRA32_U2_Filter()
  : m_g()
  , m_one_over_a_0()
  , m_g_plus_k_over_a_0()
  , m_s_1()
  , m_s_2()
  , m_resonance_target()
  , m_resonance_current()
  , m_cutoff_current()
  , m_cutoff_target()
  , m_cutoff_eg_amt_target()
  , m_cutoff_eg_amt_current()
  , m_cutoff_lfo_amt()
  , m_cutoff_lfo_amt_current()
  , m_cutoff_pitch_amt()
  , m_filter_mode()
  , m_cutoff_breath_amt()
  , m_breath_controller()
  , m_cutoff_base_current()
  {
    set_cutoff(127);
    set_resonance(0);
    set_cutoff_eg_amt(0, 64);
    set_cutoff_eg_amt(1, 64);
    set_cutoff_lfo_amt(0, 64);
    set_cutoff_lfo_amt(1, 64);
    set_cutoff_pitch_amt(0);

    // Bootstrap initial smooth states to prevent sudden filter sweeps on power-up
    m_cutoff_base_current = m_cutoff_target;
    m_cutoff_eg_amt_current[0] = static_cast<int32_t>(m_cutoff_eg_amt_target[0]) << 16;
    m_cutoff_eg_amt_current[1] = static_cast<int32_t>(m_cutoff_eg_amt_target[1]) << 16;
    m_cutoff_lfo_amt_current[0] = 0;
    m_cutoff_lfo_amt_current[1] = 0;
    m_cutoff_current = m_cutoff_base_current;
    m_resonance_current = static_cast<int32_t>(m_resonance_target) << 8;

    update_coefs(0, 0, 60 << 8);
  }

  INLINE void set_cutoff(uint8_t controller_value) {
    m_cutoff_target = static_cast<int32_t>(controller_value) << 8;
  }

  INLINE void set_resonance(uint8_t controller_value) {
    m_resonance_target = controller_value;
  }

  INLINE int8_t get_cutoff_mod_amt(uint8_t controller_value) {
    static int16_t cutoff_mod_amt_table[128] = {
      -120, -120, -120, -120, -120, -118, -116, -114,
      -112, -110, -108, -106, -104, -102, -100,  -98,
       -96,  -94,  -92,  -90,  -88,  -86,  -84,  -82,
       -80,  -78,  -76,  -74,  -72,  -70,  -68,  -66,
       -64,  -62,  -60,  -58,  -56,  -54,  -52,  -50,
       -48,  -46,  -44,  -42,  -40,  -38,  -36,  -34,
       -32,  -30,  -28,  -26,  -24,  -22,  -20,  -18,
       -16,  -14,  -12,  -10,   -8,   -6,   -4,   -2,
        +0,   +2,   +4,   +6,   +8,  +10,  +12,  +14,
       +16,  +18,  +20,  +22,  +24,  +26,  +28,  +30,
       +32,  +34,  +36,  +38,  +40,  +42,  +44,  +46,
       +48,  +50,  +52,  +54,  +56,  +58,  +60,  +62,
       +64,  +66,  +68,  +70,  +72,  +74,  +76,  +78,
       +80,  +82,  +84,  +86,  +88,  +90,  +92,  +94,
       +96,  +98, +100, +102, +104, +106, +108, +110,
      +112, +114, +116, +118, +120, +120, +120, +120,
    };

    return cutoff_mod_amt_table[controller_value];
  }

  INLINE void set_cutoff_eg_amt(uint8_t index, uint8_t controller_value) {
    m_cutoff_eg_amt_target[index] = get_cutoff_mod_amt(controller_value) << 1;
  }

  INLINE void set_cutoff_lfo_amt(uint8_t index, uint8_t controller_value) {
    m_cutoff_lfo_amt[index] = get_cutoff_mod_amt(controller_value) << 1;
  }

  INLINE void set_cutoff_pitch_amt(uint8_t controller_value) {
    m_cutoff_pitch_amt = ((controller_value - 64) + 4) >> 3;
  }

  INLINE void set_filter_mode(uint8_t controller_value) {
    m_filter_mode = controller_value;
  }

  INLINE void set_cutoff_breath_amt(uint8_t controller_value) {
    m_cutoff_breath_amt = get_cutoff_mod_amt(controller_value) << 1;
  }

  INLINE void set_breath_controller(uint8_t controller_value) {
    m_breath_controller = (controller_value * 16384) / 127;
  }

  INLINE void reset() {
    m_s_1 = 0;
    m_s_2 = 0;
    m_cutoff_base_current = 0;
    m_cutoff_eg_amt_current[0] = 0;
    m_cutoff_eg_amt_current[1] = 0;
    m_cutoff_lfo_amt_current[0] = 0;
    m_cutoff_lfo_amt_current[1] = 0;
  }

  INLINE void process_at_low_rate(uint8_t count, int32_t eg_input, int32_t lfo_input, uint16_t osc_pitch) {
    static_cast<void>(count);
    update_coefs(eg_input, lfo_input, osc_pitch);
  }

  INLINE int32_t process(int32_t audio_input_int24) {
#if 1
    // ZDF (Zero-Delay Feedback) / TPT (Topology-Preserving Transform) State Variable Filter
    int32_t x_0 = audio_input_int24 << FILTER_CALC_SCALING_BITS;

    // The soft clipping in the feedback path is predicted from the integrator
    // states of the previous sample, so its gain is constant within the
    // sample: the zero-delay feedback equation keeps its closed-form solution
    // and needs no iteration
    int32_t s_1 = soft_clip(m_s_1);
    int32_t s_2 = soft_clip(m_s_2);

    // high_pass = (x_0 - (g + k) * s_1 - s_2) / a_0
    int32_t high_pass = multiply_shift_right(m_one_over_a_0,      x_0 - s_2, FILTER_TABLE_FRACTION_BITS)
                      - multiply_shift_right(m_g_plus_k_over_a_0, s_1,       FILTER_TABLE_FRACTION_BITS);

    // Two trapezoidal integrators in series
    int32_t v_1 = multiply_shift_right(m_g, high_pass, FILTER_G_FRACTION_BITS);
    int32_t band_pass = v_1 + s_1;
    int32_t v_2 = multiply_shift_right(m_g, band_pass, FILTER_G_FRACTION_BITS);
    int32_t low_pass = v_2 + s_2;
    m_s_1 = band_pass + v_1;
    m_s_2 = low_pass  + v_2;

    int32_t y_0 = (m_filter_mode >= 64) ? high_pass : low_pass;
#else
    int32_t y_0 = audio_input_int24 << FILTER_CALC_SCALING_BITS;
#endif

    return y_0 >> FILTER_CALC_SCALING_BITS;
  }

private:
  INLINE void update_coefs(int32_t eg_input_q23, int32_t lfo_input_q23, uint16_t osc_pitch) {
    // 0. Round the Q23 control signals down to the resolution this filter actually uses
    int32_t eg_input  = (eg_input_q23  + (1 << 7)) >> 8;
    int32_t lfo_input = (lfo_input_q23 + (1 << 7)) >> 8;

    // 1. Synthesize base cutoff and smoothable modulation signals (LFO, Pitch, Breath)
    int32_t base_candidate = m_cutoff_target;
    base_candidate += ((m_breath_controller * m_cutoff_breath_amt) >> (14 - 2 - 5));

    // 2. Smooth the integrated base modulation target and EG Amt parameters simultaneously
    int32_t base_target = clamp(base_candidate, 0, CONTROLLER_VALUE_Q8_MAX);
    m_cutoff_base_current = approach_exp(m_cutoff_base_current, base_target, SMOOTH_RATE);

    int32_t lfo_mod_target = 0;
    for (int i = 0; i < 2; ++i) {
      m_cutoff_lfo_amt_current[i] = approach_exp(m_cutoff_lfo_amt_current[i], m_cutoff_lfo_amt[i], SMOOTH_RATE);
      lfo_mod_target += ((lfo_input * m_cutoff_lfo_amt_current[i]) >> (15 - 2 - 5));
    }

    int32_t pitch_mod = (((osc_pitch - (60 << 8)) * m_cutoff_pitch_amt) + (1 << ((10 - 1) - 2 - 5))) >> (10 - 2 - 5);

    int32_t eg_amt_target[2] = {
      static_cast<int32_t>(m_cutoff_eg_amt_target[0]) << 16,
      static_cast<int32_t>(m_cutoff_eg_amt_target[1]) << 16
    };
    for (int i = 0; i < 2; ++i) {
      m_cutoff_eg_amt_current[i] = approach_exp(m_cutoff_eg_amt_current[i], eg_amt_target[i], SMOOTH_RATE);
    }

    // 3. Smooth the EG cutoff modulation and add it to the base cutoff
    int32_t eg_mod_target = 0;
    eg_mod_target += ((static_cast<int16_t>(m_cutoff_eg_amt_current[0] >> 16) * eg_input) >> (15 - 2 - 5));
    eg_mod_target += ((static_cast<int16_t>(m_cutoff_eg_amt_current[1] >> 16) * eg_input) >> (15 - 2 - 5));

    // 4. Bound and lock final composite values into active controller value registers
    m_cutoff_current = clamp(m_cutoff_base_current + lfo_mod_target + pitch_mod + eg_mod_target, 0, CONTROLLER_VALUE_Q8_MAX);
    m_resonance_current = approach_exp(m_resonance_current, static_cast<int32_t>(m_resonance_target) << 8, SMOOTH_RATE);

    // 5. Interpolate the coefficient tables and solve the zero-delay feedback
    int32_t g = interpolate_filter_table(g_filter_g_table, m_cutoff_current);
    int32_t k = interpolate_filter_table(g_filter_k_table, m_resonance_current);
    int32_t g_plus_k = g + (k >> (FILTER_TABLE_FRACTION_BITS - FILTER_G_FRACTION_BITS));
    uint32_t a_0 = (1 << 24) + multiply_shift_right(g, g_plus_k, (FILTER_G_FRACTION_BITS * 2) - 24);

    m_g = g;
    m_one_over_a_0 = static_cast<int32_t>((static_cast<uint64_t>(1) << (FILTER_TABLE_FRACTION_BITS + 24)) / a_0);
    m_g_plus_k_over_a_0 = multiply_shift_right(g_plus_k, m_one_over_a_0, FILTER_G_FRACTION_BITS);
  }
};
