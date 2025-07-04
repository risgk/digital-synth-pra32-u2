## Digital Synth PRA32-U2 Change History

- v0.4.0 (2025-06-30):
    - Increase the maximum overtone of Oscillator output in the low frequency range
    - Merge Osc 1 Multi Saw to Saw Wave
    - Enable Osc 1 Morph for Multi Saw
    - Enable Osc 1 Shape and Morph for Sync Square
    - Improve Filter Cutoff frequency accuracy (4x)
    - Change the signal flow: Change the points at which the levels change
    - Extend Delay Time range (1 to 333.3 -> 1 to 340 ms)
    - Revert the specification change of Chorus Mix
    - Change the Preset and the Editor Only Preset programs
    - PRA32-U2 with Panel: Change the specification of Seq Tempo to BPM 56-300
    - PRA32-U2 with Panel: Change analog input parameters
    - Fix the problem that PRA32_U2_USE_DEBUG_PRINT does not work correctly  if PRA32_U2_USE_CONTROL_PANEL is not defined
    - Tested with Arduino-Pico version 4.6.0
- v0.3.0 (2025-05-10):
    - Renew High Pass Filter: No noise when changing Resonance
    - PRA32-U2 with Panel: Easier to change MIDI Ch
    - Tested with Arduino-Pico version 4.5.3
- v0.2.0 (2025-02-17):
    - Change the Preset programs
    - Change the control numbers of Control Changes
    - Change the meanings of the values of Osc 1 and 2 Wave
        - Add Multi Saw to Osc 1 Wave
    - Change the specification of Chorus Mix
    - Simplify the meanings of the values of Control Changes
    - PRA32-U Editor: Add Editor Only Preset programs
    - Improve documentation
    - Tested with Arduino-Pico version 4.4.3
- v0.1.0 (2025-01-19):
    - Specification is similar to PRA32-U v3.1.0
    - Optimize processing for Raspberry Pi Pico 2/RP2350
    - Improve documentation
    - Tested with Arduino-Pico version 4.4.1
- v0.0.1 (2025-01-19):
    - Specification is similar to PRA32-U v3.1.0
    - PRA32-U2 with Panel: Change the circuit and use INPUT_PULLUP (instead of INPUT_PULLDOWN) to avoid RP2350-E9 Erratum
    - Tested with Arduino-Pico version 4.4.1
