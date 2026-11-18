#pragma once

// The values shown as they work (3 characters at most, right-aligned), by the panel (PRA32_U2_ControlPanel_calc_value_display()
// of "pra32-u2-control-panel.h") and by PRA32-U2 Native (PRA32U2Engine::getValueText(), also without the panel); after
// "pra32-u2-synth.h" (the tables of the LFO and the Filter); the same as PRA32-U2 Editor
//
// They are in the RAM (__not_in_flash_func), as the panel calls them on the secondary core, between the samples, so that they
// cause no XIP cache misses; noinline, as each has more than one caller or would be put in the flash if not inlined (GCC, -Os)

#include "pra32-u2-common.h"

// The text (3 characters at most) right-aligned in 3 characters
static INLINE void PRA32_U2_ControlPanel_copy_right_aligned(char value_display_text[5], const char* buff, uint32_t length) {
  for (uint32_t i = 0; i < 3; ++i) {
    value_display_text[i] = (i >= 3 - length) ? buff[i - (3 - length)] : ' ';
  }
  value_display_text[3] = '\0';
}

static INLINE void PRA32_U2_ControlPanel_append_decimal(char* buff, uint32_t& length, uint32_t value) {
  if (value >= 100) { buff[length++] = '0' + (value / 100); }
  if (value >= 10)  { buff[length++] = '0' + ((value / 10) % 10); }
  buff[length++] = '0' + (value % 10);
}

// A time in 1/10000 ms, as it works (3 characters at most, right-aligned; the same as timeDispValue() of PRA32-U2 Editor):
// the milliseconds rounded, without the unit (e.g. "100"), and the seconds with "." (e.g. "1.0", and "28." for 27.9)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_format_time)(uint32_t t, char value_display_text[5]) {
  char     buff[4];
  uint32_t length = 0;

  if (t < 9995000) {  // below 999.5 ms
    PRA32_U2_ControlPanel_append_decimal(buff, length, (t + 5000) / 10000);
  } else {
    uint32_t tenths = (t + 500000) / 1000000;  // the seconds in 0.1 s
    if (tenths < 100) {
      buff[length++] = '0' + (tenths / 10);
      buff[length++] = '.';
      buff[length++] = '0' + (tenths % 10);
    } else {
      PRA32_U2_ControlPanel_append_decimal(buff, length, (t + 5000000) / 10000000);
      buff[length++] = '.';
    }
  }

  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// The times of the EG and the Amp (decay_release: Decay and Release, 3 times Attack), and Portamento Time (as Attack):
// Attack 1 ms * 10^(value / 32), 1.00 ms to 9.31 s, by the mantissas of 10^(value / 32) for a decade of 32 values
// (5 digits, so that the times rounded are the same as the exact ones)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_time)(uint8_t controller_value, boolean decay_release, char value_display_text[5]) {
  static const uint32_t ary_mantissas[32] PRA32_U2_TABLE_IN_RAM(panel_text) = {
    10000, 10746, 11548, 12409, 13335, 14330, 15399, 16548, 17783, 19110, 20535, 22067, 23714, 25483, 27384, 29427,
    31623, 33982, 36517, 39242, 42170, 45316, 48697, 52330, 56234, 60430, 64938, 69783, 74989, 80584, 86596, 93057
  };
  static const uint32_t ary_decades[4] PRA32_U2_TABLE_IN_RAM(panel_text) = { 1, 10, 100, 1000 };
  // The time in 1/10000 ms (2.8e8 at most)
  PRA32_U2_ControlPanel_format_time(ary_mantissas[controller_value & 0x1F] * (decay_release ? 3 : 1) * ary_decades[(controller_value >> 5) & 0x03],
                                    value_display_text);
}

// A frequency in 0.001 Hz, as it works (3 characters at most, right-aligned; the same as frequencyDispValue() of PRA32-U2
// Editor): "0", ".01" to ".99" (below 1 Hz), "1.0" to "9.9", "10" to "999", "1k0" to "9k9", and "10k" to "99k" (kHz);
// also for the Q of Filter Resonance (in 0.001)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_format_frequency)(uint32_t milli_hz, char value_display_text[5]) {
  char     buff[4];
  uint32_t length = 0;

  if (milli_hz == 0) {
    buff[length++] = '0';
  } else if (((milli_hz + 5) / 10) < 100) {
    uint32_t hundredths = (milli_hz + 5) / 10;
    buff[length++] = '.';
    buff[length++] = '0' + (hundredths / 10);
    buff[length++] = '0' + (hundredths % 10);
  } else if (((milli_hz + 50) / 100) < 100) {
    uint32_t tenths = (milli_hz + 50) / 100;
    buff[length++] = '0' + (tenths / 10);
    buff[length++] = '.';
    buff[length++] = '0' + (tenths % 10);
  } else if (((milli_hz + 500) / 1000) < 1000) {
    PRA32_U2_ControlPanel_append_decimal(buff, length, (milli_hz + 500) / 1000);
  } else if (((milli_hz + 50000) / 100000) < 100) {
    uint32_t hundreds = (milli_hz + 50000) / 100000;  // 0.1 kHz
    buff[length++] = '0' + (hundreds / 10);
    buff[length++] = 'k';
    buff[length++] = '0' + (hundreds % 10);
  } else {
    uint32_t kilos = (milli_hz + 500000) / 1000000;
    buff[length++] = '0' + (kilos / 10);
    buff[length++] = '0' + (kilos % 10);
    buff[length++] = 'k';
  }

  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// The phase step of an LFO (of the LFO, or of the Chorus) per control period (SAMPLING_RATE / 4) -> the frequency in 0.001 Hz
