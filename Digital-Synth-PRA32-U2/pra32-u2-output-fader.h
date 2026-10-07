#pragma once

// Output Fader (for writing the user programs to the flash with less noise)
//
// - write_parameters_to_program() of the synth sets g_eeprom_commit_requested (EEPROM.write() is done immediately)
// - begin_block() (once per loop) fades out the output (>= 3 ms), and then does the following at once (the output is silent):
//   - Writes the silence to the whole ring buffer, because the DMA keeps playing it while the flash is being written
//   - (I2S DAC with PRA32_U2_I2S_DAC_MUTE_OFF_PIN only) XSMT = LOW, and waits for the soft mute (10 ms)
//   - EEPROM.commit() (about 50-100 ms)
//   - (I2S DAC with PRA32_U2_I2S_DAC_MUTE_OFF_PIN only) XSMT = HIGH, and waits for the soft unmute (10 ms)
//   and then fades in the output (>= 3 ms)
// - next_gain_q8() (once per frame) returns the gain (256 = 1.0), so (* gain) can be used instead of (<< 8)
//   - It is always calculated in the same way, so that the processing time does not change
//   - The gain reaches exactly 0 or 256 at the end of the fade (clamped)
// - Any buffer settings work (the lengths are calculated from PRA32_U2_I2S_BUFFER_WORDS and the actual ring buffer size)

#include "pra32-u2-common.h"
#include "pra32-u2-audio-ring-buffer.h"

#if defined(ARDUINO_ARCH_RP2040) && defined(PRA32_U2_USE_EMULATED_EEPROM)
#include <EEPROM.h>
#endif  // defined(ARDUINO_ARCH_RP2040) && defined(PRA32_U2_USE_EMULATED_EEPROM)

class PRA32_U2_OutputFader {
  static const uint32_t FADE_BLOCKS     = ((SAMPLING_RATE * 3 / 1000) + PRA32_U2_I2S_BUFFER_WORDS - 1) / PRA32_U2_I2S_BUFFER_WORDS;
  static const int32_t  FADE_DELTA_Q16  = ((1 << 16) + (FADE_BLOCKS * PRA32_U2_I2S_BUFFER_WORDS) - 1) / (FADE_BLOCKS * PRA32_U2_I2S_BUFFER_WORDS);
  static const uint32_t DAC_MUTE_FRAMES = SAMPLING_RATE * 10 / 1000;

  static_assert(FADE_BLOCKS * PRA32_U2_I2S_BUFFER_WORDS <= (1 << 16), "FADE_DELTA_Q16 must be 1 or more");

  PRA32_U2_AudioRingBuffer& m_output;
  uint32_t                  m_fade_blocks;      // Remaining blocks of the fade after the current block
  int32_t                   m_gain_q16;         // 1.0 = 1 << 16
  int32_t                   m_gain_delta_q16;   // Per frame (< 0: fading out, > 0: fading in)

public:
  PRA32_U2_OutputFader(PRA32_U2_AudioRingBuffer& output)
  : m_output(output)
  , m_fade_blocks()
  , m_gain_q16(1 << 16)
  , m_gain_delta_q16()
  {}

  INLINE void begin_block() {
    if (m_fade_blocks != 0) {
      --m_fade_blocks;
    } else if (m_gain_delta_q16 < 0) {
      // The fade-out is done
      commit_eeprom();
      start_fade(FADE_DELTA_Q16);
    } else {
      // The fade-in is done, or not fading
      m_gain_delta_q16 = 0;
      if (g_eeprom_commit_requested) {
        start_fade(-FADE_DELTA_Q16);
      }
    }
  }

  INLINE int32_t next_gain_q8() {
    m_gain_q16 = clamp(m_gain_q16 + m_gain_delta_q16, 0, 1 << 16);
    return m_gain_q16 >> 8;
  }

private:
  INLINE void start_fade(int32_t gain_delta_q16) {
    m_gain_delta_q16 = gain_delta_q16;
    m_fade_blocks = FADE_BLOCKS - 1;
  }

  void write_silence(uint32_t frames) {
    for (uint32_t i = 0; i < frames; ++i) {
      m_output.writeSilence();
    }
  }

  void commit_eeprom() {
    // The whole ring buffer + 8 frames (more than the I2S TX FIFO, 4 frames)
    write_silence(m_output.getBufferFrames() + 8);

#if !defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S) && defined(PRA32_U2_I2S_DAC_MUTE_OFF_PIN)
    digitalWrite(PRA32_U2_I2S_DAC_MUTE_OFF_PIN, LOW);
    write_silence(DAC_MUTE_FRAMES);
#endif

    // The requests until here are included (EEPROM.write() has already been done)
    g_eeprom_commit_requested = false;
#if defined(ARDUINO_ARCH_RP2040) && defined(PRA32_U2_USE_EMULATED_EEPROM)
    EEPROM.commit();
#endif  // defined(ARDUINO_ARCH_RP2040) && defined(PRA32_U2_USE_EMULATED_EEPROM)
    m_output.resync();

#if !defined(PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S) && defined(PRA32_U2_I2S_DAC_MUTE_OFF_PIN)
    digitalWrite(PRA32_U2_I2S_DAC_MUTE_OFF_PIN, HIGH);
    write_silence(DAC_MUTE_FRAMES);
#endif
  }
};
