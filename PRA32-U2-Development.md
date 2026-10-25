# Digital Synth PRA32-U2 Development v3.17.1

- 2026-10-16 ISGK Instruments
- <https://github.com/risgk/digital-synth-pra32-u2>
- Notes for changing the code of PRA32-U2: how it runs and how to measure it; for the settings, see [Modification Guide](./PRA32-U2-Modification-Guide.md)


## Debug Print

- Uncomment `//#define PRA32_U2_USE_DEBUG_PRINT` in "Digital-Synth-PRA32-U2.ino"
- USB Serial (CDC) by default; comment out `PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL` to use UART0 (GP0: TX, GP1: RX, 115200 bps)
- The times in us, since the last report, against the 1333 us that a loop of 64 frames lasts at 48 kHz:
    - `min(audio)` and `max(audio)`: the processing of the 64 frames by the synth
    - `min(loop)` and `max(loop)`: from the start of the loop (MIDI and the panel) to the end of that processing
- It is printed by the secondary core (core 1), a line at a time, and skipped while the host is not reading


## Runtime

- `loop()` on the primary core (core 0) reads MIDI, and then processes 64 frames (`PRA32_U2_I2S_BUFFER_WORDS`)
  and writes them to the output, which blocks while the output buffer is full: the loop is paced by the audio output
- With `PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING`, core 1 (`loop1()`) takes a part of the processing of each frame on request from core 0, which waits for it;
  core 1 also updates the panel and prints the debug print
- The output buffer is a ring buffer read by DMA without interrupts ("pra32-u2-audio-ring-buffer.h"): 2 * 64 frames (2.7 ms) by default
- The code that runs while playing is put in RAM (`__not_in_flash_func`), so that an XIP cache miss (e.g. after a burst of MIDI or USB) does not delay it


## Processing Time

- A loop must end within 1333 us whatever the parameters, the notes, and the MIDI messages: what matters is the slowest loop, not the average
- So the processing is kept about the same every sample, as far as possible:
    - Branchless: a select rather than a branch (e.g. `branchless_conditional()`), and an index rather than a switch (e.g. the FX Routing into the FX bus)
    - Always calculated: e.g. the gain of the Output Fader, the division of the Output Limiter, and the Reverb lengths even while the Delay Time does not move;
      the Output Compressor has no division, and no branch that skips or adds work
    - Spread: the control processing (12 kHz, once per 4 samples) is shared out among the 4 samples by `m_count`,
      and the smoothing at 6 kHz takes the amounts and the balances by turns
- Some settings still take more than others (e.g. Osc 1 Wave Sqr and WT, Delay Mode Rev, and PRA32-U2/M with 4 Synths in Mono): check the heaviest with the Load Estimate below


## Load Estimate

- [PRA32-U2 Load Estimate](./PRA32-U2-Load-Estimate/README.md) estimates the load of each core without the hardware, from the UF2 build,
  in a few cases (e.g. the heaviest Osc 1 Wave and Delay Mode), and keeps the estimates of each version


## Writing to the Flash

- Writing the user programs (Write Program) does `EEPROM.write()` at once, and defers `EEPROM.commit()` (about 50-100 ms, which stops both cores)
  to the Output Fader ("pra32-u2-output-fader.h"):
    - Fades out the output (>= 3 ms), writes the silence to the whole ring buffer (the DMA keeps playing it), and mutes the I2S DAC (`PRA32_U2_I2S_DAC_MUTE_OFF_PIN`)
    - Commits, and then unmutes the I2S DAC and fades in the output
- The user programs are kept in the emulated EEPROM (`PRA32_U2_USE_EMULATED_EEPROM`), at the end of the flash


## Files and Tools

- "pra32-u2-generate-sample-midi-stream.rb" (Ruby) makes "pra32-u2-sample-midi-stream.bin", and "pra32-u2-make-sample-wav-file.cc" (GCC) renders it to a WAV file on PC,
  for checking a change of the sound ("pra32-u2-make-sample-wav-file-cc.bat" on Windows)
- GitHub Actions "PRA32-U2 Build" (".github/workflows/pra32-u2.yml", manual runs): builds the UF2 files, PRA32-U2 Native, and PRA32-U2 Web,
  and commits them to the selected branch with `commit_package`
