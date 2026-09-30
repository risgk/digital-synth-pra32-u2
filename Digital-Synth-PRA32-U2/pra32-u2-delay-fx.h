#pragma once

#include "pra32-u2-common.h"

// Reverb: fractional line lengths with linear interpolation (smooth Time changes).
// Comment out to disable (4 fewer reads and multiplies per Reverb step): integer lengths
// step by 1 sample while Time is moving, which is heard as noise.
#define PRA32_U2_REVERB_INTERPOLATION

// Reverb: 4 input APF stages per channel instead of 2 (smoother attack, heavier).
// Uncomment to enable.
// #define PRA32_U2_REVERB_INPUT_APF_4_STAGES

// Reverb: one allpass inside each FDN feedback path (denser, less metallic tail).
// Comment out to disable.
#define PRA32_U2_REVERB_LOOP_APF

#if !defined(PRA32_U2_REVERB_LOOP_APF_LENS)
#define PRA32_U2_REVERB_LOOP_APF_LENS 73, 89, 107, 127  // primes, ~3-5 ms at 24 kHz
#endif
#if !defined(PRA32_U2_REVERB_LOOP_APF_SHIFT2)
#define PRA32_U2_REVERB_LOOP_APF_SHIFT2 0
#endif

// Delay / Reverb effect
//
// Delay Mode:  0 -  63  Stereo Delay
//             64 -  95  Ping Pong Delay
//             96 - 127  Reverb
//
// Reverb algorithm (low cost, no extra RAM: reuses m_delay_buff)
//
//   L in -> Level -> APF x2 (L) --+--> [line 0] --+
//   R in -> Level -> APF x2 (R) --|--> [line 1] --|--> 4x4 Hadamard -> Decay gain -> 1-pole LPF -> loop APF --+
//                                 |    [line 2] --|                                                           |
//                                 |    [line 3] --+                                                           |
//                                 +<------------------------------- feedback ---------------------------------+
//
//   L out = L in + (line 0 + line 3) * 17/16
//   R out = R in + (line 1 - line 2) * 17/16
//
//   - Runs at half the sampling rate (24 kHz), every other sample, to halve its processing
//     time: the input is the average of two samples, and the output is interpolated between
//     the last two results. All the Reverb lengths below are in 24 kHz samples.
//   - Input diffusion: 2 (4 with PRA32_U2_REVERB_INPUT_APF_4_STAGES) series Schroeder allpasses
//     per channel (Dattorro's input diffuser lengths scaled to 24 kHz; R lengths slightly detuned).
//     Gain 0.75/0.75/0.625/0.625 by shifts. Smears the input so the tail starts dense.
//   - Loop diffusion (PRA32_U2_REVERB_LOOP_APF): one short allpass (73/89/107/127, g = 0.5) per
//     feedback path. Lossless, so stability is unchanged; multiplies echo density on every pass
//     and reduces flutter. The input is added after it, so the first reflection stays in place.
//   - FDN: 4 delay lines mixed by an orthonormal Hadamard matrix (add/sub only).
//     Line lengths: line 0 = Delay Time / 3, lines 1-3 = 219/256, 187/256, 157/256 of line 0.
//     Every ring (lines and allpasses) has a power-of-two region and shares one write counter;
//     it is read L samples behind, like the Delay, so changing L always reads valid history.
//   - Parameters, set so that the same Delay parameters give the same impression as the Delay:
//       Delay Time     -> Size  (line 0 = 1/3 of the delay time; default 250 ms -> 83 ms)
//       Delay Feedback -> Decay: by Feedback / 256 over each Delay Time, as the Delay's echoes
//                         (default -> RT60 ~1.25 s); the send is scaled by it too, as the Delay's
//       Delay Level    -> Send level (same as Delay)
//     The wet level is set so that the reverb after the note off is about as loud as the
//     Delay's echoes (the reverb starts earlier, at 1/3 of the Delay Time, so matching the total
//     energy would leave it about 3 dB quieter after the note off, and sound shorter).
//   - Damping: a light 1-pole LPF in each feedback path, so that the high frequencies decay
//     about as fast as the Delay's echoes do.
//   - Switching to or from the Reverb, which uses the buffer differently, fades the wet
//     signal out (1.3 ms), then keeps the reads, the send and the feedback at 0 for one round
//     of the buffer, so that the buffer is written with zeros by the ordinary processing
//     (the processing time stays the same); the send then fades in as its level is smoothed.
//   - Time changes: smoothed like the Delay, and with PRA32_U2_REVERB_INTERPOLATION the line
//     lengths are fractional (Q8) with linear interpolation, so sweeps glide without zipper.
//     Without it, lengths are integers (odd for lines 1-3) and step by 1 sample.

// Smallest power of two >= x (compile time)
static constexpr uint16_t pra32_u2_pow2_ceil(uint32_t x) {
  uint32_t p = 1;
  while (p < x) { p <<= 1; }
  return static_cast<uint16_t>(p);
}

