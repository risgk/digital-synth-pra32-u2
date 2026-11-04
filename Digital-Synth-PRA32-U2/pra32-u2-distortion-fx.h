#pragma once

#include "pra32-u2-common.h"
#include "pra32-u2-filter.h"  // g_filter_drive_gain_table and interpolate_filter_table()

// Distortion, at the head of the FX: the input of the Distortion bus (FX Routing: Dst) is driven
// by D = 2^(value / 32), x 1 at 0 to x 15.7 at 127 (6 dB every 32 values, as the Filter Drive),
// into a curve that is a straight line up to the knee, 1.0 (1 << 23), and a quadratic shoulder
// from there that lands on 2.0 with zero slope at 3.0 (below zero, a shorter one, to -1.5 at -2.0,
// so that it adds even harmonics too, with its DC taken off), and is taken back down by D^(-15/16):
// 15/16 of the drive in dB, so that the level stays within about 1.5 dB as the drive goes up (a little
// up halfway, and a little down where it saturates hard). What comes out goes on to the Chorus, with
// the Chorus bus.
// The curve's part beyond the straight line, f(u) - u, goes through first-order antiderivative
// anti-aliasing (ADAA): the difference of its antiderivative over the step from the last sample,
// divided by the step. On the straight line that part is zero, so a sample that reaches no
// shoulder, now or a sample ago, passes exactly, neither late nor rounded off in the highs; past
// the knee it is half a sample late against the straight part. Below 0.001 of a step, where the
// difference loses its precision, the part is taken at the sample itself instead. At x 1, the
// Presets' and that of a user program written before, a synth below 1.0 passes as it did.
// After the curve, a 1-pole low pass (the tone) takes the edge off the harmonics, more with more
// drive: none at 0, about 10 kHz at 64 and 5 kHz at 127, as many overdrives have after their clip.
// In fixed point (int32, the curve's unit 1.0 as 1 << 23, the int24 sample's), so that it needs no
// FPU: the products as 64-bit ones shifted back (multiply_shift_right()), and the one division of
// the antiderivatives a 64-bit by 32-bit one. The driven sample is held within +-16.0, where the
// curve has long been flat, so that the antiderivative stays within int32 (under 100.0, 2^29.7).
// The same operations for every sample, with no branch (the sign and the short step are weights
// of 1 or 0, and shifts by them)
class PRA32_U2_DistortionFx {
  static const int32_t ONE = 1 << 23;  // The curve's unit, as the int24 sample's full scale

  uint8_t m_distortion_target;
  int32_t m_distortion_stage_1;
  int32_t m_distortion_current;        // Q16 controller value, smoothed
  int32_t m_drive;                     // D, Q16
  int32_t m_make_up;                   // D^(-15/16), Q16 (1 << 16 at x 1)
  int32_t m_tone_rest;                 // The weight of the tone's last output, Q30, 0 at x 1
  int32_t m_tone;                      // (1 << 30) - m_tone_rest, the weight of its input
  int32_t m_last_u[2];
  int32_t m_last_antiderivative[2];
  int32_t m_tone_out[2];
  int32_t m_dc[2];

public:
  PRA32_U2_DistortionFx()
  : m_distortion_target()
  , m_distortion_stage_1()
  , m_distortion_current()
  , m_drive(1 << 16)
  , m_make_up(1 << 16)
  , m_tone_rest()
  , m_tone(1 << 30)
  , m_last_u()
  , m_last_antiderivative()
  , m_tone_out()
  , m_dc()
  {
    set_distortion(0);
  }

  INLINE void set_distortion(uint8_t controller_value) {
    m_distortion_target = controller_value;
  }

  // The drive and its make-up from the smoothed value, every time whatever the value: D^(15/16) is
  // 2^(value * 15/16 / 32), the drive's own table at 15/16 of the value, so that no power is taken
  INLINE void process_at_low_rate(uint8_t count) {
    if (is_slow_smoothing_period(count)) {
      approach_exp_slow(m_distortion_stage_1, m_distortion_current, static_cast<int32_t>(m_distortion_target) << 16);
    }
    m_drive = interpolate_filter_table(g_filter_drive_gain_table, m_distortion_current);
    const int32_t drive_15_16 = interpolate_filter_table(g_filter_drive_gain_table, (m_distortion_current * 15) >> 4);
    // 2^32 / D^(15/16) in Q16, by a 32-bit division (exact at x 1, 1 << 16; about 11 bits at the top)
    m_make_up = static_cast<int32_t>(((static_cast<uint32_t>(1) << 31) / static_cast<uint32_t>(drive_15_16)) << 1);
    // The tone: a 1-pole low pass after the curve, from none at 0 (the weight of its input 1.0) down
    // to about 10 kHz at 64 and 5 kHz at 127 (0.52 of its last output), against the harsh highs
    // of the harmonics: 67 = 0.52 * 2^30 / (127 << 16)
    m_tone_rest = m_distortion_current * 67;
    m_tone      = (1 << 30) - m_tone_rest;
  }

