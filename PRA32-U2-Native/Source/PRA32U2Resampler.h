#pragma once

// A streaming stereo resampler from 48 kHz (the PRA32-U2 core) to the output sampling rate
// (windowed sinc interpolation with a Kaiser window). At 48 kHz, it is bypassed (no latency).
//
// Usage: for each output sample, push the input samples while
// getNumInputsPushed() < getNumInputsNeeded(), then call popOutput()

#include <cstdint>
#include <vector>

class PRA32U2Resampler {
public:
  static constexpr int kInputSamplingRate = 48000;
  static constexpr int kHalfTaps          = 24;   // Taps on each side (48 taps)
  static constexpr int kPhases            = 256;  // Resolution of the fractional position

  void prepare(double outputSamplingRate);

  bool isBypassed() const { return m_bypassed; }

  // Latency in the input samples (48 kHz) and in the output samples
  int getLatencyInInputSamples() const { return m_bypassed ? 0 : kHalfTaps; }
  int getLatencyInOutputSamples() const;

  int64_t getNumInputsPushed() const  { return m_numInputsPushed; }
  int64_t getNumOutputsPopped() const { return m_numOutputsPopped; }

  // The number of the input samples needed to calculate the next output sample
  int64_t getNumInputsNeeded() const { return getNumInputsNeededFor(m_numOutputsPopped); }
  int64_t getNumInputsNeededFor(int64_t outputIndex) const;

  // The index of the input sample whose timing corresponds to the output sample,
  // including the latency (used to schedule the MIDI events)
  int64_t getInputIndexFor(int64_t outputIndex) const;

  void pushInput(float left, float right);
  void popOutput(float& left, float& right);

private:
  static constexpr int kRingSize = 128;  // Power of 2, more than (kHalfTaps * 2)

  bool                 m_bypassed      = true;
  int64_t              m_outputRate    = kInputSamplingRate;
  int64_t              m_numInputsPushed  = 0;
  int64_t              m_numOutputsPopped = 0;
  float                m_ringL[kRingSize] = {};
  float                m_ringR[kRingSize] = {};
  std::vector<float>   m_table;  // (kPhases + 1) x (kHalfTaps * 2)
};