class PRA32_U2_DelayFx {
#if !defined(PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM)
  static const uint16_t DELAY_BUFF_SIZE = 16384;
#else  // !defined(PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM)
  static const uint16_t DELAY_BUFF_SIZE = 8192;
#endif  // !defined(PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM)

  static const int32_t SMOOTH_RATE = 2048;

  // How fast the read positions may move, as the Delay Time is turned: a fast turn of the knob
  // would otherwise move them by many samples per sample, reading the buffer at many times the
  // speed (heard as noise); limited, the pitch of the echoes and of the reverb bends smoothly,
  // as on a tape delay, and the new Delay Time is reached a moment later
  static const int32_t DELAY_TIME_SLEW  = 256 / 4;   // Q8 samples per sample: the pitch within +-25%
  static const int32_t REVERB_LEN_SLEW  = 256 / 32;  // Q8 samples per Reverb step: within +-3%

  static const uint8_t DELAY_MODE_STEREO    = 0;
  static const uint8_t DELAY_MODE_PING_PONG = 1;
  static const uint8_t DELAY_MODE_REVERB    = 2;

  // Switching to or from the Reverb: the wet signal fades out over 16 control intervals, then
  // the buffer is written with zeros for one round of it (in control intervals of 4 samples)
  static const int32_t  WET_GAIN_ONE   = 1 << 16;
  static const int32_t  WET_FADE_STEP  = WET_GAIN_ONE / 16;
  static const uint16_t MUTE_INTERVALS = DELAY_BUFF_SIZE / 4;

  // Reverb mode reuses m_delay_buff. Every ring (FDN line or allpass) has a power-of-two
  // region and is addressed from one shared counter (m_rev_ctr, one step per 2 samples):
  //   write at (ctr & mask), read L samples back at ((ctr - L) & mask)
  //   buff[0] (L side): [line 0][line 3][in APF L0][L1][loop APF 0][loop APF 3][in APF L2][L3]
  //   buff[1] (R side): [line 1][line 2][in APF R0][R1][loop APF 1][loop APF 2][in APF R2][R3]
  //   (in APF L2/L3/R2/R3 only with PRA32_U2_REVERB_INPUT_APF_4_STAGES)
  static const uint16_t REVERB_LEN_MAX_0 = DELAY_BUFF_SIZE / 6 + 1;  // > max Delay Time / 3, at 24 kHz
  static const uint16_t REVERB_LEN_MAX_1 = static_cast<uint16_t>((static_cast<uint32_t>(REVERB_LEN_MAX_0) * 219) >> 8) | 1;
  static const uint16_t REVERB_LEN_MAX_2 = static_cast<uint16_t>((static_cast<uint32_t>(REVERB_LEN_MAX_0) * 187) >> 8) | 1;
  static const uint16_t REVERB_LEN_MAX_3 = static_cast<uint16_t>((static_cast<uint32_t>(REVERB_LEN_MAX_0) * 157) >> 8) | 1;

  // FDN lines
  static constexpr uint8_t  LINE_BUFF[4]   = { 0, 1, 1, 0 };
  static constexpr uint16_t LINE_SIZE[4]   = { pra32_u2_pow2_ceil(REVERB_LEN_MAX_0), pra32_u2_pow2_ceil(REVERB_LEN_MAX_1),
                                               pra32_u2_pow2_ceil(REVERB_LEN_MAX_2), pra32_u2_pow2_ceil(REVERB_LEN_MAX_3) };
  static constexpr uint16_t LINE_OFFSET[4] = { 0, 0, LINE_SIZE[1], LINE_SIZE[0] };

  // Input diffusers (Dattorro's lengths scaled to 24 kHz, primes; R slightly detuned for decorrelation)
  //   gain: 0.75 0.75 0.625 0.625
  static constexpr uint16_t IN_APF_LEN[2][4]  = { { 113, 83, 307, 223 },
                                                  { 109, 89, 311, 227 } };
  // Loop allpasses (one per FDN line, g = 0.5 by default)
  static constexpr uint16_t LOOP_APF_LEN[4]   = { PRA32_U2_REVERB_LOOP_APF_LENS };

  static constexpr uint16_t IN_APF_SIZE[2][4] = {
    { pra32_u2_pow2_ceil(IN_APF_LEN[0][0]), pra32_u2_pow2_ceil(IN_APF_LEN[0][1]), pra32_u2_pow2_ceil(IN_APF_LEN[0][2]), pra32_u2_pow2_ceil(IN_APF_LEN[0][3]) },
    { pra32_u2_pow2_ceil(IN_APF_LEN[1][0]), pra32_u2_pow2_ceil(IN_APF_LEN[1][1]), pra32_u2_pow2_ceil(IN_APF_LEN[1][2]), pra32_u2_pow2_ceil(IN_APF_LEN[1][3]) } };
  static constexpr uint16_t LOOP_APF_SIZE[4]  = { pra32_u2_pow2_ceil(LOOP_APF_LEN[0]), pra32_u2_pow2_ceil(LOOP_APF_LEN[1]),
                                                  pra32_u2_pow2_ceil(LOOP_APF_LEN[2]), pra32_u2_pow2_ceil(LOOP_APF_LEN[3]) };

