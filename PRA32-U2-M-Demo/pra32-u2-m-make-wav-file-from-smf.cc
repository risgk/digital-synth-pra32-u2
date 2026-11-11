// Renders a Standard MIDI File (format 0 or 1) with PRA32-U2/M (the Main Synth and the Sub Synths 1-3,
// with the Layering) to a WAV file (48 kHz, 24 bit, stereo) on PC
// - The synths are processed for each frame in the order of "Digital-Synth-PRA32-U2-M.ino", on one core,
//   and the Sub Synths only while the Main Synth is not in Poly, as on the device
// - The MIDI messages are read at the start of each loop (64 frames), as on the device; a Program Change
//   (or one by CC #111-119) is processed at once, where the device processes it in one of the next loops
// - The output fader is not used (the output is at full level from the start)
// - Basic Channel 1: the MIDI messages are handled by "pra32-u2-m-midi.h", the same as the device: Channels 1-4 for
//   the Main Synth and the Sub Synths 1-3, 16 for the Layering, and 5 and 6 by the Voice Group Mode (Off at first)
//
// g++ -O2 -std=gnu++17 pra32-u2-m-make-wav-file-from-smf.cc -o pra32-u2-m-make-wav-file-from-smf
// ./pra32-u2-m-make-wav-file-from-smf input.mid output.wav

#define PRA32_U2_ENABLE_POLY_ON_1_CORE

#define PRA32_U2_MIDI_CH (0)  // 0-based

#include <stdint.h>
#include <stdio.h>
#include <vector>
#include <algorithm>

typedef signed char boolean;
typedef uint8_t byte;

#define __not_in_flash_func(func) (func)

uint8_t g_midi_ch = PRA32_U2_MIDI_CH;

#include "../Digital-Synth-PRA32-U2/pra32-u2-common.h"
#include "../Digital-Synth-PRA32-U2/pra32-u2-synth.h"
#include "../pra32-u2-wav-file-out.h"

PRA32_U2_Synth<false, false, true, 0, false> g_synth;
PRA32_U2_Synth<true,  false, true, 1, true>  g_sub_1_synth;
PRA32_U2_Synth<true,  false, true, 2, true>  g_sub_2_synth;
PRA32_U2_Synth<true,  false, true, 3, true>  g_sub_3_synth;
PRA32_U2_WAVFileOut                                 g_wav_file_out;

boolean g_synth_is_in_polyphonic_mode = true;

#define PRA32_U2_ENABLE_LAYERING
#include "../Digital-Synth-PRA32-U2/pra32-u2-m-midi.h"

const uint32_t FRAMES_PER_LOOP = 64;
const uint32_t TAIL_SEC        = 1;

struct MIDIEvent {
  uint32_t tick;
  uint32_t order;      // In the file, for the events of the same tick
  uint32_t tempo;      // us per quarter note, for a Set Tempo (status 0xFF)
  uint8_t  status;
  uint8_t  data[2];
};

static uint32_t readVLQ(const std::vector<uint8_t>& smf, size_t& pos) {
  uint32_t value = 0;
  while (pos < smf.size()) {
    uint8_t b = smf[pos++];
    value = (value << 7) | (b & 0x7F);
    if ((b & 0x80) == 0) {
      break;
    }
  }
  return value;
}

static uint32_t readBE(const std::vector<uint8_t>& smf, size_t pos, uint32_t bytes) {
  uint32_t value = 0;
  for (uint32_t i = 0; i < bytes; ++i) {
    value = (value << 8) | smf[pos + i];
  }
  return value;
}

// The events of all the tracks, sorted by the tick (and the order in the file); the end tick in end_tick
static boolean readSMF(const std::vector<uint8_t>& smf, std::vector<MIDIEvent>& events, uint32_t& division, uint32_t& end_tick) {
  if ((smf.size() < 14) || (readBE(smf, 0, 4) != 0x4D546864)) {  // "MThd"
    return false;
  }
  uint32_t tracks = readBE(smf, 10, 2);
  division = readBE(smf, 12, 2);
  if (division & 0x8000) {  // SMPTE time is not supported
    return false;
  }
  size_t pos = 8 + readBE(smf, 4, 4);
  uint32_t order = 0;
  end_tick = 0;
  for (uint32_t t = 0; (t < tracks) && (pos + 8 <= smf.size()); ++t) {
    size_t track_end = pos + 8 + readBE(smf, pos + 4, 4);
    if (readBE(smf, pos, 4) != 0x4D54726B) {  // "MTrk"
      pos = track_end;
      continue;
    }
    pos += 8;
    uint32_t tick = 0;
    uint8_t running_status = 0;
    while (pos < std::min(track_end, smf.size())) {
      tick += readVLQ(smf, pos);
      uint8_t status = smf[pos];
      if (status & 0x80) {
        ++pos;
      } else {
        status = running_status;
      }
      if (status == 0xFF) {  // Meta Event
        uint8_t type = smf[pos++];
        uint32_t length = readVLQ(smf, pos);
        if ((type == 0x51) && (length == 3)) {  // Set Tempo
          events.push_back({ tick, order++, readBE(smf, pos, 3), 0xFF, { 0, 0 } });
        }
        pos += length;
      } else if ((status == 0xF0) || (status == 0xF7)) {  // SysEx (ignored)
        uint32_t length = readVLQ(smf, pos);
        pos += length;
      } else if (status & 0x80) {
        running_status = status;
        uint32_t data_bytes = (((status & 0xF0) == 0xC0) || ((status & 0xF0) == 0xD0)) ? 1 : 2;
        MIDIEvent event = { tick, order++, 0, status, { smf[pos], 0 } };
        if (data_bytes == 2) {
          event.data[1] = smf[pos + 1];
        }
        pos += data_bytes;
        events.push_back(event);
      } else {
        return false;  // Data with no status
      }
    }
    end_tick = std::max(end_tick, tick);
    pos = track_end;
  }
  std::sort(events.begin(), events.end(), [](const MIDIEvent& a, const MIDIEvent& b) {
    return (a.tick != b.tick) ? (a.tick < b.tick) : (a.order < b.order);
  });
  return true;
}