  INLINE PRA32_U2_StereoSample process(PRA32_U2_StereoSample input_int24) {
    return { process_channel<0>(input_int24.left), process_channel<1>(input_int24.right) };
  }

private:
  template <uint8_t N>
  INLINE int32_t process_channel(int32_t input_int24) {
    // The driven sample (up to about +-64.0 with 4 synths at full scale), and it held within +-16.0
    const int32_t driven = multiply_shift_right(input_int24, m_drive, 16);
    const int32_t u = clamp(driven, -16 * ONE, 16 * ONE);
    const int32_t a = std::abs(u);
    // The shoulder, of the span s from the knee 1.0: 2.0 above zero (to 2.0 at 3.0), and 1.0 below
    // (to -1.5 at -2.0), selected by the sign as a weight of 1 or 0
    const int32_t is_negative = static_cast<uint32_t>(u) >> 31;
    const int32_t ceiling     = (3 * ONE) - (is_negative * ONE);                       // 1 + s
    const int32_t e = clamp(a, ONE, ceiling) - ONE;
    const int32_t d = maximum(a - ceiling, 0);
    // f(u) - u: zero up to the knee, -(a - 1)^2 / (2s) over the shoulder (1/4 or 1/2 of the square),
    // -(s / 2) - d past it, with the sign of -u (-1 above zero and 1 below, from the same weight)
    const int32_t curved = multiply_shift_right(e, e, 23 + 2 - is_negative);
    const int32_t part   = (curved + d) * ((2 * is_negative) - 1);
    // Its antiderivative from 0 (of each side: at zero, both are zero): -(a - 1)^3 / (6s) (a third
    // of curved * e), -(s / 2) * d (d or d / 2), and -d^2 / 2
    const int32_t antiderivative = - multiply_shift_right(multiply_shift_right(curved, e, 23), 1431655766, 32)  // 1431655766 = 2^32 / 3
                                   - (d - ((d >> 1) * is_negative))
                                   - multiply_shift_right(d, d, 24);
    const int32_t step     = u - m_last_u[N];
    const int32_t is_short = (std::abs(step) < (ONE / 1000));
    // The difference of the antiderivatives over the step, in the curve's unit (one more shift by 23),
    // or the part at the sample for a short step (for which the divisor is moved by 1.0, away from zero)
    const int32_t difference = antiderivative - m_last_antiderivative[N];
    const int32_t r = (part * is_short) + (divide_in_unit(difference, step + (is_short << 23)) * (1 - is_short));
    m_last_u[N] = u;
    m_last_antiderivative[N] = antiderivative;
    // The part against the driven sample itself (what the hold within +-16.0 took off too), so that
    // the straight line is the driven sample, whose mean is that of the input
    const int32_t whole_part = r + (u - driven);
    // The DC that the curve, not symmetric, puts into its part is taken off: its 1-pole low pass
    // (about 3.7 Hz, 2^-11, a shift) taken from it, on that part only: zero in, zero out, so that
    // a sound below the knee passes exactly
    const int32_t dc = m_dc[N] + ((whole_part - m_dc[N]) >> 11);
    m_dc[N] = dc;
    // The tone, as the sum of the two weights (not a difference), so that at x 1 it passes exactly
    const int32_t toned = multiply_shift_right(driven + (whole_part - dc), m_tone, 30) + multiply_shift_right(m_tone_out[N], m_tone_rest, 30);
    m_tone_out[N] = toned;
    return multiply_shift_right(toned, m_make_up, 16);
  }

  // numerator * 2^23 / denominator, about 15 bits exact, by one 32-bit division (no 64-bit one, a
  // call into the library, longer and with branches): the numerator is shifted up to 30 bits,
  // and the denominator down to 16 bits (from up to 29), and the quotient back by the difference,
  // with no branch (the shifts are by counts that are zero on the side not needed)
  static INLINE int32_t divide_in_unit(int32_t numerator, int32_t denominator) {
    const int32_t numerator_shift   = maximum(__builtin_clz(static_cast<uint32_t>(std::abs(numerator)) | 1) - 2, 0);  // to 30 bits
    const int32_t denominator_bits  = 32 - __builtin_clz(static_cast<uint32_t>(std::abs(denominator)) | 1);
    const int32_t denominator_shift = maximum(denominator_bits - 16, 0);                                            // to 16 bits
    const int32_t quotient = static_cast<int32_t>(static_cast<uint32_t>(numerator) << numerator_shift) / (denominator >> denominator_shift);
    // The quotient is numerator / denominator * 2^(numerator_shift + denominator_shift): times 2^23, back by that
    const int32_t shift = 23 - numerator_shift - denominator_shift;
    return static_cast<int32_t>(static_cast<uint32_t>(quotient) << maximum(shift, 0)) >> maximum(-shift, 0);
  }
};