// (truncated, so that the value shown is rounded only once)
static INLINE uint32_t PRA32_U2_ControlPanel_milli_hz_from_lfo_step(uint32_t step) {
  return static_cast<uint32_t>((static_cast<uint64_t>(step) * ((SAMPLING_RATE / 4) * 1000)) >> 24);
}

// The times and the rates: Portamento Time (0 for none), LFO Fade Time (0 for none), LFO Rate (with LFO Wave Nos, the
// cutoff of the low-pass of the noise, "Off" for none), Chorus Rate, and Chorus Depth (the milliseconds, as the frequency,
// e.g. ".04", "1.3", and "5.3"), by the tables (or the steps) that the synth uses, so that they are shown as they work (Delay Time: the milliseconds, as before)
static __attribute__((noinline)) boolean __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_time_rate)(uint8_t control_target, uint8_t controller_value, uint8_t lfo_wave, char value_display_text[5]) {
  uint32_t v = controller_value;

  if (control_target == PORTAMENTO) {
    if (v == 0) {
      PRA32_U2_ControlPanel_format_time(0, value_display_text);  // "0"
    } else {
      PRA32_U2_ControlPanel_calc_value_display_time(v, false, value_display_text);
    }
  } else if (control_target == LFO_FADE_TIME) {
    if (v == 0) {
      PRA32_U2_ControlPanel_format_time(0, value_display_text);  // "0"
    } else {  // the fade of LFO_FADE_LEVEL_MAX (128) steps, of the coef control periods each: coef * 32 / 3 ms
      PRA32_U2_ControlPanel_format_time(((g_lfo_fade_coef_table[v] * 320000) + 1) / 3, value_display_text);
    }
  } else if (control_target == LFO_RATE) {
    if ((((lfo_wave * 10) + 128) >> 8) == 2) {  // Nos (Red Noise)
      uint32_t cutoff = g_lfo_red_noise_cutoff_table[v];
      if (cutoff == 0xFFFFFFFF) {
        value_display_text[0] = 'O';
        value_display_text[1] = 'f';
        value_display_text[2] = 'f';
        value_display_text[3] = '\0';
      } else {
        PRA32_U2_ControlPanel_format_frequency(cutoff, value_display_text);
      }
    } else {
      PRA32_U2_ControlPanel_format_frequency(PRA32_U2_ControlPanel_milli_hz_from_lfo_step(g_lfo_rate_table[v]), value_display_text);
    }
  } else if (control_target == CHORUS_RATE) {
    PRA32_U2_ControlPanel_format_frequency(PRA32_U2_ControlPanel_milli_hz_from_lfo_step(g_chorus_rate_table[v]), value_display_text);
  } else if (control_target == CHORUS_DEPTH) {  // +/- (value / 24) ms, 126 at most: in 0.001 ms (truncated), as the frequency
    PRA32_U2_ControlPanel_format_frequency((minimum(v, 126) * 125) / 3, value_display_text);
  } else {
    return false;
  }

  return true;
}

// Filter Cutoff, as it works (the same as PRA32-U2 Editor): 13.75 Hz * 2^((value - 1) / 12), 13.0 Hz to 19.9 kHz, by the
// frequencies of the top octave (116-127) in 0.001 Hz * 128, truncated, shifted right by the octaves below it and by 7
// (so that the frequency in 0.001 Hz is truncated, and the value shown is rounded only once)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_cutoff)(uint8_t controller_value, char value_display_text[5]) {
  static const uint32_t ary_top_octave[12] PRA32_U2_TABLE_IN_RAM(panel_text) = {
    1350154473, 1430438835, 1515497155, 1605613305, 1701088041, 1802240000,
    1909406767, 2022946001, 2143236630, 2270680112, 2405701778, 2548752250
  };
  uint32_t v        = minimum(controller_value, 127);
  uint32_t octaves  = (127 - v) / 12;
  uint32_t milli_hz = ary_top_octave[(v + (octaves * 12)) - 116] >> (octaves + 7);
  PRA32_U2_ControlPanel_format_frequency(milli_hz, value_display_text);
}

