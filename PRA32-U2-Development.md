# Digital Synth PRA32-U2 Development v3.17.5

- 2026-10-26 ISGK Instruments
- <https://github.com/risgk/digital-synth-pra32-u2>
- Notes for changing the code of PRA32-U2: how it runs and how to measure it; for the settings, see [Modification Guide](./PRA32-U2-Modification-Guide.md)


## Debug Print

- Uncomment `//#define PRA32_U2_USE_DEBUG_PRINT` in "Digital-Synth-PRA32-U2.ino"
- USB Serial (CDC) by default; comment out `PRA32_U2_DEBUG_PRINT_USE_USB_SERIAL` to use UART0 (GP0: TX, GP1: RX, 115200 bps)
- The times in us, since the last report, against the 1333 us that a loop of 64 frames lasts at 48 kHz:
    - `min(audio)` and `max(audio)`: the processing of the 64 frames by the synth
    - `min(loop)` and `max(loop)`: from the start of the loop (MIDI and the panel) to the end of that processing
- It is printed by the secondary core (core 1), a line at a time, and skipped while the host is not reading
- To measure PRA32-U2/M, /P, or /M/P, build it as "pra32-u2-load-estimate.sh" in "PRA32-U2-Load-Estimate" does with arduino-cli
  (a copy of the "Digital-Synth-PRA32-U2" folder, with "Digital-Synth-PRA32-U2-M.ino" as its .ino for /M, and `PRA32_U2_USE_CONTROL_PANEL` uncommented for /P,
  with `PRA32_U2_USE_DEBUG_PRINT` uncommented), or use the UF2 files that it builds into "PRA32-U2-Load-Estimate/build"


## Runtime

- `loop()` on the primary core (core 0) reads MIDI, and then processes 64 frames (`PRA32_U2_I2S_BUFFER_WORDS`)
  and writes them to the output, which blocks while the output buffer is full: the loop is paced by the audio output
- With `PRA32_U2_USE_2_CORES_FOR_SIGNAL_PROCESSING`, core 1 (`loop1()`) takes a part of the processing of each frame on request from core 0, which waits for it
  (the Voices 2 and 3 of the Main Synth, and the Sub Synths 1 and 3 of PRA32-U2/M); core 1 also updates the panel and prints the debug print
    - The FX (Chorus, Delay, and the Output Compressor) are processed by core 1 too, a frame after the synths and between them: core 0 puts the FX input of each frame
      with the count of the synth for it (`process_fx(fx_bus, count)`), and at the end of the loop waits until all are done, so the output is the same, with no more latency
    - In PRA32-U2/M with the 4 Synths, core 1 is then about as busy as core 0, which waits for it (on the device, 1205 us from 1258 us a loop with the 4 Synths)
- MIDI is read at the start of each loop: 5 bytes of USB MIDI and 8 bytes of UART MIDI, more than the about 4.2 bytes that 31250 bps brings in a loop;
  the messages are processed there, so a burst of MIDI (e.g. from USB MIDI and UART MIDI at the same time) makes that loop a little longer
- The output buffer is a ring buffer read by DMA without interrupts ("pra32-u2-audio-ring-buffer.h"): 2 * 64 frames (2.7 ms) by default
- The code that runs while playing is put in RAM (`__not_in_flash_func`), so that an XIP cache miss (e.g. after a burst of MIDI or USB) does not delay it


## Processing Time

- A loop should end within 1333 us on average, and later only now and then, whatever the parameters, the notes, and the MIDI messages:
  the output buffer (2 * 64 frames) takes up the delay of a slow loop, but not for long.
  As the processing takes about the same time every loop (below), the maximum is what to watch
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
- The user programs are kept in the emulated EEPROM (`PRA32_U2_USE_EMULATED_EEPROM`), at the end of the flash;
  copying a UF2 file (e.g. a new version) keeps them, as the UF2 file does not write that part of the flash
- A program read at power-on is checked only by its first 2 bytes ('U' and its number) and by each value being below 128:
  there is no version or checksum, so a version that changed the meaning of a parameter would read the old programs in the new meaning