  static constexpr uint16_t APF_BASE[2] = { static_cast<uint16_t>(LINE_SIZE[0] + LINE_SIZE[3]),
                                            static_cast<uint16_t>(LINE_SIZE[1] + LINE_SIZE[2]) };
  static constexpr uint16_t IN_APF_OFFSET[2][4] = {
    { APF_BASE[0],
      static_cast<uint16_t>(APF_BASE[0] + IN_APF_SIZE[0][0]),
      static_cast<uint16_t>(APF_BASE[0] + IN_APF_SIZE[0][0] + IN_APF_SIZE[0][1] + LOOP_APF_SIZE[0] + LOOP_APF_SIZE[3]),
      static_cast<uint16_t>(APF_BASE[0] + IN_APF_SIZE[0][0] + IN_APF_SIZE[0][1] + LOOP_APF_SIZE[0] + LOOP_APF_SIZE[3] + IN_APF_SIZE[0][2]) },
    { APF_BASE[1],
      static_cast<uint16_t>(APF_BASE[1] + IN_APF_SIZE[1][0]),
      static_cast<uint16_t>(APF_BASE[1] + IN_APF_SIZE[1][0] + IN_APF_SIZE[1][1] + LOOP_APF_SIZE[1] + LOOP_APF_SIZE[2]),
      static_cast<uint16_t>(APF_BASE[1] + IN_APF_SIZE[1][0] + IN_APF_SIZE[1][1] + LOOP_APF_SIZE[1] + LOOP_APF_SIZE[2] + IN_APF_SIZE[1][2]) } };
  static constexpr uint16_t LOOP_APF_OFFSET[4] = {
    static_cast<uint16_t>(APF_BASE[0] + IN_APF_SIZE[0][0] + IN_APF_SIZE[0][1]),
    static_cast<uint16_t>(APF_BASE[1] + IN_APF_SIZE[1][0] + IN_APF_SIZE[1][1]),
    static_cast<uint16_t>(APF_BASE[1] + IN_APF_SIZE[1][0] + IN_APF_SIZE[1][1] + LOOP_APF_SIZE[1]),
    static_cast<uint16_t>(APF_BASE[0] + IN_APF_SIZE[0][0] + IN_APF_SIZE[0][1] + LOOP_APF_SIZE[0]) };

#if defined(PRA32_U2_REVERB_INPUT_APF_4_STAGES)
  static constexpr uint32_t REVERB_END[2] = { IN_APF_OFFSET[0][3] + static_cast<uint32_t>(IN_APF_SIZE[0][3]),
                                              IN_APF_OFFSET[1][3] + static_cast<uint32_t>(IN_APF_SIZE[1][3]) };
#else
  static constexpr uint32_t REVERB_END[2] = { IN_APF_OFFSET[0][2], IN_APF_OFFSET[1][2] };
#endif

  static_assert(REVERB_END[0] <= DELAY_BUFF_SIZE && REVERB_END[1] <= DELAY_BUFF_SIZE, "reverb does not fit in m_delay_buff");
  static_assert(LINE_SIZE[0] <= 0x8000, "m_rev_ctr range");
  // One round of the buffer in Reverb steps (2 samples each) covers every ring
  static_assert(LINE_SIZE[0] <= (MUTE_INTERVALS * 2), "the mute must cover every ring");

  int32_t  m_delay_buff[2][DELAY_BUFF_SIZE];
  uint16_t m_delay_wp[2];

  uint16_t m_delay_level_target;
  uint16_t m_delay_level_current;
  uint8_t  m_delay_feedback_target;
  uint8_t  m_delay_feedback_current;
  int32_t  m_delay_time_target;
  int32_t  m_delay_time_current;
  int32_t  m_delay_time_read;   // Follows m_delay_time_current, at most DELAY_TIME_SLEW per sample
  uint8_t  m_delay_mode;
  uint8_t  m_delay_mode_target;
  int32_t  m_wet_gain;          // Q16
  uint16_t m_mute_count;        // In control intervals

  int32_t  m_lpf_out_0;
  int32_t  m_lpf_out_1;