// Fine Tune, as it works (the same as PRA32-U2 Editor): the cents, rounded (1.5625 cents a value, e.g. "+2" for 1.5625,
// and "+98" for 98.4375), with the sign but where it does not fit ("100" for -100, 0)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_fine_tune)(uint8_t controller_value, char value_display_text[5]) {
  int32_t  n = static_cast<int32_t>(controller_value) - 64;
  uint32_t a = (n >= 0) ? n : -n;
  char     buff[4];
  uint32_t length = 0;
  if (n <= -64) {
    PRA32_U2_ControlPanel_append_decimal(buff, length, 100);
  } else if (a == 0) {
    buff[length++] = '0';
  } else {
    buff[length++] = (n > 0) ? '+' : '-';
    PRA32_U2_ControlPanel_append_decimal(buff, length, ((a * 25) + 8) / 16);
  }
  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// Filter Resonance (the Q, as the frequency, e.g. ".71", "5.7", "256", by k = 1 / Q of the table that the Filter uses,
// "Osc" for the self-oscillation, 123-127), Filter Drive and Distortion Drive (the gain in dB, 2^(value / 24): 6.02 / 24 dB
// a value, rounded, 1 at least but at 0, e.g. "+1" for +0.25, "+6", "+32"), and Stretch Tune (the cents at note number 108, 37.5 / 64 cents a value, rounded,
// e.g. "+5" for +4.69, and "-38" for -37.5), Filter EG Amt and LFO Filter Amt (the cutoff in semitones, 2 a value, +-120
// at most, e.g. "+2", and "110" for +-110: with the sign but where it does not fit), Breath Filter Amt (the same, 2x, with
// the max breath: +-240 at most), as they work (the same as filterTuneDispValue() of PRA32-U2 Editor)
static __attribute__((noinline)) boolean __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_filter_tune)(uint8_t control_target, uint8_t controller_value, char value_display_text[5]) {
  uint32_t v = minimum(controller_value, 127);
  char     buff[4];
  uint32_t length = 0;

  if (control_target == FILTER_RESO) {
    if (v > 122) {
      buff[length++] = 'O';
      buff[length++] = 's';
      buff[length++] = 'c';
    } else {  // k is in Q30 (2^30 / 256 at least); the Q in 0.001 = 1000 * 2^22 / (k >> 8), within 32 bits
      PRA32_U2_ControlPanel_format_frequency(4194304000u / (static_cast<uint32_t>(g_filter_k_table[v]) >> 8), value_display_text);
      return true;
    }
  } else if ((control_target == FILTER_DRIVE) || (control_target == DISTORTION)) {
    uint32_t db = maximum(((v * 16441) + 32768) >> 16, (v == 0) ? 0 : 1);  // 20 * log10(2) / 24 = 16441 / 65536 (0.2508583);
                                                                          // 1 at least but at 0 (not shown as 0)
    if (db == 0) {
      buff[length++] = '0';
    } else {
      buff[length++] = '+';
      PRA32_U2_ControlPanel_append_decimal(buff, length, db);
    }
  } else if ((control_target == FILTER_EG_AMT) || (control_target == LFO_FILTER_AMT) || (control_target == BTH_FILTER_AMT)) {
    int32_t  n = clamp(static_cast<int32_t>(v) - 64, -60, 60);
    uint32_t a = static_cast<uint32_t>((n >= 0) ? n : -n) * ((control_target == BTH_FILTER_AMT) ? 4 : 2);
    if (a == 0) {
      buff[length++] = '0';
    } else {
      if (a < 100) {
        buff[length++] = (n > 0) ? '+' : '-';
      }
      PRA32_U2_ControlPanel_append_decimal(buff, length, a);
    }
  } else if (control_target == STRETCH_TUNE) {
    int32_t  n = static_cast<int32_t>(v) - 64;
    uint32_t a = (n >= 0) ? n : -n;
    if (a == 0) {
      buff[length++] = '0';
    } else {
      buff[length++] = (n > 0) ? '+' : '-';
      PRA32_U2_ControlPanel_append_decimal(buff, length, ((a * 75) + 64) / 128);
    }
  } else {
    return false;
  }

  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
  return true;
}

