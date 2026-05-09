#include "Arduino.h"
#include "EEPROM.h"
#include "I2S.h"

EEPROMClass EEPROM;
I2SClass g_i2s_output;

// Missing dependency from Arduino sketch logic
uint8_t g_midi_ch = 0;

void digitalWrite(uint8_t pin, uint8_t val) {}

#define PRA32_U2_USE_EMULATED_EEPROM 1
//#define PRA32_U2_USE_PWM_AUDIO_INSTEAD_OF_I2S 0

#include "pra32-u2-synth.h"
#include <emscripten.h>

PRA32_U2_Synth<false, false, false, 4> synth;

const int BLOCK_SIZE = 4096;
int16_t outputBufferLeft[BLOCK_SIZE];
int16_t outputBufferRight[BLOCK_SIZE];

extern "C" {

EMSCRIPTEN_KEEPALIVE
void initSynth() {
    synth.initialize();
    synth.program_change(15); // Load Initial preset
}

EMSCRIPTEN_KEEPALIVE
void processAudioBlock(int numSamples) {
    int limit = numSamples;
    if (limit > BLOCK_SIZE) limit = BLOCK_SIZE;

    for (int i = 0; i < limit; i++) {
        int16_t right_out = 0;
        int16_t left_out = synth.process(0, 0, right_out);
        outputBufferLeft[i] = left_out;
        outputBufferRight[i] = right_out;
    }
}

EMSCRIPTEN_KEEPALIVE
void noteOn(uint8_t note, uint8_t vel) {
    synth.note_on(note, vel);
}

EMSCRIPTEN_KEEPALIVE
void noteOff(uint8_t note) {
    synth.note_off(note);
}

EMSCRIPTEN_KEEPALIVE
void controlChange(uint8_t cc, uint8_t value) {
    synth.control_change(cc, value);
}

EMSCRIPTEN_KEEPALIVE
int16_t* getLeftBuffer() {
    return outputBufferLeft;
}

EMSCRIPTEN_KEEPALIVE
int16_t* getRightBuffer() {
    return outputBufferRight;
}

} // extern "C"