  int32_t  m_rev_lpf[4];
  uint32_t m_rev_len[4];        // Q8 with PRA32_U2_REVERB_INTERPOLATION, else samples
  uint32_t m_rev_len_target[4]; // Followed at most REVERB_LEN_SLEW per Reverb step, with PRA32_U2_REVERB_INTERPOLATION
  int32_t  m_rev_gain[4];       // Q32, including the 1/2 of the Hadamard matrix
  uint16_t m_rev_ctr;
  uint8_t  m_rev_phase;         // 0: the Reverb is processed at this sample, 1: not
  int32_t  m_rev_send_prev[2];
  int32_t  m_rev_out_prev[2];
  int32_t  m_rev_out[2];

public:
  PRA32_U2_DelayFx()
  : m_delay_buff()
  , m_delay_wp()

  , m_delay_level_target()
  , m_delay_level_current()
  , m_delay_feedback_target()
  , m_delay_feedback_current()
  , m_delay_time_target()
  , m_delay_time_current()
  , m_delay_time_read()
  , m_delay_mode()
  , m_delay_mode_target()
  , m_wet_gain(WET_GAIN_ONE)
  , m_mute_count()

  , m_lpf_out_0()
  , m_lpf_out_1()

  , m_rev_lpf()
  , m_rev_len()
  , m_rev_len_target()
  , m_rev_gain()
  , m_rev_ctr()
  , m_rev_phase()
  , m_rev_send_prev()
  , m_rev_out_prev()
  , m_rev_out()
  {
    m_delay_wp[0] = DELAY_BUFF_SIZE - 1;
    m_delay_wp[1] = DELAY_BUFF_SIZE - 1;

    set_delay_level   (0  );
    set_delay_feedback(64 );
    set_delay_time    (87 );

    m_delay_time_current = m_delay_time_target;
    m_delay_time_read = m_delay_time_current;
    update_reverb_len();
    for (uint32_t i = 0; i < 4; ++i) {
      m_rev_len[i] = m_rev_len_target[i];
    }
  }

  INLINE void set_delay_level(uint8_t controller_value) {
    m_delay_level_target = ((controller_value + 1) >> 1) << 1;
  }

  INLINE void set_delay_feedback(uint8_t controller_value) {
    m_delay_feedback_target = controller_value;
  }

  INLINE void set_delay_time(uint8_t controller_value) {
#if !defined(PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM)
    static uint16_t delay_time_table[128] = {
         48,    96,   144,   192,   240,   288,   384,   480,
        576,   672,   768,   864,   960,  1056,  1152,  1248,
       1344,  1440,  1536,  1632,  1728,  1824,  1920,  2016,
       2112,  2208,  2304,  2400,  2560,  2720,  2880,  3040,
       3200,  3360,  3520,  3680,  3840,  4000,  4160,  4320,
       4480,  4640,  4800,  4960,  5120,  5280,  5440,  5600,
       5760,  5920,  6080,  6240,  6400,  6560,  6720,  6880,
       7040,  7200,  7360,  7520,  7680,  7840,  8000,  8160,
       8320,  8480,  8640,  8800,  8960,  9120,  9280,  9440,
       9600,  9760,  9920, 10080, 10240, 10400, 10560, 10720,
      10880, 11040, 11200, 11360, 11520, 11680, 11840, 12000,
      12160, 12320, 12480, 12640, 12800, 12960, 13120, 13280,
      13440, 13600, 13760, 13920, 14080, 14240, 14400, 14560,
      14720, 14880, 15040, 15200, 15360, 15520, 15680, 15840,
      16160, 16320, 16320, 16320, 16320, 16320, 16320, 16320,
      16320, 16320, 16320, 16320, 16320, 16320, 16320, 16320,
    };
#else  // !defined(PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM)
    static uint16_t delay_time_table[128] = {
         48,    96,   144,   192,   240,   288,   384,   480,
        576,   672,   768,   864,   960,  1056,  1152,  1248,
       1344,  1440,  1536,  1632,  1728,  1824,  1920,  2016,
       2112,  2208,  2304,  2400,  2560,  2720,  2880,  3040,
       3200,  3360,  3520,  3680,  3840,  4000,  4160,  4320,
       4480,  4640,  4800,  4960,  5120,  5280,  5440,  5600,
       5760,  5920,  6080,  6240,  6400,  6560,  6720,  6880,
       7040,  7200,  7360,  7520,  7680,  7840,  8000,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
       8160,  8160,  8160,  8160,  8160,  8160,  8160,  8160,
    };
#endif  // !defined(PRA32_U2_LIMIT_DELAY_TIME_TO_SAVE_MEM)

    m_delay_time_target = delay_time_table[controller_value] << 8;
  }

  INLINE void set_delay_mode(uint8_t controller_value) {
    m_delay_mode_target = (controller_value >= 96) ? DELAY_MODE_REVERB :
                          (controller_value >= 64) ? DELAY_MODE_PING_PONG : DELAY_MODE_STEREO;
  }

