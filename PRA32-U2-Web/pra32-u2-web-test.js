// Tests "../pra32-u2-web-synth.js" (the built file) with Node.js: the embedded WebAssembly plays a note
// (Program #16-31 and the sampling rates of the Web browsers), and is almost silent (below -40 dB, i.e. no stuck notes) after the note off.
// Usage (in this folder): node pra32-u2-web-test.js

"use strict";

const fs = require("fs");
const path = require("path");

const source = fs.readFileSync(path.join(__dirname, "..", "pra32-u2-web-synth.js"), "utf8");
const base64 = source.match(/WASM_BASE64\s*=\s*"([A-Za-z0-9+/=]+)"/)[1];
const wasmModule = new WebAssembly.Module(Buffer.from(base64, "base64"));

function peakOf(wasm, numSamples) {
  const blockSize = 128;
  const left  = new Float32Array(wasm.memory.buffer, wasm.pra32u2_web_left(),  blockSize);
  const right = new Float32Array(wasm.memory.buffer, wasm.pra32u2_web_right(), blockSize);
  let peak = 0;
  for (let n = 0; n < numSamples; n += blockSize) {
    wasm.pra32u2_web_render(blockSize);
    for (let i = 0; i < blockSize; i++) {
      if (!Number.isFinite(left[i]) || !Number.isFinite(right[i])) {
        throw new Error("Not finite output");
      }
      peak = Math.max(peak, Math.abs(left[i]), Math.abs(right[i]));
    }
  }
  return peak;
}

let failed = false;
for (const samplingRate of [48000, 44100, 96000]) {
  for (let program = 16; program <= 31; program++) {
    const stub = () => 0;
    const wasm = new WebAssembly.Instance(wasmModule, { wasi_snapshot_preview1: { fd_close: stub, fd_write: stub, fd_seek: stub } }).exports;
    wasm._initialize();
    wasm.pra32u2_web_prepare(samplingRate);

    wasm.pra32u2_web_midi(0, 0xC0, program, 0, 2);
    wasm.pra32u2_web_midi(0, 0x90, 60, 100, 3);
    const noteOnPeak = peakOf(wasm, samplingRate);       // 1 s
    wasm.pra32u2_web_midi(0, 0x80, 60, 64, 3);
    wasm.pra32u2_web_midi(0, 0xB0, 120, 0, 3);           // All Sound Off (the delay and the release)
    peakOf(wasm, samplingRate);                          // 1 s
    const afterPeak = peakOf(wasm, samplingRate / 10);   // 0.1 s

    const ok = (noteOnPeak > 0.01) && (noteOnPeak <= 1.0) && (afterPeak < 0.01);
    failed = failed || !ok;
    console.log(samplingRate + " Hz, Program #" + program + ": peak " + noteOnPeak.toFixed(3) +
                ", after " + afterPeak.toFixed(4) + (ok ? "" : " NG"));
  }
}

if (failed) {
  console.log("FAILED");
  process.exit(1);
}
console.log("OK");
