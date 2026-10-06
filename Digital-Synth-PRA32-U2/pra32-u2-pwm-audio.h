#pragma once

// PWM Audio Output (independent of the Arduino-Pico PWMAudio library)
//
// - The PWM period is (sys_clk / sampling rate) cycles, and each PWM slice paces its DMA by its own wrap DREQ,
//   so exactly one sample is latched at each PWM wrap (no sample drops or repeats due to clock or phase mismatch)
// - setSysClk(48000) sets sys_clk to 153.6 MHz (the same as I2S), so the PWM period is exactly 3200 cycles
// - See "pra32-u2-audio-ring-buffer.h" for the transmit buffer (setBufferFrames())
// - write24() takes the same 32-bit (left-justified 24-bit) samples as PRA32_U2_I2SOutput::write24(), and blocks while the buffer is full

#include "pra32-u2-audio-ring-buffer.h"
#include <hardware/pwm.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>

class PRA32_U2_PWMAudioOutput : public PRA32_U2_AudioRingBuffer {
  uint32_t  m_pin[2];
  uint32_t  m_slice[2];
  uint32_t  m_shift[2];
  uint32_t  m_period;

public:
  PRA32_U2_PWMAudioOutput(uint32_t pin_l, uint32_t pin_r)
  : m_pin{pin_l, pin_r}
  , m_slice{}
  , m_shift{}
  , m_period()
  {}

  bool setSysClk(int sampling_rate) {
    // 153.6 MHz = 48 kHz * 3200 (PWM period)
    if ((sampling_rate <= 0) || ((153600000 % sampling_rate) != 0)) {
      return false;
    }
    return set_sys_clock_khz(153600, false);
  }

  bool begin(uint32_t sampling_rate) {
    m_period = (clock_get_hz(clk_sys) + (sampling_rate / 2)) / sampling_rate;
    if (m_period > 65536) {
      m_period = 65536;
    }
    uint32_t mid_level = m_period / 2;

    for (uint32_t ch = 0; ch < 2; ++ch) {
      m_slice[ch] = pwm_gpio_to_slice_num(m_pin[ch]);
      m_shift[ch] = (pwm_gpio_to_channel(m_pin[ch]) == PWM_CHAN_B) ? 16 : 0;
    }

    // If L and R are on the same slice, 1 ring (1 word per frame) has both L (CC A or B) and R (CC B or A)
    // Otherwise, 2 rings for each slice are used (both CC A and B have the same level)
    ring_allocate((m_slice[0] == m_slice[1]) ? 1 : 2, 1, mid_level | (mid_level << 16));

    uint32_t enable_mask = 0;
    for (uint32_t k = 0; k < m_number_of_rings; ++k) {
      uint32_t slice = m_slice[k];

      pwm_config pwm_cfg = pwm_get_default_config();
      pwm_config_set_clkdiv_int(&pwm_cfg, 1);
      pwm_config_set_wrap(&pwm_cfg, m_period - 1);  // The PWM period is (TOP + 1) cycles
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

    uint32_t level_l = ((static_cast<uint32_t>(left)  ^ 0x80000000u) >> 16) * m_period >> 16;
    uint32_t level_r = ((static_cast<uint32_t>(right) ^ 0x80000000u) >> 16) * m_period >> 16;

    if (m_number_of_rings == 1) {
      m_ring[0][m_write_index] = (level_l << m_shift[0]) | (level_r << m_shift[1]);
    } else {
      m_ring[0][m_write_index] = level_l * 0x00010001u;
      m_ring[1][m_write_index] = level_r * 0x00010001u;
    }

    ring_advance();
  }
};
