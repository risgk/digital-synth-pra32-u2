#pragma once

#include "pra32-u2-common.h"
#include "pra32-u2-filter.h"  // g_filter_drive_gain_table and interpolate_filter_table()

// Distortion, at the head of the FX: the input of the Distortion bus (FX Routing: Dst) is driven
// by D = 2^(value / 32), x 1 at 0 to x 15.7 at 127 (6 dB every 32 values, as the Filter Drive),
// into a curve that is a straight line up to the knee, 1.0 (1 << 23), and a quadratic shoulder
// from there that lands on 2.0 with zero slope at 3.0, and is taken back down by 1 / sqrt(D): half
// the drive in dB, so that a sound at 1.0 saturates without falling away, and the Output
// Compressor holds the rest. What comes out goes on to the Chorus, with the Chorus bus.
// The curve's part beyond the straight line, f(u) - u, goes through first-order antiderivative
// anti-aliasing (ADAA): the difference of its antiderivative over the step from the last sample,
// divided by the step. On the straight line that part is zero, so a sample that reaches no
// shoulder, now or a sample ago, passes exactly, neither late nor rounded off in the highs; past
// the knee it is half a sample late against the straight part. Below 0.001 of a step, where the
// difference loses its precision, the part is taken at the sample itself instead. At x 1, the
// Presets' and that of a user program written before, a synth below 1.0 passes as it did.
// In single-precision float (the FPU of the RP2350), as the division of the antiderivatives would
// otherwise be a 64-bit one, in software; the same number of operations for every sample, with
// no branch (the sign is copied, and the short step is a select between two constants)
class PRA32_U2_DistortionFx {
  uint8_t m_distortion_target;
  int32_t m_distortion_stage_1;
  int32_t m_distortion_current;        // Q16 controller value, smoothed
  float   m_drive;                     // D / (1 << 23): from the int24 sample to the curve's unit
  float   m_make_up;                   // (1 << 23) / sqrt(D): back to the int24 sample
  float   m_last_u[2];
  float   m_last_antiderivative[2];

public:
  PRA32_U2_DistortionFx()
  : m_distortion_target()
  , m_distortion_stage_1()
  , m_distortion_current()
  , m_drive(1.0f / 8388608.0f)
  , m_make_up(8388608.0f)
  , m_last_u()
  , m_last_antiderivative()
  {
    set_distortion(0);
  }

  INLINE void set_distortion(uint8_t controller_value) {
    m_distortion_target = controller_value;
  }

  // The drive and its make-up from the smoothed value, every time whatever the value: sqrt(D) is
  // 2^(value / 64), the drive's own table at half the value, so that no square root is taken
  // (sqrtf() is a call into the library)
  INLINE void process_at_low_rate(uint8_t count) {
    if (is_slow_smoothing_period(count)) {
      approach_exp_slow(m_distortion_stage_1, m_distortion_current, static_cast<int32_t>(m_distortion_target) << 16);
    }
    const float drive      = static_cast<float>(interpolate_filter_table(g_filter_drive_gain_table, m_distortion_current))      * (1.0f / 65536.0f);
    const float drive_sqrt = static_cast<float>(interpolate_filter_table(g_filter_drive_gain_table, m_distortion_current >> 1)) * (1.0f / 65536.0f);
    m_drive   = drive * (1.0f / 8388608.0f);
    m_make_up = 8388608.0f / drive_sqrt;
  }

  INLINE PRA32_U2_StereoSample process(PRA32_U2_StereoSample input_int24) {
    return { process_channel<0>(input_int24.left), process_channel<1>(input_int24.right) };
  }

private:
  template <uint8_t N>
  INLINE int32_t process_channel(int32_t input_int24) {
    const float u = static_cast<float>(input_int24) * m_drive;
    const float a = __builtin_fabsf(u);
    const float e = __builtin_fminf(__builtin_fmaxf(a, 1.0f), 3.0f) - 1.0f;
    const float d = __builtin_fmaxf(a - 3.0f, 0.0f);
    // f(u) - u, odd: zero up to the knee, -(a - 1)^2 / 4 over the shoulder, 2 - a past it
    const float part = __builtin_copysignf((e * e * 0.25f) + d, -u);
    // Its antiderivative from 0, even
    const float antiderivative = -(e * e * e * (0.25f / 3.0f)) - d - (d * d * 0.5f);
    const float step = u - m_last_u[N];
    const float is_short = (__builtin_fabsf(step) < 0.001f) ? 1.0f : 0.0f;
    const float r = (part * is_short) + ((antiderivative - m_last_antiderivative[N]) / (step + is_short)) * (1.0f - is_short);
    m_last_u[N] = u;
    m_last_antiderivative[N] = antiderivative;
    return static_cast<int32_t>((u + r) * m_make_up);
  }
};
