#pragma once

// Runs the PRA32-U2 engine at 48 kHz and resamples its output to the host sampling rate,
// with the MIDI events scheduled at the corresponding 48 kHz samples. Without JUCE

#include <cstdint>

#include "PRA32U2Engine.h"
#include "PRA32U2Resampler.h"

class PRA32U2Renderer {
public:
  void prepare(double hostSamplingRate);

  // Latency in the host samples (0 at 48 kHz)
  int getLatencySamples() const { return m_resampler.getLatencyInOutputSamples(); }

  // Adds a MIDI event at sampleOffset in the next render() block.
  // Call it in the order of sampleOffset. Returns false if the queue is full
  bool addMidiEvent(int sampleOffset, const uint8_t* data, int size);

  void render(float* left, float* right, int numSamples);

  PRA32U2Engine&       getEngine()       { return m_engine; }
  const PRA32U2Engine& getEngine() const { return m_engine; }

private:
  struct MidiEvent {
    int64_t inputIndex;  // The 48 kHz sample before which the event is handled
    uint8_t data[3];
    uint8_t size;
  };

  static constexpr int kQueueSize = 4096;  // Power of 2

  void dispatchMidiEventsUpTo(int64_t inputIndex);
  void flushMidiEvents();

  PRA32U2Engine    m_engine;
  PRA32U2Resampler m_resampler;

  MidiEvent        m_queue[kQueueSize] = {};
  uint32_t         m_queueHead = 0;  // Read
  uint32_t         m_queueTail = 0;  // Write
};
