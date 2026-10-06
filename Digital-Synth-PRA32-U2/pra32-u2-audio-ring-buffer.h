#pragma once

// Audio Ring Buffer (base class of PRA32_U2_PWMAudioOutput and PRA32_U2_I2SOutput)
//
// - A data DMA channel reads a ring buffer endlessly (a control DMA channel rewinds and retriggers it),
//   paced by the DREQ of the output device, so no IRQs are used
// - The buffer size is set in frames (1 frame = L + R) by setBufferFrames(), and it is the maximum output latency
// - The writer blocks while the buffer is full, and reads the DMA read address only when the cached writable count is 0

#include "pra32-u2-common.h"
#include <hardware/dma.h>

class PRA32_U2_AudioRingBuffer {
  static const uint32_t DEFAULT_BUFFER_FRAMES = 128;  // 2.7 ms at 48 kHz

  uint32_t  m_buffer_frames;
  uint32_t  m_ring_words;
  uint32_t  m_words_per_frame_shift;
  uint32_t* m_ring_start[2];
  int       m_dma_data[2];
  int       m_dma_control[2];
  const volatile uint32_t* m_dma_read_addr;

protected:
  uint32_t  m_number_of_rings;
  uint32_t* m_ring[2];
  uint32_t  m_write_index;  // in words
  uint32_t  m_writable;     // in frames

  PRA32_U2_AudioRingBuffer()
  : m_buffer_frames(DEFAULT_BUFFER_FRAMES)
  , m_ring_words()
  , m_words_per_frame_shift()
  , m_ring_start{}
  , m_dma_data{-1, -1}
  , m_dma_control{-1, -1}
  , m_dma_read_addr()
  , m_number_of_rings()
  , m_ring{}
  , m_write_index()
  , m_writable()
  {}

  // words_per_frame: 1 or 2
  void ring_allocate(uint32_t number_of_rings, uint32_t words_per_frame, uint32_t silence_word) {
    m_number_of_rings = number_of_rings;
    m_words_per_frame_shift = (words_per_frame == 2) ? 1 : 0;
    m_ring_words = m_buffer_frames << m_words_per_frame_shift;

    for (uint32_t k = 0; k < m_number_of_rings; ++k) {
      m_ring[k] = new uint32_t[m_ring_words];
      for (uint32_t i = 0; i < m_ring_words; ++i) {
        m_ring[k][i] = silence_word;
      }
      m_ring_start[k] = m_ring[k];
    }
  }

  // The data channel is not started until ring_start_dma() is called
  void ring_configure_dma(uint32_t k, uint32_t dreq, volatile void* write_addr) {
    m_dma_data[k]    = dma_claim_unused_channel(true);
    m_dma_control[k] = dma_claim_unused_channel(true);

    dma_channel_config data_cfg = dma_channel_get_default_config(m_dma_data[k]);
    channel_config_set_transfer_data_size(&data_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&data_cfg, true);
    channel_config_set_write_increment(&data_cfg, false);
    channel_config_set_dreq(&data_cfg, dreq);
    channel_config_set_chain_to(&data_cfg, m_dma_control[k]);
    dma_channel_configure(m_dma_data[k], &data_cfg, write_addr, m_ring[k], m_ring_words, false);

    // Rewinds the read address of the data channel to the start of the ring and retriggers it
    dma_channel_config control_cfg = dma_channel_get_default_config(m_dma_control[k]);
    channel_config_set_transfer_data_size(&control_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&control_cfg, false);
    channel_config_set_write_increment(&control_cfg, false);
    dma_channel_configure(m_dma_control[k], &control_cfg, &dma_hw->ch[m_dma_data[k]].al3_read_addr_trig,
                          &m_ring_start[k], 1, false);
  }

  void ring_start_dma() {
    for (uint32_t k = 0; k < m_number_of_rings; ++k) {
      dma_channel_start(m_dma_data[k]);  // Waits for the DREQ
    }

    m_dma_read_addr = &dma_hw->ch[m_dma_data[0]].read_addr;
    m_write_index = 0;
    m_writable = 0;
  }

  void ring_free() {
    for (uint32_t k = 0; k < m_number_of_rings; ++k) {
      // Disable chaining first, so that the data channel and the control channel do not retrigger each other
      hw_write_masked(&dma_hw->ch[m_dma_data[k]].al1_ctrl,
                      static_cast<uint32_t>(m_dma_data[k]) << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB,
                      DMA_CH0_CTRL_TRIG_CHAIN_TO_BITS);
      dma_channel_abort(m_dma_control[k]);
      dma_channel_abort(m_dma_data[k]);
      dma_channel_unclaim(m_dma_control[k]);
      dma_channel_unclaim(m_dma_data[k]);
      m_dma_control[k] = -1;
      m_dma_data[k] = -1;

      delete[] m_ring[k];
      m_ring[k] = nullptr;
      m_ring_start[k] = nullptr;
    }
    m_number_of_rings = 0;
  }

  INLINE void ring_wait_for_writable() {
    if (m_writable == 0) {
      wait_for_writable();
    }
  }

  INLINE void ring_advance() {
    m_write_index += 1u << m_words_per_frame_shift;
    if (m_write_index == m_ring_words) {
      m_write_index = 0;
    }
    --m_writable;
  }

public:
  bool setBufferFrames(uint32_t buffer_frames) {
    if ((m_number_of_rings != 0) || (buffer_frames < 8)) {
      return false;
    }
    m_buffer_frames = buffer_frames;
    return true;
  }

private:
  void __not_in_flash_func(wait_for_writable)() {
    // The frames in [read_frame, write_frame) are waiting to be played
    // The frames in [write_frame, read_frame - 1) are writable (1 frame is kept as a gap)
    // A partially read frame is regarded as not read
    uint32_t frames = m_ring_words >> m_words_per_frame_shift;
    uint32_t write_frame = m_write_index >> m_words_per_frame_shift;
    uint32_t writable;
    do {
      uint32_t read_frame = ((*m_dma_read_addr - reinterpret_cast<uintptr_t>(m_ring[0])) >> 2) >> m_words_per_frame_shift;
      if (read_frame >= frames) {
        read_frame -= frames;  // The data channel is being rewound
      }
      writable = read_frame + frames - write_frame - 1;
      if (writable >= frames) {
        writable -= frames;
      }
    } while (writable == 0);
    m_writable = writable;
  }
};
