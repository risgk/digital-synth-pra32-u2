#include "PRA32U2Resampler.h"

#include <algorithm>
#include <cmath>

namespace {

const double PI          = 3.14159265358979323846;
const double KAISER_BETA = 9.0;

// The zeroth-order modified Bessel function of the first kind
double bessel_i0(double x) {
  double sum  = 1.0;
  double term = 1.0;
  for (int k = 1; k < 64; ++k) {
    term *= (x / (2.0 * k)) * (x / (2.0 * k));
    sum += term;
    if (term < sum * 1e-17) {
      break;
    }
  }
  return sum;
}

double sinc(double x) {
  if (std::fabs(x) < 1e-12) {
    return 1.0;
  }
  return std::sin(PI * x) / (PI * x);
}

}  // namespace

void PRA32U2Resampler::prepare(double outputSamplingRate) {
  m_outputRate = std::max<int64_t>(1, static_cast<int64_t>(std::llround(outputSamplingRate)));
  m_bypassed = (m_outputRate == kInputSamplingRate);
  m_numInputsPushed  = 0;
  m_numOutputsPopped = 0;
  std::fill(std::begin(m_ringL), std::end(m_ringL), 0.0f);
  std::fill(std::begin(m_ringR), std::end(m_ringR), 0.0f);

  if (m_bypassed) {
    m_table.clear();
    return;
  }

  // Cutoff (cycles per input sample): a little below the lower Nyquist frequency.
  // The core limits the oscillators to 23 kHz (FREQUENCY_MAX), so that little is lost at 48 kHz or higher
  const double nyquist_ratio = std::min(1.0, static_cast<double>(m_outputRate) / kInputSamplingRate);
  const double cutoff        = 0.5 * nyquist_ratio * ((m_outputRate >= kInputSamplingRate) ? 0.96 : 0.92);

  const int    taps          = kHalfTaps * 2;
  const double i0_beta       = bessel_i0(KAISER_BETA);

  m_table.assign(static_cast<size_t>(kPhases + 1) * taps, 0.0f);

  for (int phase = 0; phase <= kPhases; ++phase) {
    const double frac = static_cast<double>(phase) / kPhases;
    double coefs[kHalfTaps * 2];
    double sum = 0.0;

    for (int tap = 0; tap < taps; ++tap) {
      const int    j = tap - kHalfTaps + 1;  // -(kHalfTaps - 1) to kHalfTaps
      const double t = j - frac;             // Distance from the output position
      const double r = t / kHalfTaps;
      double window = 0.0;
      if (std::fabs(r) < 1.0) {
        window = bessel_i0(KAISER_BETA * std::sqrt(1.0 - r * r)) / i0_beta;
      }
      coefs[tap] = 2.0 * cutoff * sinc(2.0 * cutoff * t) * window;
      sum += coefs[tap];
    }

    // Normalized for the unity gain at DC
    for (int tap = 0; tap < taps; ++tap) {
      m_table[static_cast<size_t>(phase) * taps + tap] = static_cast<float>(coefs[tap] / sum);
    }
  }
}

int PRA32U2Resampler::getLatencyInOutputSamples() const {
  if (m_bypassed) {
    return 0;
  }
  return static_cast<int>((static_cast<int64_t>(kHalfTaps) * m_outputRate + (kInputSamplingRate / 2)) / kInputSamplingRate);
}

int64_t PRA32U2Resampler::getNumInputsNeededFor(int64_t outputIndex) const {
  if (m_bypassed) {
    return outputIndex + 1;
  }
  const int64_t input_index = (outputIndex * kInputSamplingRate) / m_outputRate;
  return input_index + kHalfTaps + 1;
}

int64_t PRA32U2Resampler::getInputIndexFor(int64_t outputIndex) const {
  if (m_bypassed) {
    return outputIndex;
  }
  return (outputIndex * kInputSamplingRate) / m_outputRate + kHalfTaps;
}

void PRA32U2Resampler::pushInput(float left, float right) {
  const int index = static_cast<int>(m_numInputsPushed & (kRingSize - 1));
  m_ringL[index] = left;
  m_ringR[index] = right;
  ++m_numInputsPushed;
}

void PRA32U2Resampler::popOutput(float& left, float& right) {
  if (m_bypassed) {
    const int index = static_cast<int>(m_numOutputsPopped & (kRingSize - 1));
    left  = m_ringL[index];
    right = m_ringR[index];
    ++m_numOutputsPopped;
    return;
  }

  // The position in the input samples: input_index + frac
  const int64_t position_num = m_numOutputsPopped * kInputSamplingRate;
  const int64_t input_index  = position_num / m_outputRate;
  const double  frac         = static_cast<double>(position_num % m_outputRate) / static_cast<double>(m_outputRate);

  const double phase_pos  = frac * kPhases;
  const int    phase      = std::min(static_cast<int>(phase_pos), kPhases - 1);
  const float  phase_frac = static_cast<float>(phase_pos - phase);

  const int    taps   = kHalfTaps * 2;
  const float* coefs0 = &m_table[static_cast<size_t>(phase) * taps];
  const float* coefs1 = coefs0 + taps;

  float sum_l = 0.0f;
  float sum_r = 0.0f;
  int64_t first = input_index - kHalfTaps + 1;
  for (int tap = 0; tap < taps; ++tap) {
    const float coef  = coefs0[tap] + (coefs1[tap] - coefs0[tap]) * phase_frac;
    const int   index = static_cast<int>((first + tap) & (kRingSize - 1));
    sum_l += m_ringL[index] * coef;
    sum_r += m_ringR[index] * coef;
  }

  left  = sum_l;
  right = sum_r;
  ++m_numOutputsPopped;
}