  INLINE void process_at_low_rate(uint8_t count) {
    m_delay_level_current = approach_exp(m_delay_level_current, m_delay_level_target, SMOOTH_RATE);
    m_delay_feedback_current = approach_exp(m_delay_feedback_current, m_delay_feedback_target, SMOOTH_RATE);

    const int32_t is_even = (count & 0x01) ^ 1;
    const int32_t next_approach_val = approach_exp_wide(m_delay_time_current, m_delay_time_target, SMOOTH_RATE);
    m_delay_time_current = (next_approach_val * is_even) + (m_delay_time_current * (is_even ^ 1));

    if (m_delay_mode != m_delay_mode_target) {
      if ((m_delay_mode != DELAY_MODE_REVERB) && (m_delay_mode_target != DELAY_MODE_REVERB)) {
        // The Stereo and the Ping Pong share the buffer as it is
        m_delay_mode = m_delay_mode_target;
      } else {
        m_wet_gain = maximum(m_wet_gain - WET_FADE_STEP, 0);
        if (m_wet_gain == 0) {
          m_delay_mode = m_delay_mode_target;
          m_mute_count = MUTE_INTERVALS;
        }
      }
    }

    if (m_mute_count > 0) {
      --m_mute_count;
      m_delay_level_current = 0;
    } else if (m_delay_mode == m_delay_mode_target) {
      m_wet_gain = WET_GAIN_ONE;
    }

    if (m_delay_mode == DELAY_MODE_REVERB) {
      // Recalculated every time, even when the delay time is not moving, so that the
      // processing time stays the same; the gain of one line at a time
      update_reverb_len();
      update_reverb_gain(count & 0x03);
    }
  }

  INLINE PRA32_U2_StereoSample process(PRA32_U2_StereoSample input_int24) {
    if (m_delay_mode == DELAY_MODE_REVERB) {
      return process_reverb(input_int24);
    }

    const int32_t left_input_int24  = input_int24.left;
    const int32_t right_input_int24 = input_int24.right;

    m_delay_time_read = approach(m_delay_time_read, m_delay_time_current, DELAY_TIME_SLEW);

    int32_t left_delay   = multiply_shift_right(delay_buff_get<0>(m_delay_time_read), m_wet_gain, 16);
    int32_t right_delay  = multiply_shift_right(delay_buff_get<1>(m_delay_time_read), m_wet_gain, 16);

    int32_t left_feedback;
    int32_t right_feedback;

    int32_t left_send  = multiply_shift_right(left_input_int24,  m_delay_level_current << 1, 8);
    int32_t right_send = multiply_shift_right(right_input_int24, m_delay_level_current << 1, 8);

    const int32_t left_final_in  = (m_delay_mode == DELAY_MODE_PING_PONG) ? (((left_send + right_send) >> 1) + right_delay)
                                                                          : (left_send  + left_delay);
    const int32_t right_final_in = (m_delay_mode == DELAY_MODE_PING_PONG) ? (left_delay)
                                                                          : (right_send + right_delay);
    const int32_t feedback_gain = m_delay_feedback_current << 8;
    left_feedback  = multiply_shift_right(left_final_in,  feedback_gain, 16);
    right_feedback = multiply_shift_right(right_final_in, feedback_gain, 16);

    int32_t curr_sample_to_push_0 = left_feedback;
    int32_t curr_sample_to_push_1 = right_feedback;

#if 0
    // Do not apply LPF to the delay component
    m_lpf_out_0 = curr_sample_to_push_0;
    m_lpf_out_1 = curr_sample_to_push_1;
#endif

    m_lpf_out_0 = curr_sample_to_push_0 - ((curr_sample_to_push_0 - m_lpf_out_0) >> 2);
    m_lpf_out_1 = curr_sample_to_push_1 - ((curr_sample_to_push_1 - m_lpf_out_1) >> 2);

    delay_buff_push<0>(m_lpf_out_0);
    delay_buff_push<1>(m_lpf_out_1);

    return { left_input_int24  + left_delay,
             right_input_int24 + right_delay };
  }

private:
  // Line 0 length = Delay Time / 3 (the first reflection is a triplet of the delay), in 24 kHz samples.
  // With PRA32_U2_REVERB_INTERPOLATION, the lengths follow these targets at most REVERB_LEN_SLEW per
  // Reverb step
  INLINE void update_reverb_len() {
    uint32_t len_q8 = static_cast<uint32_t>(m_delay_time_current) / 6;  // a 32-bit division, low rate only
    if (len_q8 > ((REVERB_LEN_MAX_0 - 1) << 8)) { len_q8 = (REVERB_LEN_MAX_0 - 1) << 8; }
    if (len_q8 < (2 << 8))                      { len_q8 = (2 << 8); }  // keep lines 1-3 >= 1 sample

#if defined(PRA32_U2_REVERB_INTERPOLATION)
    m_rev_len_target[0] = len_q8;
    m_rev_len_target[1] = (len_q8 * 219) >> 8;
    m_rev_len_target[2] = (len_q8 * 187) >> 8;
    m_rev_len_target[3] = (len_q8 * 157) >> 8;
#else  // defined(PRA32_U2_REVERB_INTERPOLATION)
    const uint32_t len = (len_q8 + 128) >> 8;
    m_rev_len[0] = m_rev_len_target[0] = len;
    m_rev_len[1] = m_rev_len_target[1] = ((len * 219) >> 8) | 1;
    m_rev_len[2] = m_rev_len_target[2] = ((len * 187) >> 8) | 1;
    m_rev_len[3] = m_rev_len_target[3] = ((len * 157) >> 8) | 1;
#endif  // defined(PRA32_U2_REVERB_INTERPOLATION)
  }

