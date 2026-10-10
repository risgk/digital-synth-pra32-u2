// The C API of PRA32-U2 Web (WebAssembly), used by the AudioWorkletProcessor (pra32-u2-web-synth-processor.js).
// The renderer of PRA32-U2 Native (PRA32U2Renderer, PRA32U2Engine, PRA32U2Resampler) is used as it is

#include <emscripten/emscripten.h>

#include "PRA32U2Renderer.h"

namespace {

constexpr int kMaxBlockSize = 1024;

PRA32U2Renderer* s_renderer = nullptr;
float            s_left[kMaxBlockSize];
float            s_right[kMaxBlockSize];

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int pra32u2_web_max_block_size() {
  return kMaxBlockSize;
}

EMSCRIPTEN_KEEPALIVE void pra32u2_web_prepare(double samplingRate) {
  if (s_renderer == nullptr) {
    s_renderer = new PRA32U2Renderer();
  }
  s_renderer->prepare(samplingRate);
}

// Adds a MIDI message (1-3 bytes) at sampleOffset in the next pra32u2_web_render() block
EMSCRIPTEN_KEEPALIVE int pra32u2_web_midi(int sampleOffset, int data0, int data1, int data2, int size) {
  const uint8_t data[3] = { static_cast<uint8_t>(data0), static_cast<uint8_t>(data1), static_cast<uint8_t>(data2) };
  return s_renderer->addMidiEvent(sampleOffset, data, size) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE float* pra32u2_web_left()  { return s_left; }
EMSCRIPTEN_KEEPALIVE float* pra32u2_web_right() { return s_right; }

// Renders numSamples (up to pra32u2_web_max_block_size()) to pra32u2_web_left() and pra32u2_web_right()
EMSCRIPTEN_KEEPALIVE void pra32u2_web_render(int numSamples) {
  s_renderer->render(s_left, s_right, numSamples);
}

}  // extern "C"
