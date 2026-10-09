#include "PRA32U2Renderer.h"

#include <algorithm>

void PRA32U2Renderer::prepare(double hostSamplingRate) {
  flushMidiEvents();
  m_resampler.prepare(hostSamplingRate);
}

bool PRA32U2Renderer::addMidiEvent(int sampleOffset, const uint8_t* data, int size) {
  // Only channel voice messages (up to 3 bytes); SysEx and system messages are ignored
  if ((size < 1) || (size > 3) || (data[0] < 0x80) || (data[0] >= 0xF0)) {
    return true;
  }

  if (m_queueTail - m_queueHead >= static_cast<uint32_t>(kQueueSize)) {
    return false;
  }

  MidiEvent& event = m_queue[m_queueTail & (kQueueSize - 1)];

  // The events are delayed by the latency of the resampler, so that they are not
  // handled after the 48 kHz samples ahead of the current output have been processed
  int64_t outputIndex = m_resampler.getNumOutputsPopped() + std::max(sampleOffset, 0);
  event.inputIndex = std::max(m_resampler.getInputIndexFor(outputIndex), m_resampler.getNumInputsPushed());

  event.size = static_cast<uint8_t>(size);
  for (int i = 0; i < 3; ++i) {
    event.data[i] = (i < size) ? data[i] : 0;
  }

  ++m_queueTail;
  return true;
}

void PRA32U2Renderer::render(float* left, float* right, int numSamples) {
  for (int n = 0; n < numSamples; ++n) {
    const int64_t needed = m_resampler.getNumInputsNeeded();
    while (m_resampler.getNumInputsPushed() < needed) {
      dispatchMidiEventsUpTo(m_resampler.getNumInputsPushed());

      float l;
      float r;
      m_engine.process(l, r);
      m_resampler.pushInput(l, r);
    }

    m_resampler.popOutput(left[n], right[n]);
  }
}

void PRA32U2Renderer::dispatchMidiEventsUpTo(int64_t inputIndex) {
  while (m_queueHead != m_queueTail) {
    const MidiEvent& event = m_queue[m_queueHead & (kQueueSize - 1)];
    if (event.inputIndex > inputIndex) {
      break;
    }
    m_engine.handleMidiMessage(event.data, event.size);
    ++m_queueHead;
  }
}

void PRA32U2Renderer::flushMidiEvents() {
  while (m_queueHead != m_queueTail) {
    const MidiEvent& event = m_queue[m_queueHead & (kQueueSize - 1)];
    m_engine.handleMidiMessage(event.data, event.size);
    ++m_queueHead;
  }
}
