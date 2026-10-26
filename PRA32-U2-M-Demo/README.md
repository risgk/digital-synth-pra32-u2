# PRA32-U2/M Demo

- A demo song of PRA32-U2/M (Multi-Timbre Edition) in 4 parts, as a Standard MIDI File, and a tool that renders a Standard MIDI File with PRA32-U2/M to a WAV file on PC
- "pra32-u2-m-demo-ode-to-joy.mid": "Ode to Joy" (Beethoven, Symphony No. 9; public domain), arranged for PRA32-U2/M (about 55 s)
    - MIDI Channel 1 (Main Synth): Melody, #20 Saw Lead up to the theme, #21 Sync Lead in the middle and the theme, and #28 Fifth Lead in them again
      (Program Changes in the song), with the Modulation Wheel up to 64 from the middle of each long note
    - MIDI Channel 2 (Sub Synth 1): Bass, #22 Synth Bass, with FX Routing Comp
    - MIDI Channel 3 (Sub Synth 2): Arpeggio, #18 FM Piano
    - MIDI Channel 4 (Sub Synth 3): Rhythm, #23 Initial made a drum by CCs: a sine with a pitch drop and noise, whose sound changes with the note,
      the higher the brighter (Filter Key Track) and the shorter (EG Att/Dec Key Track): the kick (note 36), the snare (55), and the hats (108)
    - The presets are used as they are but for the rhythm, with Amp Gain lowered for the balance of the parts
      (Saw Lead 100 to 79, Sync Lead 80 to 63, Fifth Lead 90 to 64, Synth Bass 100 to 82, and FM Piano 110 to 73; the rhythm up from 90 to 127);
      the FX (Chorus, and Delay of an eighth note at 120 BPM) of the Main Synth are for all the synths
    - Play it to PRA32-U2/M (with the Basic Channel 1) by a MIDI player; it sets the programs and the parameters of the 4 synths at the start
- "pra32-u2-m-demo-ode-to-joy.json": the sounds of the song as the user programs #0-11 for PRA32-U2 Editor ("Import All"; #12-15 are Initial)
    - #0-3: Saw Lead, Synth Bass, FM Piano, and Drums (up to the theme); #4-7: Sync Lead and the same (the middle and the theme);
      #8-11: Fifth Lead and the same (them again)
    - With the Layering of PRA32-U2/M, Program Change #0 (or #4, or #8) by MIDI Channel 16 sets the 4 synths to #0-3 (or #4-7, or #8-11) at once
- "pra32-u2-m-demo-ode-to-joy.mp3": "pra32-u2-m-demo-ode-to-joy.mid" rendered by "pra32-u2-m-make-wav-file-from-smf.cc",
  and converted by [FFmpeg](https://ffmpeg.org/) (6.1.1), with the silence at the end cut

    ```
    ffmpeg -i pra32-u2-m-demo-ode-to-joy.wav -t 54.5 -b:a 128k -metadata title="Ode to Joy (PRA32-U2/M Demo)" pra32-u2-m-demo-ode-to-joy.mp3
    ```

- "pra32-u2-m-generate-demo-smf.rb" (Ruby) makes "pra32-u2-m-demo-ode-to-joy.mid"
- "pra32-u2-m-make-wav-file-from-smf.cc" (GCC) renders a Standard MIDI File (format 0 or 1) with PRA32-U2/M to a WAV file (48 kHz, 24 bit, stereo)

    ```
    g++ -O2 -std=gnu++17 pra32-u2-m-make-wav-file-from-smf.cc -o pra32-u2-m-make-wav-file-from-smf
    ./pra32-u2-m-make-wav-file-from-smf pra32-u2-m-demo-ode-to-joy.mid pra32-u2-m-demo-ode-to-joy.wav
    ```

    - GCC (g++) for PC with C++17 is required (no other library); "pra32-u2-m-make-wav-file-from-smf-cc.bat" makes "pra32-u2-m-demo-ode-to-joy.wav" (on Windows, e.g. with g++ of MinGW-w64)
    - The WAV file is in whole seconds, with 1 s or more of silence at the end
    - The synths are processed for each frame in the order of "Digital-Synth-PRA32-U2-M.ino" (on one core), and the Sub Synths only while the Main Synth is not in Poly
    - The MIDI messages are read at the start of each loop (64 frames), as on the device; MIDI Channels 1-4 are for the Main Synth and the Sub Synths 1-3, and 14-16 for the Layering
    - Unlike the device, a Program Change (or one by CC #111-119) is processed at once (the device processes it in one of the next loops), and the output fader is not used
    - It takes about 1 s for a song of 1 min; it can also be used to check that a change of PRA32-U2/M keeps the sound (compare the WAV files)
