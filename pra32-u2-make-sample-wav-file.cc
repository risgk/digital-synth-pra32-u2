#define PRA32_U2_ENABLE_POLY_ON_1_CORE

#define PRA32_U2_MIDI_CH (0)  // 0-based


#include <stdint.h>
#include <stdio.h>

typedef signed char boolean;

#define __not_in_flash_func(func) (func)

uint8_t g_midi_ch = PRA32_U2_MIDI_CH;

#include "./Digital-Synth-PRA32-U2/pra32-u2-common.h"
#include "./Digital-Synth-PRA32-U2/pra32-u2-synth.h"
#include "./pra32-u2-midi-in.h"
#include "./pra32-u2-wav-file-out.h"

PRA32_U2_Synth<false, false, true> g_synth;
PRA32_U2_MIDIIn                    g_midi_in;
PRA32_U2_WAVFileOut                g_wav_file_out;

const uint16_t RECORDING_SEC = 60;
const uint16_t SERIAL_SPEED_38400 = 38400;

int main(int argc, char *argv[]) {
  // setup
  g_midi_in.open(g_synth);
  FILE* bin_file = ::fopen(argv[1], "rb");
  g_wav_file_out.open(argv[2], RECORDING_SEC);
  g_synth.initialize();

  // loop
  int c;
  while ((c = ::fgetc(bin_file)) != EOF) {
    g_midi_in.receive_midi_byte(c);
    uint16_t r = SAMPLING_RATE / (SERIAL_SPEED_38400 / 10);
    for (uint16_t i = 0; i < r; i++) {
      PRA32_U2_StereoSample synth_output = g_synth.process(0, 0);
      g_wav_file_out.write(soft_clip_output(synth_output.left), soft_clip_output(synth_output.right));
    }
  }

  // teardown
  g_wav_file_out.close();
  ::fclose(bin_file);

  return 0;
}
