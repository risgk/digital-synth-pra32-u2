// The AudioWorkletProcessor of PRA32-U2 Web: runs the PRA32-U2 engine (WebAssembly) on the audio thread.
// Embedded in "pra32-u2-web-synth.js" (as a string, loaded by a Blob URL) by "pra32-u2-web-build.js"

// The output gain of the "+6 dB" output: x 2, with a soft clip that leaves the samples below 0.8 (-2 dBFS) as they are,
// and rounds off those above it toward 1.0 (so that a peak is not cut); x 1 leaves the output as it is
const SOFT_CLIP_KNEE = 0.8;
function applyOutputGain(samples, n, gain) {
  for (let i = 0; i < n; i++) {
    const x = samples[i] * gain;
    const a = Math.abs(x);
    if (a > SOFT_CLIP_KNEE) {
      const y = SOFT_CLIP_KNEE + (1 - SOFT_CLIP_KNEE) * Math.tanh((a - SOFT_CLIP_KNEE) / (1 - SOFT_CLIP_KNEE));
      samples[i] = (x < 0) ? -y : y;
    } else {
      samples[i] = x;
    }
  }
}

class PRA32U2WebSynthProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();

    // The WASI functions are used only by the error messages of the C/C++ libraries (not used normally)
    const stub = () => 0;
    const imports = { wasi_snapshot_preview1: { fd_close: stub, fd_write: stub, fd_seek: stub } };
    const module = new WebAssembly.Module(options.processorOptions.wasmBytes);
    this.wasm = new WebAssembly.Instance(module, imports).exports;
    this.wasm._initialize();
    this.wasm.pra32u2_web_prepare(sampleRate);

    this.maxBlockSize = this.wasm.pra32u2_web_max_block_size();
    this.left  = new Float32Array(this.wasm.memory.buffer, this.wasm.pra32u2_web_left(),  this.maxBlockSize);
    this.right = new Float32Array(this.wasm.memory.buffer, this.wasm.pra32u2_web_right(), this.maxBlockSize);

    this.outputGain = 1;

    // A MIDI message (an array of 1-3 bytes) is handled at the beginning of the next block;
    // { outputGain: gain, m: m } sets the output gain and selects PRA32-U2 Web or PRA32-U2/M/P Web (by the output chosen in MIDI Out)
    this.port.onmessage = (event) => {
      const data = event.data;
      if (!Array.isArray(data)) {
        this.outputGain = data.outputGain;
        this.wasm.pra32u2_web_select(data.m ? 1 : 0);
        return;
      }
      this.wasm.pra32u2_web_midi(0, data[0] | 0, data[1] | 0, data[2] | 0, Math.min(data.length, 3));
    };
  }

  process(inputs, outputs) {
    const output = outputs[0];
    const numSamples = output[0].length;
    for (let offset = 0; offset < numSamples; offset += this.maxBlockSize) {
      const n = Math.min(numSamples - offset, this.maxBlockSize);
      this.wasm.pra32u2_web_render(n);
      output[0].set(this.left.subarray(0, n), offset);
      if (output.length >= 2) {
        output[1].set(this.right.subarray(0, n), offset);
      }
    }
    if (this.outputGain !== 1) {
      output.forEach((channel) => applyOutputGain(channel, numSamples, this.outputGain));
    }
    return true;
  }
}

registerProcessor("pra32-u2-web-synth", PRA32U2WebSynthProcessor);
