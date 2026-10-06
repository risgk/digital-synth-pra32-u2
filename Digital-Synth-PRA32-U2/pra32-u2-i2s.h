#pragma once

// PIO I2S Output (independent of the Arduino-Pico I2S library)
//
// - 32-bit slots (BCLK = 64 fs), and the 24-bit samples are left-justified (the same data as I2S::write24())
// - setSysClk(48000) sets sys_clk to 153.6 MHz (= 128 * 48 kHz * 25), so BCLK has no fractional divider jitter
// - MCLK is not supported
// - See "pra32-u2-audio-ring-buffer.h" for the transmit buffer (setBufferFrames())
// - begin() and end() can be called again (e.g. to stop the output while writing to the flash)

#include "pra32-u2-audio-ring-buffer.h"
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <algorithm>
#include <cstring>

class PRA32_U2_I2SOutput : public PRA32_U2_AudioRingBuffer {
  static const uint32_t I2S_PROGRAM_LENGTH = 8;

  uint32_t      m_freq;
  int           m_pin_data;
  int           m_pin_bclk;
  bool          m_swap_clocks;
  bool          m_running;

  PIO           m_pio;
  uint          m_sm;
  uint          m_offset;
  uint16_t      m_instructions[I2S_PROGRAM_LENGTH];
  pio_program_t m_program;

public:
  PRA32_U2_I2SOutput()
  : m_freq(48000)
  , m_pin_data(-1)
  , m_pin_bclk(-1)
  , m_swap_clocks(false)
  , m_running(false)
  , m_pio()
  , m_sm()
  , m_offset()
  , m_instructions{}
  {
    std::memset(&m_program, 0, sizeof(m_program));
  }

  bool setSysClk(int sampling_rate) {
    // 153.6 MHz = 128 * 48 kHz * 25 (2 PIO cycles per BCLK cycle, BCLK = 64 fs)
    if ((sampling_rate <= 0) || ((153600000 % (128 * sampling_rate)) != 0)) {
      return false;
    }
    return set_sys_clock_khz(153600, false);
  }

  bool setFrequency(int sampling_rate) {
    if (m_running || (sampling_rate <= 0)) {
      return false;
    }
    m_freq = sampling_rate;
    return true;
  }

  bool setDATA(pin_size_t pin) {
    if (m_running) {
      return false;
    }
    m_pin_data = pin;
    return true;
  }

  bool setBCLK(pin_size_t pin) {  // LRCLK is (BCLK + 1), or (BCLK - 1) if swapClocks() is called
    if (m_running) {
      return false;
    }
    m_pin_bclk = pin;
    return true;
  }

  bool swapClocks() {  // The pin set by setBCLK() is LRCLK, and the next pin is BCLK
    if (m_running) {
      return false;
    }
    m_swap_clocks = true;
    return true;
  }

