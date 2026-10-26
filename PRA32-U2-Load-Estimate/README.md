# PRA32-U2 Load Estimate

- Estimates the load of PRA32-U2 on Raspberry Pi Pico 2 (RP2350) without the hardware, from the UF2 build of the sketch,
  and keeps the estimate of the latest version below, so that a change that makes PRA32-U2 heavier is seen
  (replaced at each release; the earlier ones are in the history of this file)
- "pra32-u2-load-estimate.sh" builds PRA32-U2, PRA32-U2/M, PRA32-U2/P, and PRA32-U2/M/P with arduino-cli (as the GitHub Actions workflow builds the UF2 files),
  estimates the load of PRA32-U2 (the lightest) and PRA32-U2/M/P (which has all of the editions) with "pra32-u2-load-estimate.py", and prints the tables below with the RAM of all 4;
  PRA32-U2/M and PRA32-U2/P differ from these little (as estimated at v3.17.1)
    - All 4 are built with Debug Print (USB Serial), as measured on the device, so their RAM is a little more than that of the UF2 files (the buffer of Debug Print);
      PRA32-U2/P and PRA32-U2/M/P with all of the keys of the panel (SW1-6, with SW3-5 that the sketch leaves out by default), released
    - The paths of the files of the core in the asserts are built as "/rp2040/..." (`-ffile-prefix-map`), so that the estimate is the same wherever it is built
      (the length of the path shifts it by about 1%; the estimate of v3.17.5 below is made again with it)
    - The builds and the estimates run in parallel, as many at a time as the CPUs (about 10 min with 4 CPUs)

    ```
    sh PRA32-U2-Load-Estimate/pra32-u2-load-estimate.sh
    ```

    - Tested on Linux (as on the GitHub Actions runners); macOS should work too, and Windows with WSL (the script is for `sh`)
    - Needs [arduino-cli](https://arduino.github.io/arduino-cli/) with the versions of [Modification Guide](../PRA32-U2-Modification-Guide.md), as the workflow installs them:

        ```
        arduino-cli core update-index --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
        arduino-cli core install rp2040:rp2040@6.3.0 --additional-urls https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
        arduino-cli lib install "MIDI Library@5.0.2"
        ```

    - And Python 3 with `pip install unicorn capstone pyelftools` (Unicorn 2)
    - Or run the GitHub Actions workflow "PRA32-U2 Build" with `load_estimate`: the table is in the summary of the run
    - "pra32-u2-load-estimate.py" estimates a case of its own, e.g. `python3 PRA32-U2-Load-Estimate/pra32-u2-load-estimate.py PRA32-U2-Load-Estimate/build/u2/Digital-Synth-PRA32-U2.ino.elf --program 0 --cc 102=26,35=127 --profile`
        - For PRA32-U2/P (and /M/P), `--adc 8` turns the knobs all the way and back every 8 loops, and `--keys 19 --keys-held 21` presses Prog + (SW4)
          as fast as the panel takes it (16 loops of each 48) while holding Shift (SW5), with `--loops 64` (see `--help`);
          on PRA32-U2/M/P with the 4 Synths, they add only about 0.2% to the maximum (v3.17.4), so they are not in the cases below
- How it works
    - The sketch runs on two instances of Unicorn (an instruction emulator: Cortex-M33, Thumb-2, and the FPU), as core 0 and core 1, which share the flash and the SRAM;
      the cores run by turns, a few hundred cycles at a time, so that one waits for the other as on the device
    - The cycles are counted by a first-order cost model of Cortex-M33 (1 for most instructions, 2 for a load, ...), at 153.6 MHz
    - Core 0: the cycles of `loop()`, from its start to its return, against the 1333 us that a loop (64 frames at 48 kHz) lasts; the output never blocks
    - Core 1: the cycles it is busy, but while it waits for core 0
    - The estimate is for comparison: it leaves out the waits of the flash (XIP), the contention of the cores on the bus, the interrupts (e.g. USB),
      and the I2C transfers of the display of PRA32-U2/P, and counts a load at 2 cycles; compare it with Debug Print on the device
      (see [Development](../PRA32-U2-Development.md)) when it matters
- The cases (MIDI Channel 1 is the Basic Channel)
    - The notes are 60, 64, 67, and 71 (velocity 100), held; the loops are measured after 8 of warm-up
    - Osc 1 Wave Sqr and Delay Mode Rev are the heaviest of the waves and the Delay Modes, as in #5 Sync Lead (Osc 1 Wave Sqr) with Delay Mode Rev (CC #35 = 127)
    - "Poly": #5 Sync Lead in Poly (Voice Mode, CC #18 = 0), the heaviest for PRA32-U2
    - "4 Synths" (PRA32-U2/M/P only): the Main Synth and the 3 Sub Synths (Channels 1-4) with #5 Sync Lead in Mono, played together by MIDI Channel 16 (Layering),
      the heaviest for PRA32-U2/M/P
    - "by UART MIDI (Running Status) and USB MIDI": the bytes arrive over and over, with no gap, while the loops run, by UART MIDI at 31250 bps (about 4.2 bytes a loop)
      with Running Status (the status byte once, and a Note Off as a Note On of velocity 0), the most messages a loop, and by USB MIDI
      as fast as PRA32-U2 reads them (5 bytes a loop), whose packets have the status byte each
        - Note On and Off of the note 72 are what a player sends
        - A Program Change, which sets all the parameters (of the 4 Synths, by MIDI Channel 16), is the heaviest message: back to back, the heaviest case,
          PRA32-U2 processes the last of them that arrive one after another, a loop (one synth a loop for PRA32-U2/M/P)
        - Program Change by CC (#112-119, as the programs #0-7, when the value goes up to 64 or above), whose value goes 127 and 0 by turns,
          sets all the parameters too: PRA32-U2 processes it as a Program Change (since v3.17.5)
        - So that the sound stays the heaviest, "Poly" (or "4 Synths") is written to the user program #0 (and #1-3 for the Sub Synths) by Write Program first,
          for the time of the estimate only (`--write`, not into the flash)

## Estimates

The maximum of core 0 / core 1, of 1333 us. A maximum over 100% is marked with "⚠", and one over 95% with "△"
(and shown as a warning in the workflow): check such a case on the device with Debug Print.
The load of each module is the average of core 0 / core 1 (`--modules`), in "Poly" for PRA32-U2 and in "4 Synths" for PRA32-U2/M/P:
"Synth" is the rest of the synth (the voices, the parameters, and the mixing), "Sketch" is `loop()` and `loop1()` (MIDI and the panel),
and "Other" is the rest (e.g. Debug Print).

#### v3.17.5

| Case (core 0 / core 1, max) | PRA32-U2 | PRA32-U2/M/P |
| :-- | --: | --: |
| #0 Synth Pad, 4 notes | 54% / 51% | 58% / 53% |
| Poly: #5 Sync Lead (Osc 1 Wave Sqr) in Poly with Delay Mode Rev, 4 notes | 73% / 64% | 75% / 63% |
| Poly, with Note On and Off of another note by UART MIDI (Running Status) and USB MIDI | 74% / 65% | 76% / 62% |
| Poly, with Program Change #0 (Poly itself) back to back by UART MIDI (Running Status) and USB MIDI | 76% / 64% | 79% / 62% |
| Poly, with Program Change by CC #112 (#0) back to back by UART MIDI (Running Status) and USB MIDI | 75% / 64% | 79% / 62% |
| 4 Synths: #5 Sync Lead (Mono, and Delay Mode Rev) by MIDI Channel 16 (Layering), 4 notes | - | 86% / 84% |
| 4 Synths, with Note On and Off of another note by MIDI Channel 16 by UART MIDI (Running Status) and USB MIDI | - | 90% / 85% |
| 4 Synths, with Program Change #0 (4 Synths itself) by MIDI Channel 16 back to back by UART MIDI (Running Status) and USB MIDI | - | 90% / 84% |
| 4 Synths, with Program Change by CC #112 (#0) by MIDI Channel 16 back to back by UART MIDI (Running Status) and USB MIDI | - | 75% / 68% |

⚠: over 100% (1333 us), △: over 95%

| Module (core 0 / core 1, avg) | PRA32-U2, Poly | PRA32-U2/M/P, 4 Synths |
| :-- | --: | --: |
| Osc | 44.6% / 28.9% | 37.9% / 37.6% |
| Filter | 12.7% / 6.8% | 10.7% / 10.7% |
| Synth | 6.7% / 7.0% | 22.7% / 6.2% |
| DelayFx | 0.0% / 8.5% | 0.0% / 8.5% |
| Sketch | 2.6% / 3.4% | 7.2% / 0.1% |
| OutputCompressor | 0.0% / 4.2% | 0.0% / 4.2% |
| ChorusFx | 0.0% / 3.7% | 0.0% / 3.7% |
| Amp | 2.2% / 1.1% | 2.1% / 2.1% |
| EG | 1.4% / 0.0% | 0.6% / 0.6% |
| LFO | 1.2% / 0.0% | 1.9% / 1.7% |
| Panner | 0.6% / 0.0% | 1.2% / 1.1% |
| NoiseGen | 0.3% / 0.0% | 0.6% / 0.6% |
| OutputFader | 0.2% / 0.0% | 0.2% / 0.0% |
| Other | 0.1% / 0.1% | 0.1% / 6.1% |

| RAM (global variables) | PRA32-U2 | PRA32-U2/M | PRA32-U2/P | PRA32-U2/M/P |
| :-- | --: | --: | --: | --: |
| | 384404 (73%) | 483968 (92%) | 404984 (77%) | 505872 (96%) |

On the device (v3.17.5, PRA32-U2/M/P, Debug Print `max(loop)`): 4 Synths 1214 us (91%; `min(loop)` 1192 us)
