# PRA32-U2 Load Estimate

- Estimates the load of PRA32-U2 on Raspberry Pi Pico 2 (RP2350) without the hardware, from the UF2 build of the sketch,
  and keeps the estimates of each version below, so that a change that makes PRA32-U2 heavier is seen
- "pra32-u2-load-estimate.sh" builds PRA32-U2, PRA32-U2/M, PRA32-U2/P, and PRA32-U2/M/P with arduino-cli (as the GitHub Actions workflow builds the UF2 files),
  estimates the load of PRA32-U2 (the lightest) and PRA32-U2/M/P (which has all of the editions) with "pra32-u2-load-estimate.py", and prints the tables below with the RAM of all 4;
  PRA32-U2/M and PRA32-U2/P differ from these little (v3.17.1, below)

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
    - Or run the GitHub Actions workflow "PRA32-U2 Build" with `load_estimate` (about 15 min): the table is in the summary of the run
    - "pra32-u2-load-estimate.py" estimates a case of its own, e.g. `python3 PRA32-U2-Load-Estimate/pra32-u2-load-estimate.py PRA32-U2-Load-Estimate/build/u2/Digital-Synth-PRA32-U2.ino.elf --program 0 --cc 102=26,35=127 --profile`
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
    - Osc 1 Wave Sqr (CC #102 = 26) and Delay Mode Rev (CC #35 = 127) are the heaviest of the waves and the Delay Modes
    - "4 Synths" is for PRA32-U2/M/P only: the Main Synth and the 3 Sub Synths in Mono, a note each


## Estimates

The maximum of core 0 / core 1, of 1333 us. A maximum over 100% is marked with "⚠", and one over 95% with "△"
(and shown as a warning in the workflow): check such a case on the device with Debug Print.

#### v3.17.1

| Case (core 0 / core 1, max) | PRA32-U2 | PRA32-U2/M | PRA32-U2/P | PRA32-U2/M/P |
| :-- | --: | --: | --: | --: |
| #0 Synth Pad, 4 notes | 67% / 25% | 68% / 26% | 67% / 26% | 69% / 27% |
| #10 WT Pad, 4 notes | 81% / 35% | 84% / 36% | 81% / 36% | 85% / 37% |
| #0 Synth Pad with Osc 1 Wave Sqr and Delay Mode Rev, 4 notes | 90% / 37% | 90% / 38% | 90% / 38% | 90% / 39% |
| 4 Synths: #4 Saw Lead with Osc 1 Wave Sqr (and Delay Mode Rev), a note each | - | 93% / 60% | - | 94% / 61% |
| RAM (global variables) | 380288 (72%) | 479576 (91%) | 398920 (76%) | 500256 (95%) |
