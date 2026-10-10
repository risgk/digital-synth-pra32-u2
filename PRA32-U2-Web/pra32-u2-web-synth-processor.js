// The AudioWorkletProcessor of PRA32-U2 Web: runs the PRA32-U2 engine (WebAssembly) on the audio thread.
// Embedded in "pra32-u2-web-synth.js" (as a string, loaded by a Blob URL) by "pra32-u2-web-build.js"

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

    // A MIDI message (an array of 1-3 bytes) is handled at the beginning of the next block
    this.port.onmessage = (event) => {
      const data = event.data;
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
    return true;
  }
}

registerProcessor("pra32-u2-web-synth", PRA32U2WebSynthProcessor);
