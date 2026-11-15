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

public:
  // LFO Trig Mode [Aut|Off|On|VOn|VOf|VAu]: the LFO of all the Voices (Aut, Off, and On), or one for
  // each Voice (VOn, VOf, and VAu, in Polyphonic; as On, Off, and Aut in the Monophonic modes, which
  // play Voice 0). The four in the middle mirror each other, as Breath Amp Mode [Off|Q|L|LO|QO|Opn]
  static const uint8_t LFO_TRIG_MODE_AUTO       = 0;  // as before: the phase of Saw, S&H, and Square, and the fade, restart at the first note on (Polyphonic and Legato) or every note on (Monophonic)
  static const uint8_t LFO_TRIG_MODE_OFF        = 1;  // free running; the fade restarts as in Aut
  static const uint8_t LFO_TRIG_MODE_ON         = 2;  // every note on restarts the phase (of every wave) and the fade
  static const uint8_t LFO_TRIG_MODE_VOICE_ON   = 3;  // the note on of each Voice restarts its phase (of every wave) and its fade
  static const uint8_t LFO_TRIG_MODE_VOICE_OFF  = 4;  // each Voice free running (a quarter apart at the start); its note on restarts its fade
  static const uint8_t LFO_TRIG_MODE_VOICE_AUTO = 5;  // the note on of each Voice restarts its phase of Saw, S&H, and Square, and its fade (Triangle and Sine run on)

private:
  uint32_t m_lfo_phase[4];
  int32_t  m_lfo_wave_level[4];
  uint32_t m_lfo_rate;
  uint8_t  m_lfo_depth_target[2];
  int32_t  m_lfo_depth_stage_1[4];
  int32_t  m_lfo_depth_result_current[4];  // Q16 (0 to 128 << 16)
  uint8_t  m_lfo_waveform;
  uint16_t m_lfo_fade_coef;
  uint16_t m_lfo_fade_cnt[4];
  uint8_t  m_lfo_fade_level[4];
  int32_t  m_noise_int23;
  int32_t  m_sampled_noise_int23[4];
  int32_t  m_red_noise_acc;
  int32_t  m_red_noise_level;
  uint16_t m_red_noise_gain;
  int32_t  m_red_noise_coef;
  uint8_t  m_pressure_amt;
  uint8_t  m_pressure[4];
  uint8_t  m_lfo_trig_mode;
  uint8_t  m_lfo_voice_mask;  // 0 (the LFO of all the Voices, that of Voice 0) or 3 (one for each Voice)
  uint8_t  m_lfo_phase_quarter;  // the start phase of Voice 0 in quarters (SYNTH_ID, see set_synth_id())