  // The Reverb decays as the Delay does: by the Delay Feedback / 256 over each Delay Time,
  // i.e. each line's gain is (Feedback / 256) ^ (loop length / Delay Time)
  INLINE void update_reverb_gain(uint32_t i) {
    // log2(Feedback / 256) in Q11; log2(0) is taken as -16
    static const int16_t REVERB_LOG2_FEEDBACK[128] = {
      -32768, -16384, -14336, -13138, -12288, -11629, -11090, -10635,
      -10240,  -9892,  -9581,  -9299,  -9042,  -8805,  -8587,  -8383,
       -8192,  -8013,  -7844,  -7684,  -7533,  -7389,  -7251,  -7120,
       -6994,  -6873,  -6757,  -6646,  -6539,  -6435,  -6335,  -6238,
       -6144,  -6053,  -5965,  -5879,  -5796,  -5715,  -5636,  -5559,
       -5485,  -5412,  -5341,  -5271,  -5203,  -5137,  -5072,  -5008,
       -4946,  -4885,  -4825,  -4767,  -4709,  -4653,  -4598,  -4544,
       -4491,  -4438,  -4387,  -4336,  -4287,  -4238,  -4190,  -4143,
       -4096,  -4050,  -4005,  -3961,  -3917,  -3874,  -3831,  -3789,
       -3748,  -3707,  -3667,  -3627,  -3588,  -3550,  -3511,  -3474,
       -3437,  -3400,  -3364,  -3328,  -3293,  -3258,  -3223,  -3189,
       -3155,  -3122,  -3089,  -3056,  -3024,  -2992,  -2960,  -2929,
       -2898,  -2867,  -2837,  -2807,  -2777,  -2748,  -2719,  -2690,
       -2661,  -2633,  -2605,  -2577,  -2550,  -2523,  -2496,  -2469,
       -2443,  -2416,  -2390,  -2364,  -2339,  -2313,  -2288,  -2263,
       -2239,  -2214,  -2190,  -2166,  -2142,  -2118,  -2095,  -2071,
    };

#if defined(PRA32_U2_REVERB_LOOP_APF)
    static const uint16_t LOOP_EXTRA[4] = { LOOP_APF_LEN[0], LOOP_APF_LEN[1], LOOP_APF_LEN[2], LOOP_APF_LEN[3] };
#else
    static const uint16_t LOOP_EXTRA[4] = { 0, 0, 0, 0 };
#endif

#if defined(PRA32_U2_REVERB_INTERPOLATION)
    const uint32_t line_len      = m_rev_len[i] >> 8;
#else
    const uint32_t line_len      = m_rev_len[i];
#endif
    const int32_t  log2_feedback = REVERB_LOG2_FEEDBACK[m_delay_feedback_current];
    const uint32_t delay_time    = static_cast<uint32_t>(m_delay_time_current) >> 8;   // 48 .. 16320
    const uint32_t loop_len      = (line_len + LOOP_EXTRA[i]) << 1;                     // In 48 kHz samples
    const int32_t  ratio_q16     = static_cast<int32_t>((loop_len << 16) / delay_time); // A 32-bit division
    const int32_t  exponent_q16  = maximum(multiply_shift_right(log2_feedback, ratio_q16, 11), -(16 << 16));

    // 2^exponent: 2^f ~= 1 + f * (0.6565 + f * 0.3435) for the fraction f
    // (within 0.3%), shifted by the integer part (-16 .. -1)
    const uint32_t fraction = exponent_q16 & 0xFFFF;
    const uint32_t mantissa = 65536 + ((fraction * (43025 + ((fraction * 22512) >> 16))) >> 16);
    const int32_t  gain_q16 = mantissa >> (-(exponent_q16 >> 16));

    // Halved for the Hadamard matrix: at most 32768 << 15, within 31 bits; 0 while the buffer
    // is being cleared
    m_rev_gain[i] = (m_mute_count > 0) ? 0 : (gain_q16 << 15);
  }

  // Ring access from the shared counter (all sizes are powers of two, dividing 65536)
  template <uint8_t BUFF, uint16_t OFFSET, uint16_t SIZE>
  INLINE int32_t ring_get(int32_t back) const {
    return m_delay_buff[BUFF][OFFSET + ((m_rev_ctr - back) & (SIZE - 1))];
  }

