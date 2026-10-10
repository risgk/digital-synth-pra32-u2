# PRA32-U2 Web v3.11.0

- 2026-10-10 ISGK Instruments
- <https://github.com/risgk/digital-synth-pra32-u2>

PRA32-U2 Web: a software synthesizer version of Digital Synth PRA32-U2 that runs in a Web browser, built into PRA32-U2 Editor

- The signal processing is the same as PRA32-U2 (Digital-Synth-PRA32-U2/*.h are used as they are),
  compiled to WebAssembly with the renderer of [PRA32-U2 Native](../PRA32-U2-Native/README.md)
  (PRA32U2Engine, PRA32U2Renderer, and PRA32U2Resampler)
- Version: the same as PRA32-U2 (`PRA32_U2_VERSION` in "Digital-Synth-PRA32-U2.ino"), shown in the console of the Web browser
- "pra32-u2-web-synth.js" (in the parent folder, next to "pra32-u2-editor.html"): the built file
    - The WebAssembly binary (about 260 KB) and the AudioWorkletProcessor are embedded, so that it works
      both on a Web server (e.g. GitHub Pages) and with a local file ("file://")
    - Without this file, PRA32-U2 Editor works as before (MIDI Out only)


## How to Use

- Open "pra32-u2-editor.html" (a local file, or on a Web server) with Google Chrome or Microsoft Edge (recommended)
- Select "PRA32-U2 Web (Built-in)" in the list "MIDI Out"
    - The current parameters of PRA32-U2 Editor are sent to PRA32-U2 Web ("Send Current")
- Play the keyboard of PRA32-U2 Editor, or a MIDI keyboard selected in "MIDI In 1-4"
    - Web browsers start the audio only after a user operation in the page (e.g. a click, a tap, or a key press),
      so click the page once before playing a MIDI keyboard (clicking the keyboard of PRA32-U2 Editor also starts the audio)
    - While the audio is not started, "PRA32-U2 Web: Click (or tap) to start the audio" is shown at the bottom of the page
- Web browsers
    - Google Chrome, Microsoft Edge (Windows, Mac, Linux, Android): recommended
      (Web MIDI API, i.e. "MIDI In" and "MIDI Out" to the devices, needs a permission)
    - Firefox: Web MIDI API needs a permission (a site permission add-on)
    - Safari (Mac, iPhone, iPad) and all the Web browsers on iPhone and iPad: Web MIDI API is not supported,
      so only the keyboard of PRA32-U2 Editor can be used
- Multiple windows (tabs) can be opened, and each of them has its own PRA32-U2 Web
    - PRA32-U2 Editor receives all the MIDI channels from "MIDI In" and sends them to "MIDI Out" (Omni On)
    - The windows share the current parameters and the user presets stored in the Web browser (localStorage)


## Specifications

- The same as PRA32-U2 Native (MIDI, parameters, and sampling rate), except:
    - Sampling rate: 48 kHz is requested to the Web browser (no resampling);
      at other sampling rates, the output is resampled as in PRA32-U2 Native
    - The MIDI messages are handled at the beginning of the next audio block (128 samples, about 2.7 ms at 48 kHz)


## Limitations

- "Write Parameters to Program" (CC #87 and #106) writes the program to the memory only,
  so the written programs are lost when the window is closed or reloaded
    - PRA32-U2 Editor stores the user programs in the Web browser
- The control panel (PRA32-U2/P) and the step sequencer are not supported
- PRA32-U2/M (Multi-Timbre Edition) is not supported


## How to Build

- Requirements: [Emscripten](https://emscripten.org) (em++ and Node.js; installed with emsdk) and Git
    - Built with Emscripten 6.0.12 (`EMSDK_VERSION` in ".github/workflows/pra32-u2-native-windows.yml")
- Install Emscripten (emsdk), once (in any folder, outside this repository):

```
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install 6.0.12
./emsdk activate 6.0.12
```

- On Windows (Command Prompt): `emsdk install 6.0.12` and `emsdk activate 6.0.12` (without "./")
- Set up the environment variables (PATH, etc.) in each new terminal:
    - Linux and macOS: `source <emsdk folder>/emsdk_env.sh`
    - Windows (Command Prompt): `<emsdk folder>\emsdk_env.bat`
- Build in this folder ("PRA32-U2-Web"), and test (optional):

```
node pra32-u2-web-build.js
node pra32-u2-web-test.js
```

- The output: "../pra32-u2-web-synth.js"
- GitHub Actions: "PRA32-U2 Build (UF2, Native, and Web)" in ".github/workflows/pra32-u2-native-windows.yml"
  builds and tests "pra32-u2-web-synth.js" (and commits it with "commit_package")
- The source files
    - "Source/PRA32U2WebMain.cpp": the C API for the AudioWorkletProcessor
      (the source files of PRA32-U2 Native in "../PRA32-U2-Native/Source/" are used as they are)
    - "pra32-u2-web-synth-processor.js": the AudioWorkletProcessor (runs the WebAssembly)
    - "pra32-u2-web-synth-main.js": the main thread part (the virtual MIDI output for PRA32-U2 Editor)
    - "pra32-u2-web-build.js": the build script
    - "pra32-u2-web-test.js": the test of the built file with Node.js (plays the Factory Presets
      at 48 kHz, 44.1 kHz, and 96 kHz, and checks that they are silent after the note off)


## License

- [CC0 1.0](../LICENSE) (the same as PRA32-U2; JUCE is not used)
