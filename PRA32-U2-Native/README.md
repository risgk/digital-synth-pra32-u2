# PRA32-U2 Native v3.9.1 (Experimental)

- 2026-10-09 ISGK Instruments
- <https://github.com/risgk/digital-synth-pra32-u2>

PRA32-U2 Native: a software synthesizer (VST3 plugin and standalone application) version of Digital Synth PRA32-U2

- PRA32-U2 for PCs: a VST3 plugin and a standalone application, built with [JUCE](https://juce.com)
- **Experimental**: the features and the behaviors may change, and there may be bugs
- The signal processing is the same as PRA32-U2 (Digital-Synth-PRA32-U2/*.h are used as they are)
- Version: the same as PRA32-U2 (`PRA32_U2_VERSION` in "Digital-Synth-PRA32-U2.ino"), shown at the top of the editor
- Binaries: Windows (x64) only, VST3 and Standalone: "bin/PRA32-U2-Native-v*-Windows-x64.zip"
    - Not signed, so Windows SmartScreen may warn
    - Built and validated (pluginval) by GitHub Actions ("PRA32-U2 Build (UF2 and Native)" in ".github/workflows/pra32-u2-native-windows.yml")
    - On Mac and Linux, build from the source (see "How to Build")


## Features

- No GUI of its own (only the standard JUCE components)
    - VST3 and Standalone: a generic parameter editor (sliders and values, the same as JUCE's,
      with wider parameter names, and the names and the values left-justified)
        - The sound parameters are shown in the same order as PRA32-U2 Editor, with the supplements in [] in the names
        - The sliders have the markers as in PRA32-U2 Editor (e.g. 6 markers for "Osc 1 Wave")
        - The header shows the name and the version, and the "Program Change"
          button (the same style as the "Options" button of the Standalone) sends Program Change #16-31 (the Factory Presets, e.g. "#16 Synth Pad")
          to itself (not the host's presets, so that the host does not change the sound when it loads the project)
        - The "Randomize Synth" and "Randomize FX" buttons in the header work as in PRA32-U2 Editor
          ("Randomize Synth" sends Program Change #127 to itself)
        - The parameters named "---" can be moved in the host's generic parameter editor, but do nothing
        - The host's generic parameter editor can also be used (the parameters are in the order of the control numbers)
    - Standalone: the "Options" menu (JUCE's): "Audio/MIDI Settings...", "Save current state...", "Load a saved state...",
      and "Reset to default state"
- MIDI input: all channels are received (Omni On)
    - Note On/Off, Control Change, Program Change, Pitch Bend, Polyphonic/Channel After Touch
    - SysEx and system messages are ignored
- Parameters: the sound parameters of PRA32-U2 (the same as the parameters of the programs)
    - The names are the same as in PRA32-U2 Editor, after the control numbers,
      so that the hosts that show only the beginning of the names show the control numbers
        - VST3: without the supplements in [] (e.g. "CC#102 Osc 1 Wave"), so that the names are not too long
          (e.g. Cubase omits the middle of a long name); the values show them (e.g. "0 [Saw]")
        - Standalone: with the supplements (e.g. "CC#102 Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]")
    - Each parameter has the same value as its Control Change (0-127; the normalized value is CC / 127, i.e. 127 = 1.0)
    - The control numbers are the base of the compatibility, the same as PRA32-U2 (and PRA32-U2 Editor)
        - The parameter numbers (indices, and the VST3 parameter IDs) are the same as the control numbers (CC#0-127)
        - The plugin state (the host's project, and the Standalone's settings) stores the values by the control numbers
        - The control numbers that are not the sound parameters have the parameters named "---" (e.g. "CC#1 ---"),
          which do nothing and cannot be automated (these controls work by MIDI, e.g. CC#1 Modulation)
        - The hosts list the parameters in the order of the control numbers;
          the Standalone shows only the sound parameters, in the same order as PRA32-U2 Editor
    - The values are shown as in PRA32-U2 Editor (e.g. "26 [Sqr]", "67 [+3]")
    - The default values are the values of Program #0 (the synth is initialized with Program #0)
        - To reset all the parameters to Program #0: "Options" > "Reset to default state" (Standalone),
          or send Program Change #0 (e.g. from the host or PRA32-U2 Editor)
    - The parameter changes by the host are sent to the synth as Control Changes
    - The values changed by MIDI (Control Changes, Program Changes, PRA32-U2 Editor) are reflected to the parameters
    - The parameters are saved in the host's project (the plugin state)
- Sampling rate: the signal processing runs at 48 kHz only, and its output is resampled to the host sampling rate
    - At 48 kHz, no resampling (no latency)
    - At other sampling rates, a windowed sinc resampler (48 taps at 48 kHz) is used,
      with a latency of 24 samples at 48 kHz (0.5 ms; reported to the host)
    - The MIDI events are handled at the corresponding samples at 48 kHz


## Using with PRA32-U2 Editor

- Connect "pra32-u2-editor.html" to PRA32-U2 Native with a virtual MIDI cable
    - On Windows, [loopMIDI](https://www.tobias-erichsen.de/software/loopmidi.html) is recommended
    - On Mac, a virtual MIDI bus (port) can be created by using the IAC bus
- Standalone: select the virtual MIDI cable as the MIDI input in "Options" > "Audio/MIDI Settings..."
- VST3: route the virtual MIDI cable to the track of PRA32-U2 Native in the host


## Limitations

- "Write Parameters to Program" (CC #87 and #106) writes the program to the memory only,
  so the written programs are lost when the plugin (application) is closed
    - PRA32-U2 Editor stores the user programs in the Web browser
- The control panel (PRA32-U2/P) and the step sequencer are not supported
- MIDI output is not supported


## How to Build

- Requirements: CMake 3.22 or later, a C++17 compiler, and Git (JUCE is downloaded by CMake)
    - To use a local JUCE checkout, add `-DPRA32_U2_NATIVE_JUCE_DIR=<path to JUCE>` to the configure command
- Windows (Visual Studio 2022): run "PRA32-U2-Native-build-windows.bat" in this folder, or:

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

- macOS (Xcode):

```
cmake -S . -B build -G Xcode
cmake --build build --config Release
```

- Linux (e.g. Ubuntu; libasound2-dev, libfreetype-dev, libfontconfig1-dev, libx11-dev, libxrandr-dev,
  libxinerama-dev, libxcursor-dev, libxcomposite-dev, and libxext-dev are needed):

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

- The outputs are in "build/PRA32_U2_Native_artefacts/Release/" ("VST3/PRA32-U2 Native.vst3" and "Standalone/")
- The source files
    - "Source/PRA32U2Engine.cpp": the only file that includes the PRA32-U2 core
    - "Source/PRA32U2NativeCompat.h": the compatibility definitions for PCs
      (e.g. `__attribute__((...))` is removed for MSVC)
    - "Source/PRA32U2Resampler.*": the resampler from 48 kHz
    - "Source/PRA32U2Renderer.*": the engine at 48 kHz + the resampler + the MIDI event scheduling
    - "Source/PluginProcessor.*": the JUCE plugin (VST3 / Standalone)
    - "Source/PluginEditor.*": the editor of the Standalone


## License

- The source files in this folder (as well as PRA32-U2): [CC0 1.0](../LICENSE)
- The binaries of PRA32-U2 Native: [GNU AGPLv3](LICENSE-AGPLv3.txt), because they include the JUCE framework
  (licensed under the AGPLv3; the commercial JUCE licence is not used)
    - The source code: this folder of <https://github.com/risgk/digital-synth-pra32-u2>
    - JUCE 8.0.15: <https://github.com/juce-framework/JUCE>
    - The VST3 SDK (included in JUCE): MIT License, Copyright (c) 2025, Steinberg Media Technologies GmbH
    - VST is a registered trademark of Steinberg Media Technologies GmbH
- The binary packages (zip files) include "LICENSE-AGPLv3.txt", this "README.md", the license of JUCE
  ("LICENSE-JUCE.md", "LICENSE.md" in JUCE), and the license of the VST3 SDK
  ("LICENSE-VST3-SDK.txt", "modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt" in JUCE)
