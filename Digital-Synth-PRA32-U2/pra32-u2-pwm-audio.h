#pragma once

// PWM Audio Output (independent of the Arduino-Pico PWMAudio library)
//
// - The sample period is (sys_clk / sampling rate) cycles, and it is divided into PRA32_U2_PWM_AUDIO_SUBPERIODS PWM periods
//   (the PWM carrier is sampling rate * PRA32_U2_PWM_AUDIO_SUBPERIODS)
//   - Each PWM slice paces its DMA by its own wrap DREQ, so exactly one subperiod level is latched at each PWM wrap
//     (no sample drops or repeats due to clock or phase mismatch)
// - setSysClk(48000) sets sys_clk to 153.6 MHz (the same as I2S), so the sample period is exactly 3200 cycles
//   - With 4 subperiods (default), the PWM period is 800 cycles (carrier: 192 kHz instead of 48 kHz)
// - The level of a sample (3200 steps) is split into the subperiods (e.g. 801 = 201 + 200 + 200 + 200),
//   so the sum (the average) is exact, and the resolution is the same as with 1 subperiod
//   - Each subperiod gets the level's quotient by the subperiods, and the remainder is spread as one
//     more on as many of them, in a bit-reversed order (e.g. subperiods 0, 2, 1, then 3); the words
//     that add the one more are taken from a table by the two remainders, so a sample costs the
//     quotients and one add and store a subperiod
//   - A higher carrier is more attenuated by the input filters of amplifiers and active speakers,
//     and is less likely to cause audible beat or alias tones (e.g. 48 kHz aliases to 3.9 kHz with a 44.1 kHz ADC)
//   - The extra CPU load is only a few additions and writes per sample (the noise shaping is still once per sample)
// - See "pra32-u2-audio-ring-buffer.h" for the transmit buffer (setBufferFrames())
// - write24() takes the same 32-bit (left-justified 24-bit) samples as PRA32_U2_I2SOutput::write24(), and blocks while the buffer is full
// - The quantization error of the PWM level (3200 steps) is fed back to the next sample (1st-order noise shaping),
//   which moves the quantization noise from the low frequencies to the high frequencies (-8 dB at 3 kHz, +3 dB at 12 kHz)
// - TPDF dither (-1 to +1 step, the sum of 2 uniform random values) is added before the quantization,
//   so that the quantization error does not depend on the signal (no distortion or idle tones in quiet sounds)
//   - The dither is included in the fed back error, so it is also noise shaped (the total noise is +4.8 dB, -3.4 dB at 3 kHz without noise shaping and dither)
//   - The 4 uniform values of a sample (2 a channel) are the 4 bytes of one xorshift32 step, in 1/256 of a step
// - The level is scaled to 5 steps less than the period and raised by 3, so that the dither and the fed
//   back error, less than 2 steps down and 3 up together, keep it in 0 to period without a clamp;
//   the 0.16% of level and the half step of offset this costs are below anything the output shows
// - What a sample costs: about 70 instructions in a row, no calls, from about 110 with the subperiods
//   each worked out from the level, 2 xorshift steps for 16-bit dither, and the level clamped

#include "pra32-u2-audio-ring-buffer.h"
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>

#if !defined(PRA32_U2_PWM_AUDIO_SUBPERIODS)
#define PRA32_U2_PWM_AUDIO_SUBPERIODS (4)  // 1, 2, 4, or 8 (PWM carrier: 48, 96, 192, or 384 kHz)
#endif

class PRA32_U2_PWMAudioOutput : public PRA32_U2_AudioRingBuffer {
  uint32_t  m_pin[2];
  uint32_t  m_slice[2];
  uint32_t  m_shift[2];
  uint32_t  m_period;     // Sample period
  uint32_t  m_subperiod;  // PWM period
  uint32_t  m_level_scale;  // period - 5: what a 16-bit sample is multiplied by
  int32_t   m_level_bias;   // 3 steps, Q16
  uint32_t  m_base_mul[2];  // What the quotients of L and R are multiplied by, for the word of ring 0
  // The words that add one more to a subperiod, by the remainders of L and R ((l << shift) | r),
  // for ring 0 (both channels on one slice: L and R; on two slices: L alone, A and B alike) and
  // ring 1 (R alone, on two slices)
  uint32_t  m_inc_0[PRA32_U2_PWM_AUDIO_SUBPERIODS * PRA32_U2_PWM_AUDIO_SUBPERIODS][PRA32_U2_PWM_AUDIO_SUBPERIODS];
  uint32_t  m_inc_1[PRA32_U2_PWM_AUDIO_SUBPERIODS * PRA32_U2_PWM_AUDIO_SUBPERIODS][PRA32_U2_PWM_AUDIO_SUBPERIODS];
  int32_t   m_error[2];  // Q16 error of the PWM level
  uint32_t  m_dither_seed;