public:
  PRA32_U2_LFO()
  : m_lfo_phase()
  , m_lfo_wave_level()
  , m_lfo_rate()
  , m_lfo_depth_target()
  , m_lfo_depth_stage_1()
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
  , m_lfo_trig_mode()
  , m_lfo_voice_mask()
  , m_lfo_phase_quarter()
  {
    m_lfo_waveform = LFO_WAVEFORM_TRIANGLE;
    m_lfo_fade_coef = LFO_FADE_COEF_OFF;
    m_lfo_trig_mode = LFO_TRIG_MODE_AUTO;
    for (uint32_t n = 0; n < 4; ++n) {
      m_lfo_fade_cnt[n] = m_lfo_fade_coef;
      m_lfo_fade_level[n] = LFO_FADE_LEVEL_MAX;
    }
    m_noise_int23 = 0;
    reset();
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

  INLINE void set_lfo_trig_mode(uint8_t controller_value) {
    static uint8_t lfo_trig_mode_table[6] = {
      LFO_TRIG_MODE_AUTO,
      LFO_TRIG_MODE_OFF,
      LFO_TRIG_MODE_ON,
      LFO_TRIG_MODE_VOICE_ON,
      LFO_TRIG_MODE_VOICE_OFF,
      LFO_TRIG_MODE_VOICE_AUTO,
    };

    int32_t index = ((controller_value * 10) + 128) >> 8;

    m_lfo_trig_mode = lfo_trig_mode_table[index];
    m_lfo_voice_mask = (m_lfo_trig_mode >= LFO_TRIG_MODE_VOICE_ON) ? 3 : 0;
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

  // A note on in Polyphonic (n: its Voice; first: the first note on, with no other note on)
  // Not inlined (one copy in RAM for all the call sites and the Synths)
  __attribute__((noinline)) void __not_in_flash_func(note_on_polyphonic)(uint8_t n, boolean first) {
    switch (m_lfo_trig_mode) {
    case LFO_TRIG_MODE_VOICE_OFF:
      restart_fade(n);
      break;
    case LFO_TRIG_MODE_VOICE_ON:
      restart_phase(n);
      restart_fade(n);
      break;
    case LFO_TRIG_MODE_VOICE_AUTO:
      restart_phase_auto(n);
      restart_fade(n);
      break;
    default:
      note_on_monophonic(first);
      break;
    }
  }

  // A note on in the Monophonic modes (Voice 0), and the note ons of Polyphonic with the LFO of all
  // the Voices (trigger: a note on that restarts the LFO in Aut, as before)
  __attribute__((noinline)) void __not_in_flash_func(note_on_monophonic)(boolean trigger) {
    switch (m_lfo_trig_mode) {
    case LFO_TRIG_MODE_OFF:
    case LFO_TRIG_MODE_VOICE_OFF:
      if (trigger) {
        restart_fade(0);
      }
      break;
    case LFO_TRIG_MODE_AUTO:
    case LFO_TRIG_MODE_VOICE_AUTO:
      if (trigger) {
        restart_phase_auto(0);
        restart_fade(0);
      }
      break;
    default:  // LFO_TRIG_MODE_ON, LFO_TRIG_MODE_VOICE_ON
      restart_phase(0);
      restart_fade(0);
      break;
    }
  }

  template <uint8_t N>
  INLINE int32_t get_output() {
    int32_t lfo_level = multiply_shift_right(m_lfo_depth_result_current[N], m_lfo_wave_level[N & m_lfo_voice_mask], 16 + 7);
    return lfo_level;
  }

  // The Synths of PRA32-U2/M start a quarter apart (Synth n at n / 4), as the Voices of a Synth do
  // (Voice n of the Main Synth at n / 4), and run a little faster by SYNTH_ID (see set_lfo_rate()),
  // as Voice n by n; so Sub Synth n and Voice n of the Main Synth run alike
  template <uint32_t SYNTH_ID>
  INLINE void set_synth_id() {
    m_lfo_phase_quarter = SYNTH_ID & 3;
    reset();
  }

  // The phases a quarter apart, so that the LFOs of the Voices (VOf) run apart from the start
  INLINE void reset() {
    for (uint32_t n = 0; n < 4; ++n) {
      m_lfo_phase[n] = ((n + m_lfo_phase_quarter) & 3) << 22;
      m_sampled_noise_int23[n] = m_noise_int23;
    }
  }

  // ONLY_VOICE_0: true for a synth that never plays voices 1-3 (e.g. the Sub Synths of PRA32-U2/M)
  template <boolean ONLY_VOICE_0 = false>
  INLINE void process_at_low_rate(uint8_t count, int32_t noise_int23) {
    if (is_slow_smoothing_period(count)) {
      update_lfo_depth_current<0>();
if constexpr (ONLY_VOICE_0 == false) {
      update_lfo_depth_current<1>();
      update_lfo_depth_current<2>();
      update_lfo_depth_current<3>();
}
    }

    m_noise_int23 = noise_int23;
    update_red_noise_level(noise_int23);
    update_lfo_wave_level(0);
  }

  // The LFO of Voice n (1-3), at the low rate of the Voice; always run (whether they are used or
  // not), so that the load stays the same. Not inlined (one copy in RAM for the 3 Voices)
  __attribute__((noinline)) void __not_in_flash_func(process_voice_at_low_rate)(uint8_t n) {
    update_lfo_wave_level(n);
  }

private:
  template <uint8_t N>
  INLINE void update_lfo_depth_current() {
    // The depth scales the LFO output for all destinations, like an Amt parameter,
    // so it is smoothed slower, in Q16 (with the Modulation Wheel and the After Touch,
    // which are often sent in sparse and irregular steps)
    int32_t lfo_depth_target = high_byte((m_lfo_depth_target[0] << 1) * m_lfo_fade_level[N & m_lfo_voice_mask]) + m_lfo_depth_target[1]
                              + ((m_pressure_amt * m_pressure[N]) >> 7);
    lfo_depth_target = minimum(lfo_depth_target, 128);
    approach_exp_slow(m_lfo_depth_stage_1[N], m_lfo_depth_result_current[N], lfo_depth_target << 16);
  }

  INLINE int32_t get_lfo_wave_level(uint8_t n, uint32_t phase) {
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
      level = m_sampled_noise_int23[n];
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

  INLINE void update_lfo_wave_level(uint8_t n) {
    --m_lfo_fade_cnt[n];

    const int32_t is_zero = (m_lfo_fade_cnt[n] == 0);
    m_lfo_fade_cnt[n] = (m_lfo_fade_coef * is_zero) + (m_lfo_fade_cnt[n] * (is_zero ^ 1));
    m_lfo_fade_level[n] += (is_zero & (m_lfo_fade_level[n] < LFO_FADE_LEVEL_MAX));

    m_lfo_wave_level[n] = get_lfo_wave_level(n, m_lfo_phase[n]);

    // The phase is advanced after the current level is output, and a crossing
    // detected by this advance takes effect from the next update. The LFO of Voice n runs a little
    // faster, by n (about 0.0007 Hz each, as the Synths of PRA32-U2/M by SYNTH_ID), so that the
    // LFOs of the Voices (VOf and VAu, Triangle and Sine) drift apart over minutes rather than stay
    // a quarter apart; Voice 0, which the LFO of all the Voices is, as before
    const uint32_t lfo_rate = m_lfo_rate + ((m_lfo_rate > 0) * n);
    m_lfo_phase[n] += lfo_rate;
    m_lfo_phase[n] &= 0x00FFFFFF;

    const int32_t mask = -((m_lfo_waveform == LFO_WAVEFORM_RANDOM) & (m_lfo_phase[n] < lfo_rate));
    m_sampled_noise_int23[n] ^= (m_sampled_noise_int23[n] ^ m_noise_int23) & mask;
  }

  INLINE void restart_fade(uint8_t n) {
    m_lfo_fade_level[n] *= (m_lfo_fade_coef <= LFO_FADE_COEF_OFF);
  }

  // As before (Aut): the phase of Saw, S&H, and Square restarts (and S&H takes a new value)
  INLINE void restart_phase_auto(uint8_t n) {
    const int32_t is_matched = (m_lfo_waveform == LFO_WAVEFORM_SAW_DOWN) |
                               (m_lfo_waveform == LFO_WAVEFORM_RANDOM)   |
                               (m_lfo_waveform == LFO_WAVEFORM_SQUARE);
    const uint32_t match_mask = -is_matched;
    m_lfo_phase[n] &= ~match_mask;
    m_sampled_noise_int23[n] ^= (m_sampled_noise_int23[n] ^ m_noise_int23) & match_mask;
  }

  // The phase of every wave restarts where the wave starts from the center going up (Triangle and
  // Sine), or from the top (Saw and Square); S&H takes a new value
  INLINE void restart_phase(uint8_t n) {
    m_lfo_phase[n] = (m_lfo_waveform == LFO_WAVEFORM_TRIANGLE) ? 0x400000 : 0x000000;
    m_sampled_noise_int23[n] = m_noise_int23;
  }
};