  template <uint8_t BUFF, uint16_t OFFSET, uint16_t SIZE>
  INLINE void ring_put(int32_t value) {
    m_delay_buff[BUFF][OFFSET + (m_rev_ctr & (SIZE - 1))] = value;
  }

  // Schroeder allpass: w = x + g*w[-M], y = w[-M] - g*w  (g = 1/2 + 1/2^SHIFT2, by shifts only)
  template <uint8_t BUFF, uint16_t OFFSET, uint16_t SIZE, uint16_t LEN, uint8_t SHIFT2>
  INLINE int32_t allpass(int32_t x) {
    const int32_t delayed = ring_get<BUFF, OFFSET, SIZE>(LEN);
    const int32_t w = x + (delayed >> 1) + (SHIFT2 ? (delayed >> SHIFT2) : 0);
    ring_put<BUFF, OFFSET, SIZE>(w);
    return delayed - ((w >> 1) + (SHIFT2 ? (w >> SHIFT2) : 0));
  }

  // Input allpass N of channel CH (L in buff[0], R in buff[1]); g = 0.75, 0.75, 0.625, 0.625
  template <uint8_t CH, uint8_t N>
  INLINE int32_t apf(int32_t x) {
    return allpass<CH, IN_APF_OFFSET[CH][N], IN_APF_SIZE[CH][N], IN_APF_LEN[CH][N], (N <= 1) ? 2 : 3>(x);
  }

  // Loop allpass of FDN line LINE
  template <uint8_t LINE>
  INLINE int32_t loop_apf(int32_t x) {
    return allpass<LINE_BUFF[LINE], LOOP_APF_OFFSET[LINE], LOOP_APF_SIZE[LINE], LOOP_APF_LEN[LINE],
                   PRA32_U2_REVERB_LOOP_APF_SHIFT2>(x);
  }

  // FDN line: read L samples back (L = m_rev_len[LINE])
  template <uint8_t LINE>
  INLINE int32_t rev_read() const {
#if defined(PRA32_U2_REVERB_INTERPOLATION)
    const uint32_t len_q8     = m_rev_len[LINE];
    const int32_t  back       = static_cast<int32_t>(len_q8 >> 8);
    const int32_t  curr_data  = ring_get<LINE_BUFF[LINE], LINE_OFFSET[LINE], LINE_SIZE[LINE]>(back);
    const int32_t  next_data  = ring_get<LINE_BUFF[LINE], LINE_OFFSET[LINE], LINE_SIZE[LINE]>(back + 1);  // one sample older
    const int32_t next_weight = len_q8 & 0xFF;

    // lerp (same as Delay)
    return curr_data + multiply_shift_right(next_data - curr_data, next_weight << 8, 16);
#else  // defined(PRA32_U2_REVERB_INTERPOLATION)
    return ring_get<LINE_BUFF[LINE], LINE_OFFSET[LINE], LINE_SIZE[LINE]>(static_cast<int32_t>(m_rev_len[LINE]));
#endif  // defined(PRA32_U2_REVERB_INTERPOLATION)
  }

  template <uint8_t LINE>
  INLINE void rev_write(int32_t value) {
    ring_put<LINE_BUFF[LINE], LINE_OFFSET[LINE], LINE_SIZE[LINE]>(value);
  }

  INLINE PRA32_U2_StereoSample process_reverb(PRA32_U2_StereoSample input_int24) {
    const int32_t left_input_int24  = input_int24.left;
    const int32_t right_input_int24 = input_int24.right;

    // The send level, scaled by the Delay Feedback / 256 too, as the Delay's input is (Q16)
    const int32_t send_gain  = (m_delay_level_current * m_delay_feedback_current) << 1;
    const int32_t left_send  = multiply_shift_right(left_input_int24,  send_gain, 16);
    const int32_t right_send = multiply_shift_right(right_input_int24, send_gain, 16);

    // Every other sample: the input is the average of two samples, and the output is
    // interpolated between the last two results, half a sample late
    int32_t left_reverb;
    int32_t right_reverb;
    if (m_rev_phase == 0) {
      m_rev_out_prev[0] = m_rev_out[0];
      m_rev_out_prev[1] = m_rev_out[1];
      process_reverb_at_half_rate((m_rev_send_prev[0] + left_send) >> 1, (m_rev_send_prev[1] + right_send) >> 1);
      left_reverb  = (m_rev_out_prev[0] + m_rev_out[0]) >> 1;
      right_reverb = (m_rev_out_prev[1] + m_rev_out[1]) >> 1;
    } else {
      m_rev_send_prev[0] = left_send;
      m_rev_send_prev[1] = right_send;
      left_reverb  = m_rev_out[0];
      right_reverb = m_rev_out[1];
    }
    m_rev_phase ^= 1;

    return { left_input_int24  + left_reverb,
             right_input_int24 + right_reverb };
  }