  static constexpr uint32_t SUBPERIOD_SHIFT = (PRA32_U2_PWM_AUDIO_SUBPERIODS == 8) ? 3 : (PRA32_U2_PWM_AUDIO_SUBPERIODS == 4) ? 2 : (PRA32_U2_PWM_AUDIO_SUBPERIODS == 2) ? 1 : 0;

public:
  PRA32_U2_PWMAudioOutput(uint32_t pin_l, uint32_t pin_r)
  : m_pin{pin_l, pin_r}
  , m_slice{}
  , m_shift{}
  , m_period()
  , m_subperiod()
  , m_level_scale()
  , m_level_bias()
  , m_base_mul{}
  , m_inc_0{}
  , m_inc_1{}
  , m_error{}
  , m_dither_seed(1)
  {}

  bool setSysClk(int sampling_rate) {
    // 153.6 MHz = 48 kHz * 3200 (PWM period)
    if ((sampling_rate <= 0) || ((153600000 % sampling_rate) != 0)) {
      return false;
    }
    return set_sys_clock_khz(153600, false);
  }

  bool begin(uint32_t sampling_rate) {
    const uint32_t subperiods = PRA32_U2_PWM_AUDIO_SUBPERIODS;
    static_assert((subperiods == 1) || (subperiods == 2) || (subperiods == 4) || (subperiods == 8),
                  "PRA32_U2_PWM_AUDIO_SUBPERIODS must be 1, 2, 4, or 8");

    m_subperiod = (clock_get_hz(clk_sys) + (sampling_rate * subperiods / 2)) / (sampling_rate * subperiods);
    if (m_subperiod * subperiods > 16384) {
      m_subperiod = 16384 / subperiods;  // So that the Q16 level fits in int32_t
    }
    m_period = m_subperiod * subperiods;
    m_level_scale = m_period - 5;
    m_level_bias = 3 << 16;
    uint32_t mid_level = m_subperiod / 2;
    m_error[0] = 0;
    m_error[1] = 0;

    for (uint32_t ch = 0; ch < 2; ++ch) {
      m_slice[ch] = pwm_gpio_to_slice_num(m_pin[ch]);
      m_shift[ch] = (pwm_gpio_to_channel(m_pin[ch]) == PWM_CHAN_B) ? 16 : 0;
    }

    // If L and R are on the same slice, 1 ring (1 word per frame) has both L (CC A or B) and R (CC B or A)
    // Otherwise, 2 rings for each slice are used (both CC A and B have the same level)
    // 1 frame = PRA32_U2_PWM_AUDIO_SUBPERIODS words
    bool one_ring = (m_slice[0] == m_slice[1]);
    ring_allocate(one_ring ? 1 : 2, subperiods, mid_level | (mid_level << 16));

    // Subperiod i gets the quotient of the level by the subperiods, and one more where the
    // remainder is above i's place in the bit-reversed order (0, 2, 1, 3 for 4), so the sum is
    // exactly the level, and the one mores are spread over the period
    m_base_mul[0] = one_ring ? (1u << m_shift[0]) : 0x00010001u;
    m_base_mul[1] = one_ring ? (1u << m_shift[1]) : 0u;
    for (uint32_t rem_l = 0; rem_l < subperiods; ++rem_l) {
      for (uint32_t rem_r = 0; rem_r < subperiods; ++rem_r) {
        uint32_t index = (rem_l << SUBPERIOD_SHIFT) | rem_r;
        for (uint32_t i = 0; i < subperiods; ++i) {
          uint32_t reversed = 0;
          for (uint32_t b = 1; b < subperiods; b <<= 1) {
            reversed = (reversed << 1) | ((i & b) ? 1 : 0);
          }
          uint32_t one_l = (rem_l > reversed) ? 1u : 0u;
          uint32_t one_r = (rem_r > reversed) ? 1u : 0u;
          if (one_ring) {
            m_inc_0[index][i] = (one_l << m_shift[0]) | (one_r << m_shift[1]);
            m_inc_1[index][i] = 0;
          } else {
            m_inc_0[index][i] = one_l * 0x00010001u;
            m_inc_1[index][i] = one_r * 0x00010001u;
          }
        }
      }
    }

    uint32_t enable_mask = 0;
    for (uint32_t k = 0; k < m_number_of_rings; ++k) {
      uint32_t slice = m_slice[k];

      pwm_config pwm_cfg = pwm_get_default_config();
      pwm_config_set_clkdiv_int(&pwm_cfg, 1);
      pwm_config_set_wrap(&pwm_cfg, m_subperiod - 1);  // The PWM period is (TOP + 1) cycles
      pwm_init(slice, &pwm_cfg, false);
      pwm_hw->slice[slice].cc = mid_level | (mid_level << 16);

      ring_configure_dma(k, pwm_get_dreq(slice), &pwm_hw->slice[slice].cc);

      enable_mask |= 1u << slice;
    }

    for (uint32_t ch = 0; ch < 2; ++ch) {
      gpio_set_function(m_pin[ch], GPIO_FUNC_PWM);
    }

    ring_start_dma();

    // Start the slices at the same time, so that L and R are always in phase
    hw_set_bits(&pwm_hw->en, enable_mask);
    return true;
  }