// Amp Gain, as it works (the same as ampGainDispValue() of PRA32-U2 Editor): the gain in dB, rounded, but -1 at most
// other than 127 ("0" at 127, e.g. "-1" at 117-126, "-12" at 64, and "-84" at 1), and "Off" at 0; the level is
// (value / 127)^2, as PRA32_U2_Amp (((value * value) * 16384) / 16129 in Q14, the integer)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_amp_gain)(uint8_t controller_value, char value_display_text[5]) {
  // -dB, rounded (1 at least for 1-126)
  static const uint8_t ary_minus_db[128] PRA32_U2_TABLE_IN_RAM(panel_text) = {
     0, 84, 72, 65, 60, 56, 53, 50, 48, 46, 44, 43, 41, 40, 38, 37,
    36, 35, 34, 33, 32, 31, 30, 30, 29, 28, 28, 27, 26, 26, 25, 24,
    24, 23, 23, 22, 22, 21, 21, 21, 20, 20, 19, 19, 18, 18, 18, 17,
    17, 17, 16, 16, 16, 15, 15, 15, 14, 14, 14, 13, 13, 13, 12, 12,
    12, 12, 11, 11, 11, 11, 10, 10, 10, 10,  9,  9,  9,  9,  8,  8,
     8,  8,  8,  7,  7,  7,  7,  7,  6,  6,  6,  6,  6,  5,  5,  5,
     5,  5,  5,  4,  4,  4,  4,  4,  3,  3,  3,  3,  3,  3,  2,  2,
     2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0
  };
  uint32_t v = minimum(controller_value, 127);
  char     buff[4];
  uint32_t length = 0;
  if (v == 0) {
    buff[length++] = 'O';
    buff[length++] = 'f';
    buff[length++] = 'f';
  } else if (v == 127) {
    buff[length++] = '0';
  } else {
    buff[length++] = '-';
    PRA32_U2_ControlPanel_append_decimal(buff, length, ary_minus_db[v]);
  }
  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// EG Sustain, Amp Sustain, EG Level Velo Sens, and Amp Level Velo Sens (65 steps: (value + 1) >> 1 / 64), Mixer Osc Mix
// (the same, the power of Osc 2, as the table of PRA32_U2_Osc is the square root: Osc 1 100% at 0, Osc 1 50% + Osc 2 50%
// at 63-64, and Osc 2 100% at 127), LFO Depth (value / 128, 127 as 128), Chorus Level and Delay Level (65 steps:
// ((value + 1) >> 1) * 2 / 128), Delay Feedback (value / 256, 0% to 49.6%; shown as 1% to 49% but at 0, so that it is not
// shown as 0% or 50%), and Filter Key Track, EG Att/Dec Velo Sens, EG Rel Velo Sens, and EG Att/Dec Key Track (65 steps:
// (value - 63) >> 1 / 32, -100% at 0, 0% at 63-64, and +100% at 127), as they work: the % rounded, with the sign but
// where it does not fit, and with "%" where it fits (e.g. "2%", "50%", "+3%", "+13", and "100"), the same as
// percentDispValue() of PRA32-U2 Editor; and Amp Gain (above)
static __attribute__((noinline)) boolean __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_percent)(uint8_t control_target, uint8_t controller_value, char value_display_text[5]) {
  uint32_t v = minimum(controller_value, 127);
  uint32_t percent;
  int32_t  n = 0;  // the sign (of the ones with the sign)

  if ((control_target == EG_SUSTAIN) || (control_target == AMP_SUSTAIN) || (control_target == MIXER_OSC_MIX) ||
      (control_target == EG_VEL_SENS) || (control_target == AMP_VEL_SENS)) {
    percent = ((((v + 1) >> 1) * 25) + 8) / 16;
  } else if (control_target == LFO_DEPTH) {
    percent = ((((v == 127) ? 128 : v) * 25) + 16) / 32;
  } else if ((control_target == CHORUS_MIX) || (control_target == DELAY_LEVEL)) {
    percent = (((((v + 1) >> 1) << 1) * 25) + 16) / 32;
  } else if (control_target == DELAY_FEEDBACK) {
    percent = (v == 0) ? 0 : clamp(((v * 25) + 32) / 64, 1, 49);
  } else if ((control_target == FILTER_KEY_TRK) || (control_target == A_D_VEL_SENS) || (control_target == REL_VEL_SENS) ||
             (control_target == A_D_KEY_TRK)) {
    n = (static_cast<int32_t>(v) - 63) >> 1;  // -32 to +32 (1/32 each), as PRA32_U2_Filter and PRA32_U2_EG
    percent = ((static_cast<uint32_t>((n >= 0) ? n : -n) * 25) + 4) / 8;  // 3.125% each, rounded
  } else if (control_target == AMP_GAIN) {  // dB (here, so that it is not put in the switch, which gets larger)
    PRA32_U2_ControlPanel_calc_value_display_amp_gain(controller_value, value_display_text);
    return true;
  } else {
    return false;
  }

  char     buff[4];
  uint32_t length = 0;
  if ((n != 0) && (percent < 100)) {
    buff[length++] = (n > 0) ? '+' : '-';
  }
  PRA32_U2_ControlPanel_append_decimal(buff, length, percent);
  if (length < 3) {
    buff[length++] = '%';
  }
  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
  return true;
}
