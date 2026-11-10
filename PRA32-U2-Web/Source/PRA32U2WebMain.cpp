// The C API of PRA32-U2 Web (WebAssembly), used by the AudioWorkletProcessor (pra32-u2-web-synth-processor.js).
// The renderer of PRA32-U2 Native (PRA32U2Renderer, PRA32U2Engine, PRA32U2Resampler) is used as it is,
// for PRA32-U2 Web and for PRA32-U2/M/P Web (built with PRA32_U2_ENGINE_M), each with its own state; the one selected
// takes the MIDI messages and is rendered

#include <emscripten/emscripten.h>

#include "PRA32U2Renderer.h"

namespace {

constexpr int kMaxBlockSize = 1024;

PRA32U2Renderer* s_renderers[2] = { nullptr, nullptr };  // PRA32-U2 Web, and PRA32-U2/M/P Web
PRA32U2Renderer* s_renderer = nullptr;                    // The one selected
float            s_left[kMaxBlockSize];
float            s_right[kMaxBlockSize];

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int pra32u2_web_max_block_size() {
  return kMaxBlockSize;
}

EMSCRIPTEN_KEEPALIVE void pra32u2_web_prepare(double samplingRate) {
  if (s_renderer == nullptr) {
    s_renderers[0] = new PRA32U2Renderer(false);
    s_renderers[1] = new PRA32U2Renderer(true);
    s_renderer = s_renderers[0];
  }
  s_renderers[0]->prepare(samplingRate);
  s_renderers[1]->prepare(samplingRate);
}

// Selects PRA32-U2 Web (0) or PRA32-U2/M/P Web (1)
EMSCRIPTEN_KEEPALIVE void pra32u2_web_select(int m) {
  s_renderer = s_renderers[(m != 0) ? 1 : 0];
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