  INLINE void write24(int32_t left, int32_t right) {
    ring_wait_for_writable();

    // 4 8-bit uniform random values (the bytes of one xorshift32 step), 2 a channel
    uint32_t seed = m_dither_seed;
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    m_dither_seed = seed;

    // TPDF dither in Q16: -65280 to 65280 (-1 to +1 step, in 1/256 of a step)
    int32_t dither_l = static_cast<int32_t>((((seed >> 24) & 0xFFu) + ((seed >> 16) & 0xFFu)) << 8) - 65280;
    int32_t dither_r = static_cast<int32_t>((((seed >> 8)  & 0xFFu) + (seed & 0xFFu))         << 8) - 65280;

    // The level in Q16, 3 to (period - 2) steps for the sample's range, plus the fed back error
    int32_t level_l_q16 = static_cast<int32_t>(((static_cast<uint32_t>(left)  ^ 0x80000000u) >> 16) * m_level_scale) + m_level_bias + m_error[0];
    int32_t level_r_q16 = static_cast<int32_t>(((static_cast<uint32_t>(right) ^ 0x80000000u) >> 16) * m_level_scale) + m_level_bias + m_error[1];
    uint32_t level_l = static_cast<uint32_t>((level_l_q16 + dither_l) >> 16);  // 0 to period, no clamp needed
    uint32_t level_r = static_cast<uint32_t>((level_r_q16 + dither_r) >> 16);
    m_error[0] = level_l_q16 - static_cast<int32_t>(level_l << 16);  // Includes the dither
    m_error[1] = level_r_q16 - static_cast<int32_t>(level_r << 16);

    // The subperiod words: the quotients, then one more from the table where the remainders say so
    const uint32_t subperiods = PRA32_U2_PWM_AUDIO_SUBPERIODS;
    const uint32_t mask = subperiods - 1;
    uint32_t index = ((level_l & mask) << SUBPERIOD_SHIFT) | (level_r & mask);
    uint32_t base_0 = (level_l >> SUBPERIOD_SHIFT) * m_base_mul[0] + (level_r >> SUBPERIOD_SHIFT) * m_base_mul[1];
    const uint32_t* inc_0 = m_inc_0[index];
    uint32_t* words_0 = &m_ring[0][m_write_index];
    for (uint32_t i = 0; i < subperiods; ++i) {
      words_0[i] = base_0 + inc_0[i];
    }
    if (m_number_of_rings == 2) {
      uint32_t base_1 = (level_r >> SUBPERIOD_SHIFT) * 0x00010001u;
      const uint32_t* inc_1 = m_inc_1[index];
      uint32_t* words_1 = &m_ring[1][m_write_index];
      for (uint32_t i = 0; i < subperiods; ++i) {
        words_1[i] = base_1 + inc_1[i];
      }
    }

    ring_advance();
  }
};
