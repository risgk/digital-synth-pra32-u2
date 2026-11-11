// Tests "../pra32-u2-web-synth.js" (the built file) with Node.js: the embedded WebAssembly plays a note
// (Program #16-31 and the sampling rates of the Web browsers), and is almost silent (below -40 dB, i.e. no stuck notes) after the note off;
// and PRA32-U2/M/P Web by the channels.
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

// PRA32-U2/M/P Web (selected by pra32u2_web_select(1)): with the Main Synth in Mono (#20 Saw Lead), a note sounds on the
// channels of the 4 synths (Basic Channel 1), and not on the others (e.g. 5 and 16, with Voice Group Mode Off);
// then on Channel 5 and 16 too, with Voice Group Mode RR4 (CC #80 on the Basic Channel)
{
  const stub = () => 0;
  const wasm = new WebAssembly.Instance(wasmModule, { wasi_snapshot_preview1: { fd_close: stub, fd_write: stub, fd_seek: stub } }).exports;
  wasm._initialize();
  wasm.pra32u2_web_prepare(48000);
  wasm.pra32u2_web_select(1);
  wasm.pra32u2_web_midi(0, 0xC0, 20, 0, 2);
  peakOf(wasm, 4800);
  const cases = [[1, true], [2, true], [3, true], [4, true], [5, false], [6, false], [7, false], [12, false], [16, false],
                 ["RR4", null], [5, true], [6, false], [16, true]];
  for (const [channel, sounds] of cases) {
    if (channel === "RR4") {
      wasm.pra32u2_web_midi(0, 0xB0, 80, 127, 3); // Voice Group Mode: RR4
      peakOf(wasm, 4800);
      continue;
    }
    wasm.pra32u2_web_midi(0, 0x90 | (channel - 1), 60, 100, 3);
    const noteOnPeak = peakOf(wasm, 9600);
    wasm.pra32u2_web_midi(0, 0x80 | (channel - 1), 60, 64, 3);
    peakOf(wasm, 96000);
    const afterPeak = peakOf(wasm, 4800);
    const ok = (sounds ? ((noteOnPeak > 0.01) && (noteOnPeak <= 1.0)) : (noteOnPeak < 0.001)) && (afterPeak < 0.01);
    failed = failed || !ok;
    console.log("PRA32-U2/M/P Web, Channel " + channel + ": peak " + noteOnPeak.toFixed(3) + ", after " + afterPeak.toFixed(4) + (ok ? "" : " NG"));
  }
}

// PRA32-U2/M/P Web with the panel parameters (Experimental): the Step Sequencer sounds by the Panel Channel (Basic Channel 1 + 14:
// Channel 15), started by Panel Play (CC #111) in Seq Mode (Panel Play Mode, CC #53), and is almost silent after it is stopped
{
  const stub = () => 0;
  const wasm = new WebAssembly.Instance(wasmModule, { wasi_snapshot_preview1: { fd_close: stub, fd_write: stub, fd_seek: stub } }).exports;
  wasm._initialize();
  wasm.pra32u2_web_prepare(48000);
  wasm.pra32u2_web_select(1);
  wasm.pra32u2_web_midi(0, 0xBE, 53, 127, 3);  // Panel Play Mode: Seq
  const beforePeak = peakOf(wasm, 48000);
  wasm.pra32u2_web_midi(0, 0xBE, 111, 127, 3); // Panel Play: on (the sequencer starts)
  const playPeak = peakOf(wasm, 96000);
  wasm.pra32u2_web_midi(0, 0xBE, 111, 0, 3);   // Panel Play: off (the sequencer stops)
  peakOf(wasm, 96000);
  const afterPeak = peakOf(wasm, 4800);
  const ok = (beforePeak < 0.001) && (playPeak > 0.01) && (playPeak <= 1.0) && (afterPeak < 0.01);
  failed = failed || !ok;
  console.log("PRA32-U2/M/P Web, Step Sequencer by the Panel Channel: before " + beforePeak.toFixed(4) + ", peak " + playPeak.toFixed(3) +
              ", after " + afterPeak.toFixed(4) + (ok ? "" : " NG"));
}

if (failed) {
  console.log("FAILED");
  process.exit(1);
}
console.log("OK");
