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