// A MIDI message to the handlers of "pra32-u2-m-midi.h"; a Program Change is processed at once
static void handleMIDIEvent(uint8_t status, uint8_t data1, uint8_t data2) {
  byte channel = (status & 0x0F) + 1;
  switch (status & 0xF0) {
  case 0x80: handleNoteOff(channel, data1, data2); break;
  case 0x90: if (data2 == 0) { handleNoteOff(channel, data1, 64); } else { handleNoteOn(channel, data1, data2); } break;
  case 0xA0: handleAfterTouchPoly(channel, data1, data2); break;
  case 0xB0: handleControlChange(channel, data1, data2); break;
  case 0xC0: handleProgramChange(channel, data1); break;
  case 0xD0: handleAfterTouchChannel(channel, data1); break;
  case 0xE0: handlePitchBend(channel, ((data2 << 7) | data1) - 8192); break;
  }
  processPendingProgramChanges();
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Usage: %s input.mid output.wav\n", argv[0]);
    return 1;
  }

  FILE* smf_file = ::fopen(argv[1], "rb");
  if (smf_file == NULL) {
    printf("Cannot open %s\n", argv[1]);
    return 1;
  }
  std::vector<uint8_t> smf;
  int c;
  while ((c = ::fgetc(smf_file)) != EOF) {
    smf.push_back(static_cast<uint8_t>(c));
  }
  ::fclose(smf_file);

  std::vector<MIDIEvent> events;
  uint32_t division;
  uint32_t end_tick;
  if (!readSMF(smf, events, division, end_tick)) {
    printf("Cannot read %s as a Standard MIDI File\n", argv[1]);
    return 1;
  }

  // The frame of each event, by the tempo (120 BPM until the first Set Tempo)
  std::vector<uint64_t> frames;
  double frame = 0.0;
  double frames_per_tick = (SAMPLING_RATE * 0.5) / division;
  uint32_t last_tick = 0;
  for (const MIDIEvent& event : events) {
    frame += (event.tick - last_tick) * frames_per_tick;
    last_tick = event.tick;
    frames.push_back(static_cast<uint64_t>(frame + 0.5));
    if (event.status == 0xFF) {
      frames_per_tick = (SAMPLING_RATE * (event.tempo / 1000000.0)) / division;
    }
  }
  frame += (end_tick - last_tick) * frames_per_tick;
  uint32_t sec = static_cast<uint32_t>(frame / SAMPLING_RATE) + 1 + TAIL_SEC;
  uint64_t total_frames = static_cast<uint64_t>(sec) * SAMPLING_RATE;

  g_wav_file_out.open(argv[2], sec);
  g_synth.initialize();
  g_sub_1_synth.initialize();
  g_sub_2_synth.initialize();
  g_sub_3_synth.initialize();

  size_t next_event = 0;
  for (uint64_t f = 0; f < total_frames; f += FRAMES_PER_LOOP) {
    while ((next_event < events.size()) && (frames[next_event] <= f)) {
      const MIDIEvent& event = events[next_event++];
      if (event.status != 0xFF) {
        handleMIDIEvent(event.status, event.data[0], event.data[1]);
      }
    }

    boolean mode = g_synth.is_in_polyphonic_mode();
    if (g_synth_is_in_polyphonic_mode != mode) {
      g_synth_is_in_polyphonic_mode = mode;
      g_sub_1_synth.all_notes_off(true);
      g_sub_2_synth.all_notes_off(true);
      g_sub_3_synth.all_notes_off(true);
      roundRobinReset();
    }

    for (uint32_t i = 0; i < FRAMES_PER_LOOP; ++i) {
      PRA32_U2_StereoSample sub_1_synth_output = { 0, 0 };
      PRA32_U2_StereoSample sub_2_synth_output = { 0, 0 };
      PRA32_U2_StereoSample sub_3_synth_output = { 0, 0 };
      if (g_synth_is_in_polyphonic_mode == false) {
        sub_2_synth_output = g_sub_2_synth.process(0, 0);
      }

      PRA32_U2_FxBusSample fx_bus;
      clear_fx_bus(fx_bus);
      g_sub_2_synth.add_to_fx_bus(sub_2_synth_output, fx_bus);

      PRA32_U2_StereoSample synth_output = g_synth.process<false, true>(0, 0);

      if (g_synth_is_in_polyphonic_mode == false) {
        sub_3_synth_output = g_sub_3_synth.process(0, 0);
        sub_1_synth_output = g_sub_1_synth.process(0, 0);
      }
      g_sub_3_synth.add_to_fx_bus(sub_3_synth_output, fx_bus);
      g_sub_1_synth.add_to_fx_bus(sub_1_synth_output, fx_bus);

      g_synth.add_to_fx_bus(synth_output, fx_bus);

      PRA32_U2_StereoSample synth_fx_output = g_synth.process_fx(fx_bus);
      g_wav_file_out.write(synth_fx_output.left, synth_fx_output.right);
    }
  }

  g_wav_file_out.close();

  return 0;
}