## Files and Tools

- [PRA32-U2/M Demo](./PRA32-U2-M-Demo/README.md): "pra32-u2-m-make-wav-file-from-smf.cc" (GCC) renders a Standard MIDI File with PRA32-U2/M to a WAV file on PC
  ("pra32-u2-m-make-wav-file-from-smf-cc.bat" on Windows), for checking a change of the sound:
    - "pra32-u2-sample.mid", which "pra32-u2-generate-sample-smf.rb" (Ruby) makes: the sample of PRA32-U2 (MIDI Channel 1 only, which the Main Synth plays as PRA32-U2 does);
      "pra32-u2-make-sample-wav-file-cc.bat" renders it on Windows
    - The demo song "pra32-u2-m-demo-ode-to-joy.mid": PRA32-U2/M (the 4 synths)

    ```
    cd PRA32-U2-M-Demo
    g++ -O2 -std=gnu++17 pra32-u2-m-make-wav-file-from-smf.cc -o pra32-u2-m-make-wav-file-from-smf
    ./pra32-u2-m-make-wav-file-from-smf ../pra32-u2-sample.mid pra32-u2-sample.wav
    ```

- GitHub Actions "PRA32-U2 Build" (".github/workflows/pra32-u2.yml", manual runs): builds the UF2 files, PRA32-U2 Native, and PRA32-U2 Web,
  and commits them to the selected branch with `commit_package`


## Release

Basically, a release goes as follows:

1. Check it on the device: PRA32-U2 and PRA32-U2/M, whose UF2 files are released (/P and /M/P are built by the users)
2. In PRA32-U2-Change-History.md, set the date of the version (instead of "2026-??-??"), and "Tested with Arduino-Pico version"
3. Change the version: `PRA32_U2_VERSION` in "Digital-Synth-PRA32-U2.ino", "Digital-Synth-PRA32-U2-M.ino", and the 4 files in "customization-examples";
   `APP_NAME_VERSION` in "pra32-u2-editor.html"; `_version` in the 2 preset JSON files; the headings of the documents
   (README.md, README.ja.md, README-PRA32-U2-P.md with its Info page, the guides, PRA32-U2-Native, and PRA32-U2-Web), and the License of README.md;
   and the dates under the headings ("YYYY-MM-DD ISGK Instruments"), all of them at every release
4. Estimate the load with `sh PRA32-U2-Load-Estimate/pra32-u2-load-estimate.sh` (it takes the version from "Digital-Synth-PRA32-U2.ino"),
   and replace the tables in [PRA32-U2 Load Estimate](./PRA32-U2-Load-Estimate/README.md) with those printed
5. Commit them all as "vX.Y.Z"
6. Run "PRA32-U2 Build" on develop with `commit_package` only, for the UF2 files, PRA32-U2 Native, and PRA32-U2 Web
   (`load_estimate` is for an estimate without arduino-cli at hand: the tables are in the summary of the run, and they are committed by hand)
7. Merge develop into main by a pull request
8. Make the GitHub Release: the tag "vX.Y.Z" on main, and the notes "# Digital Synth PRA32-U2 vX.Y.Z", the line of what it is,
   and "## Change History" with the version of this release only; write "#N" (a program, a CC, ...) as "#&#8203;N" (with a zero-width space),
   so that GitHub does not link it to an issue or a pull request
9. Add the next version to PRA32-U2-Change-History.md ("vX.Y.Z (2026-??-??)", with "Tested with Arduino-Pico version")
10. Update PRA32-U2 Editor and PRA32-U2 Web on [risgk.github.io](https://github.com/risgk/risgk.github.io): "digital-synth-pra32-u2/pra32-u2-editor.html" and "pra32-u2-web-synth.js",
    copied from main (commit "Update pra32-u2-editor.html and pra32-u2-web-synth.js (PRA32-U2 vX.Y.Z)")

- In the commit messages too, "#N" is linked to an issue or a pull request: write the name of a program instead (e.g. "Sync Lead"), or "CC 112"
