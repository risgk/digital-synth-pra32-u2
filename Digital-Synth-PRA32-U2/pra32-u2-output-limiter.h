#pragma once

#include "pra32-u2-common.h"

// Look-ahead peak limiter for the final output, followed by soft_clip_output()
// (1.0 = 1 << 23): the output is delayed by 1 ms, and the gain is lowered by
// the time a peak above the threshold arrives, so that loud chords with a high
// Resonance stay below the soft clipping instead of being distorted by it
class PRA32_U2_OutputLimiter {
  static_assert(SAMPLING_RATE == 48000, "the look-ahead time and the rates are for 48 kHz");

  static const uint32_t DELAY_BUFF_SIZE       = 64;
  static const uint32_t LOOK_AHEAD            = 48;  // 1 ms

  // The gain is recalculated once per control interval, and interpolated over
  // the samples in between, as PRA32_U2_Amp does
  static const uint8_t  CONTROL_INTERVAL_BITS = 2;

  // The peak is held over the samples in the delay buffer and the current one:
  // one block per control interval, and one more for the block in progress
  static const uint32_t PEAK_BLOCKS           = (LOOK_AHEAD >> CONTROL_INTERVAL_BITS) + 1;

  // At the knee of soft_clip_output(); the gain comes down over the attack
  // time, so a sudden peak still overshoots it a little, into the knee
  static const int32_t  THRESHOLD             = 6291456;  // 0.75

  // 1 - e^(-1 / n) in Q24, where n is the time constant in control intervals
  // (12 kHz): attack 1 ms, release 100 ms
  static const int32_t  ATTACK_RATE           = 1341432;
  static const int32_t  RELEASE_RATE          = 13975;

  static const int32_t  GAIN_ONE              = 1 << 16;

  int32_t  m_delay_buff[2][DELAY_BUFF_SIZE];
  uint32_t m_delay_wp;
  uint32_t m_count;
  int32_t  m_block_peak;
  int32_t  m_peaks[PEAK_BLOCKS];
  uint32_t m_peak_index;
  int32_t  m_envelope;
  int32_t  m_gain_current;
  int32_t  m_gain_next;
  int32_t  m_gain_step;
  int32_t  m_depth;                    // 0 (Off) .. 128 (full), Q7

public:
  PRA32_U2_OutputLimiter()
  : m_delay_buff()
  , m_delay_wp()
  , m_count()
  , m_block_peak()
  , m_peaks()
  , m_peak_index()
  , m_envelope()
  , m_gain_current(GAIN_ONE)
  , m_gain_next(GAIN_ONE)
  , m_gain_step()
  , m_depth(128)
  {
  }

  // 0: Off (the output is still delayed by 1 ms and clipped), 127: full limiting (default);
  // 2 controller values per step, 65 steps, as the Delay Level
  INLINE void set_depth(uint8_t controller_value) {
    m_depth = ((controller_value + 1) >> 1) << 1;
  }

  INLINE PRA32_U2_StereoSample process(PRA32_U2_StereoSample input_int24) {
    const int32_t left_input_int24  = input_int24.left;
    const int32_t right_input_int24 = input_int24.right;

    int32_t left_abs  = (left_input_int24  < 0) ? -left_input_int24  : left_input_int24;
    int32_t right_abs = (right_input_int24 < 0) ? -right_input_int24 : right_input_int24;
    m_block_peak = maximum(m_block_peak, maximum(left_abs, right_abs));

    m_delay_buff[0][m_delay_wp] = left_input_int24;
    m_delay_buff[1][m_delay_wp] = right_input_int24;
    const uint32_t delay_rp = (m_delay_wp - LOOK_AHEAD) & (DELAY_BUFF_SIZE - 1);
    m_delay_wp = (m_delay_wp + 1) & (DELAY_BUFF_SIZE - 1);

    if ((m_count & ((1 << CONTROL_INTERVAL_BITS) - 1)) == 0) {
      update_gain();
    }
    ++m_count;

    m_gain_current = approach(m_gain_current, m_gain_next, m_gain_step);

    return { soft_clip_output(multiply_shift_right(m_delay_buff[0][delay_rp], m_gain_current, 16)),
             soft_clip_output(multiply_shift_right(m_delay_buff[1][delay_rp], m_gain_current, 16)) };
  }

private:
  INLINE void update_gain() {
    m_peaks[m_peak_index] = m_block_peak;
    m_block_peak = 0;
    m_peak_index = (m_peak_index + 1 == PEAK_BLOCKS) ? 0 : (m_peak_index + 1);

    int32_t peak_hold = 0;
    for (uint32_t i = 0; i < PEAK_BLOCKS; ++i) {
      peak_hold = maximum(peak_hold, m_peaks[i]);
    }

    const int32_t rate = (peak_hold > m_envelope) ? ATTACK_RATE : RELEASE_RATE;
    m_envelope += multiply_shift_right(peak_hold - m_envelope, rate, 24);

    // THRESHOLD / envelope in Q16, from a 32-bit division; the envelope is
    // above the threshold, so its upper bits keep enough precision
    int32_t gain_next = GAIN_ONE;
    if (m_envelope > THRESHOLD) {
      gain_next = static_cast<int32_t>(static_cast<uint32_t>(THRESHOLD << 8) /
                                       static_cast<uint32_t>(m_envelope >> 8));
    }
    // The Depth scales the gain reduction, not the gain
    m_gain_next = GAIN_ONE - (((GAIN_ONE - gain_next) * m_depth) >> 7);

    // Rounded up, so that the target is reached by the end of the interval
    const int32_t delta = m_gain_next - m_gain_current;
    m_gain_step = (maximum(delta, -delta) +
                   ((1 << CONTROL_INTERVAL_BITS) - 1)) >> CONTROL_INTERVAL_BITS;
  }
};