  bool begin() {
    if (m_running || (m_pin_data < 0) || (m_pin_bclk < 0)) {
      return false;
    }

    // The I2S state machine executes 2 instructions (1 PIO cycle each) per BCLK cycle
    uint32_t pio_freq = 128 * m_freq;
    uint64_t div_256  = ((static_cast<uint64_t>(clock_get_hz(clk_sys)) << 8) + (pio_freq / 2)) / pio_freq;
    uint32_t div_int  = static_cast<uint32_t>(div_256 >> 8);
    uint8_t  div_frac = static_cast<uint8_t>(div_256 & 0xFF);

    build_program();

    uint32_t pin_min = std::min(m_pin_data, m_pin_bclk);
    uint32_t pin_max = std::max(m_pin_data, m_pin_bclk + 1);

    if (!pio_claim_free_sm_and_add_program_for_gpio_range(&m_program, &m_pio, &m_sm, &m_offset,
                                                          pin_min, pin_max - pin_min + 1, true)) {
      return false;
    }

    pio_gpio_init(m_pio, m_pin_data);
    pio_gpio_init(m_pio, m_pin_bclk);
    pio_gpio_init(m_pio, m_pin_bclk + 1);
    pio_sm_config i2s_cfg = pio_get_default_sm_config();
    sm_config_set_wrap(&i2s_cfg, m_offset, m_offset + I2S_PROGRAM_LENGTH - 1);
    sm_config_set_sideset(&i2s_cfg, 2, false, false);
    sm_config_set_sideset_pins(&i2s_cfg, m_pin_bclk);
    sm_config_set_out_pins(&i2s_cfg, m_pin_data, 1);
    sm_config_set_out_shift(&i2s_cfg, false, true, 32);  // MSB first, autopull
    sm_config_set_fifo_join(&i2s_cfg, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv_int_frac8(&i2s_cfg, div_int, div_frac);
    pio_sm_init(m_pio, m_sm, m_offset, &i2s_cfg);
    pio_sm_set_pins_with_mask(m_pio, m_sm, 0, (1u << m_pin_data) | (3u << m_pin_bclk));
    pio_sm_set_consecutive_pindirs(m_pio, m_sm, m_pin_data, 1, true);
    pio_sm_set_consecutive_pindirs(m_pio, m_sm, m_pin_bclk, 2, true);
    pio_sm_exec(m_pio, m_sm, pio_encode_set(pio_y, 32 - 2));

    // 1 ring (2 words per frame: L, R)
    ring_allocate(1, 2, 0);
    ring_configure_dma(0, pio_get_dreq(m_pio, m_sm, true), &m_pio->txf[m_sm]);
    ring_start_dma();  // Fills the TX FIFO with silence

    pio_sm_set_enabled(m_pio, m_sm, true);

    m_running = true;
    return true;
  }

  bool end() {
    if (m_running) {
      pio_sm_set_enabled(m_pio, m_sm, false);

      ring_free();

      pio_sm_clear_fifos(m_pio, m_sm);
      pio_remove_program_and_unclaim_sm(&m_program, m_pio, m_sm, m_offset);
      m_running = false;
    }
    return true;
  }

  INLINE size_t write24(int32_t left, int32_t right) {
    if (!m_running) {
      return 0;
    }
    ring_wait_for_writable();
    m_ring[0][m_write_index]     = left;
    m_ring[0][m_write_index + 1] = right;
    ring_advance();
    return 1;
  }

private:
  uint32_t sideset(uint32_t bclk, uint32_t lrclk) {
    uint32_t value = m_swap_clocks ? ((bclk << 1) | lrclk) : ((lrclk << 1) | bclk);
    return pio_encode_sideset(2, value);
  }

  void build_program() {
    // Each BCLK cycle consists of 2 instructions (BCLK low, BCLK high)
    // Y = 30 (bits per slot - 2), and LRCLK changes 1 BCLK cycle before the MSB (I2S format)
    m_instructions[0] = pio_encode_mov(pio_x, pio_y)   | sideset(1, 0);  // (2nd half of the R LSB)
    m_instructions[1] = pio_encode_out(pio_pins, 1)    | sideset(0, 0);  // L bits
    m_instructions[2] = pio_encode_jmp_x_dec(1)        | sideset(1, 0);
    m_instructions[3] = pio_encode_out(pio_pins, 1)    | sideset(0, 1);  // L LSB
    m_instructions[4] = pio_encode_mov(pio_x, pio_y)   | sideset(1, 1);
    m_instructions[5] = pio_encode_out(pio_pins, 1)    | sideset(0, 1);  // R bits
    m_instructions[6] = pio_encode_jmp_x_dec(5)        | sideset(1, 1);
    m_instructions[7] = pio_encode_out(pio_pins, 1)    | sideset(0, 0);  // R LSB

    m_program.instructions = m_instructions;
    m_program.length       = I2S_PROGRAM_LENGTH;
    m_program.origin       = -1;
  }
};
