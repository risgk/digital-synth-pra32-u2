// The main thread part of PRA32-U2 Web: "PRA32U2WebSynth.output" is a virtual MIDI output
// ("PRA32U2WebSynth.outputs" has it and the one 6 dB louder)
// (with "name" and "send()", the same as MIDIOutput of Web MIDI API) for PRA32-U2 Editor.
// "pra32-u2-web-build.js" generates "pra32-u2-web-synth.js" from this file
// (with the WebAssembly binary and the AudioWorkletProcessor embedded, so that it works with "file://" too)

(function() {
  const PRA32_U2_WEB_VERSION = /*@PRA32_U2_WEB_VERSION@*/"";
  const WASM_BASE64          = /*@WASM_BASE64@*/"";
  const PROCESSOR_SOURCE     = /*@PROCESSOR_SOURCE@*/"";

  const MAX_PENDING_MESSAGES = 4096;

  let context  = null;
  let node     = null;
  let notice   = null;
  const pendingMessages = [];  // The MIDI messages before the AudioWorkletNode is ready

  function decodeBase64(base64) {
    const binary = atob(base64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) {
      bytes[i] = binary.charCodeAt(i);
    }
    return bytes;
  }

  // Browsers start the audio only after a user gesture (e.g. a click, a tap, or a key press)
  function resume() {
    if (context && (context.state !== "running")) {
      context.resume();
    }
  }

  // Shows a notice while the audio is not started (e.g. when only a MIDI keyboard is played)
  function updateNotice() {
    const suspended = (context !== null) && (context.state === "suspended");
    if (suspended && (notice === null)) {
      notice = document.createElement("div");
      notice.textContent = "PRA32-U2 Web: Click (or tap) to start the audio";
      notice.style.cssText = "position: fixed; bottom: 8px; left: 50%; transform: translateX(-50%); z-index: 1000;" +
                             "padding: 8px 16px; border-radius: 4px; background: #c00000; color: #ffffff;" +
                             "font-weight: bold; cursor: pointer; box-shadow: 0 2px 6px rgba(0, 0, 0, 0.4);";
      notice.addEventListener("click", resume);
      document.body.appendChild(notice);
    }
    if (notice !== null) {
      notice.style.display = suspended ? "block" : "none";
    }
  }

  async function start() {
    try {
      // The PRA32-U2 engine runs at 48 kHz (no resampling at 48 kHz)
      context = new AudioContext({ sampleRate: 48000, latencyHint: "interactive" });
    } catch (e) {
      context = new AudioContext({ latencyHint: "interactive" });
    }
    ["pointerdown", "touchend", "keydown"].forEach(function(type) {
      document.addEventListener(type, resume, true);
    });
    context.addEventListener("statechange", updateNotice);
    updateNotice();

    // A data URL (Chrome cannot load a Blob URL with "file://"), or a Blob URL (if a data URL fails)
    try {
      await context.audioWorklet.addModule("data:text/javascript;charset=utf-8," + encodeURIComponent(PROCESSOR_SOURCE));
    } catch (e) {
      const url = URL.createObjectURL(new Blob([PROCESSOR_SOURCE], { type: "text/javascript" }));
      try {
        await context.audioWorklet.addModule(url);
      } finally {
        URL.revokeObjectURL(url);
      }
    }

    node = new AudioWorkletNode(context, "pra32-u2-web-synth", {
      numberOfInputs: 0,
      numberOfOutputs: 1,
      outputChannelCount: [2],
      processorOptions: { wasmBytes: decodeBase64(WASM_BASE64) },
    });
    node.connect(context.destination);

    pendingMessages.forEach(function(data) { node.port.postMessage(data); });
    pendingMessages.length = 0;
    console.log(PRA32_U2_WEB_VERSION + " ready (" + context.sampleRate + " Hz)");
  }

  let outputGain = 1;      // That of the output that sent last
  let outputIsM  = false;  // PRA32-U2/M/P Web, or PRA32-U2 Web

  function post(message) {
    if (node) {
      node.port.postMessage(message);
    } else if (pendingMessages.length < MAX_PENDING_MESSAGES) {
      pendingMessages.push(message);
    }
  }

  // The outputs for MIDI Out: as PRA32-U2 sounds, and 6 dB louder (x 2, with a soft clip above -2 dBFS),
  // for a level closer to that of other sounds (e.g. of a Web browser); and PRA32-U2/M/P Web (the 4 synths by the MIDI
  // channels, with the Basic Channel 1), the same. The one that sends sets the gain and selects the synth (each of
  // PRA32-U2 Web and PRA32-U2/M/P Web keeps its own state)
  function makeOutput(name, gain, m) {
    return {
      name: name,
      isPRA32U2WebSynth: true,
      send: function(data) {
        if ((outputGain !== gain) || (outputIsM !== m)) {
          outputGain = gain;
          outputIsM  = m;
          post({ outputGain: gain, m: m });
        }
        post(Array.from(data).slice(0, 3));

        if (!context) {
          start().catch(function(e) { console.log("Failed to start PRA32-U2 Web - " + e); });
        }
        resume();
      },
    };
  }

  const output      = makeOutput("PRA32-U2 Web (Built-in)", 1, false);
  const outputLoud  = makeOutput("PRA32-U2 Web (Built-in, +6 dB)", 2, false);
  const outputM     = makeOutput("PRA32-U2/M/P Web (Built-in)", 1, true);
  const outputMLoud = makeOutput("PRA32-U2/M/P Web (Built-in, +6 dB)", 2, true);

  window.PRA32U2WebSynth = { version: PRA32_U2_WEB_VERSION, output: output, outputs: [output, outputLoud, outputM, outputMLoud] };
})();