  INLINE void process_reverb_at_half_rate(int32_t left_send, int32_t right_send) {
    ++m_rev_ctr;  // one shared write position for every ring

#if defined(PRA32_U2_REVERB_INTERPOLATION)
    for (uint32_t i = 0; i < 4; ++i) {
      m_rev_len[i] = approach(m_rev_len[i], m_rev_len_target[i], REVERB_LEN_SLEW);
    }
#endif  // defined(PRA32_U2_REVERB_INTERPOLATION)

    const int32_t x0 = rev_read<0>();
    const int32_t x1 = rev_read<1>();
    const int32_t x2 = rev_read<2>();
    const int32_t x3 = rev_read<3>();

    // 4x4 Hadamard / 2 (orthonormal) and the decay of each line, each product taken as the
    // upper 32 bits (a single instruction); the 1/2 is in m_rev_gain
    const int32_t a = x0 + x1;
    const int32_t b = x0 - x1;
    const int32_t c = x2 + x3;
    const int32_t d = x2 - x3;
    const int32_t h0 = multiply_shift_right(a + c, m_rev_gain[0], 32);
    const int32_t h1 = multiply_shift_right(b + d, m_rev_gain[1], 32);
    const int32_t h2 = multiply_shift_right(a - c, m_rev_gain[2], 32);
    const int32_t h3 = multiply_shift_right(b - d, m_rev_gain[3], 32);

    // Damping (1-pole LPF, pole 1/16): light, as the loop passes it 3 to 5 times per Delay Time,
    // so that the high frequencies decay about as fast as the Delay's echoes do
    m_rev_lpf[0] += ((h0 - m_rev_lpf[0]) * 15) >> 4;
    m_rev_lpf[1] += ((h1 - m_rev_lpf[1]) * 15) >> 4;
    m_rev_lpf[2] += ((h2 - m_rev_lpf[2]) * 15) >> 4;
    m_rev_lpf[3] += ((h3 - m_rev_lpf[3]) * 15) >> 4;

    // Stereo send -> input diffusion (L -> line 0 -> L out, R -> line 1 -> R out)
    left_send  = apf<0, 0>(left_send);
    left_send  = apf<0, 1>(left_send);
    right_send = apf<1, 0>(right_send);
    right_send = apf<1, 1>(right_send);
#if defined(PRA32_U2_REVERB_INPUT_APF_4_STAGES)
    left_send  = apf<0, 2>(left_send);
    left_send  = apf<0, 3>(left_send);
    right_send = apf<1, 2>(right_send);
    right_send = apf<1, 3>(right_send);
#endif  // defined(PRA32_U2_REVERB_INPUT_APF_4_STAGES)

    int32_t fb[4];
#if defined(PRA32_U2_REVERB_LOOP_APF)
    // Diffusion inside the loop (allpass: lossless, does not affect stability).
    // The input is added after it, so the first reflection stays at line 0's length.
    fb[0] = loop_apf<0>(m_rev_lpf[0]);
    fb[1] = loop_apf<1>(m_rev_lpf[1]);
    fb[2] = loop_apf<2>(m_rev_lpf[2]);
    fb[3] = loop_apf<3>(m_rev_lpf[3]);
#else
    fb[0] = m_rev_lpf[0];
    fb[1] = m_rev_lpf[1];
    fb[2] = m_rev_lpf[2];
    fb[3] = m_rev_lpf[3];
#endif

    rev_write<0>(fb[0] + left_send);
    rev_write<1>(fb[1] + right_send);
    rev_write<2>(fb[2]);
    rev_write<3>(fb[3]);

    // Wet: scaled so that the reverb after the note off is about as loud as the Delay's echoes
    // with the same Delay parameters, and faded by m_wet_gain when switching the mode
    m_rev_out[0] = multiply_shift_right(((x0 + x3) * 17) >> 4, m_wet_gain, 16);
    m_rev_out[1] = multiply_shift_right(((x1 - x2) * 17) >> 4, m_wet_gain, 16);
  }

  template <uint8_t N>
  INLINE void delay_buff_push(int32_t audio_input) {
    m_delay_wp[N] = (m_delay_wp[N] + 1) & (DELAY_BUFF_SIZE - 1);
    m_delay_buff[N][m_delay_wp[N]] = audio_input;
  }

  template <uint8_t N>
  INLINE int32_t delay_buff_get(int32_t sample_delay) {
    uint16_t curr_index  = (m_delay_wp[N] - (sample_delay >> 8)) & (DELAY_BUFF_SIZE - 1);
    uint16_t next_index  = (curr_index - 1) & (DELAY_BUFF_SIZE - 1);
    int32_t  next_weight = sample_delay & 0xFF;
    int32_t  curr_data   = m_delay_buff[N][curr_index];
    int32_t  next_data   = m_delay_buff[N][next_index];

    // lerp
    return curr_data + multiply_shift_right(next_data - curr_data, next_weight << 8, 16);
  }
};
