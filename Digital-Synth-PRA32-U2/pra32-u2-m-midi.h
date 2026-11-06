#pragma once

// PRA32-U2/M: the MIDI messages of the 4 synths (the Main Synth and the Sub Synths 1-3) by the MIDI channels:
// each synth on Basic Channel + 0 to + 3, the Layering, and the Round Robin (see README.md); the Program Changes
// kept and processed later. Used by "Digital-Synth-PRA32-U2-M.ino" (and its Customization Examples),
// "PRA32-U2-M-Demo", and PRA32-U2/M Web, so that they handle the MIDI messages in the same way.
// Defined before it is included: g_midi_ch (the Basic Channel - 1), g_synth, g_sub_1_synth, g_sub_2_synth,
// g_sub_3_synth, g_synth_is_in_polyphonic_mode, and PRA32_U2_ENABLE_LAYERING and PRA32_U2_ENABLE_ROUND_ROBIN if used.
// processOnePendingProgramChange() (or processPendingProgramChanges()) is called after the MIDI messages of a
// loop, and roundRobinReset() when the Sub Synths have their notes turned off (as the Main Synth goes in and
// out of Poly)

// The Program Changes that arrive one after another are kept here for each synth (the Main Synth
// and the Sub Synths 1-3), and processed after the MIDI messages of the loop are read, one synth a
// loop (or all of them before the next message for any of the synths, so that the order is kept),
// as one at most of each:
// - A program (#0-31) sets all the parameters of a program, so only the last of them is kept, and
//   a Random Control kept before it is dropped
// - #127 Random Control sets some of them (at random) after the program kept, so many of them in a
//   row are kept as one
static int16_t s_pending_program_change[4] = { -1, -1, -1, -1 };
static boolean s_pending_random_control[4] = { false, false, false, false };

static void __not_in_flash_func(programChangeOf)(uint32_t synth, uint8_t program_number) {
  switch (synth) {
  case 0: g_synth      .program_change(program_number); break;
  case 1: g_sub_1_synth.program_change(program_number); break;
  case 2: g_sub_2_synth.program_change(program_number); break;
  case 3: g_sub_3_synth.program_change(program_number); break;
  }
}

// The ones kept for a synth, if any; true if processed
static boolean __not_in_flash_func(processPendingProgramChangeOf)(uint32_t synth) {
  int16_t program_number = s_pending_program_change[synth];
  boolean random_control = s_pending_random_control[synth];
  if ((program_number < 0) && !random_control) {
    return false;
  }
  s_pending_program_change[synth] = -1;
  s_pending_random_control[synth] = false;
  if (program_number >= 0) {
    programChangeOf(synth, static_cast<uint8_t>(program_number));
  }
  if (random_control) {
    programChangeOf(synth, 127);
  }
  return true;
}

static void __not_in_flash_func(processPendingProgramChanges)() {
  for (uint32_t i = 0; i < 4; ++i) {
    processPendingProgramChangeOf(i);
  }
}

// In a loop, the ones kept of only one of the synths (the next of them by turns) are processed, so
// that a Program Change of the 4 synths at once (e.g. by the Layering) is spread over 4 loops, each
// with the load of one synth
static void __not_in_flash_func(processOnePendingProgramChange)() {
  static uint32_t s_next_synth = 0;
  for (uint32_t k = 0; k < 4; ++k) {
    uint32_t i = (s_next_synth + k) & 0x03;
    if (processPendingProgramChangeOf(i)) {
      s_next_synth = (i + 1) & 0x03;
      return;
    }
  }
}

// The other numbers (#128-129 the panel parameters, which only the panel sends, and #32-126, which
// do nothing) are processed at once, after those kept, so that each takes effect as it did
static void __not_in_flash_func(programChange)(uint32_t synth, uint8_t program_number) {
  if (program_number <= PROGRAM_NUMBER_MAX) {
    s_pending_program_change[synth] = program_number;
    s_pending_random_control[synth] = false;
    return;
  }
  if (program_number == 127) {
    s_pending_random_control[synth] = true;
    return;
  }
  processPendingProgramChanges();
  programChangeOf(synth, program_number);
}

// Program Change by CC (#112-119: the programs #0-7) and Special Random Control (#111: #127) are
// kept as the Program Changes too, when the value goes up from 63 or below to 64 or above, so that a
// stream of them does not overload a loop either; the synths do not get them
static uint8_t s_special_cc_values[4][PC_BY_CC_7 - SP_RAND_CTRL + 1];

static void __not_in_flash_func(controlChange)(uint32_t synth, uint8_t number, uint8_t value) {
  if ((number >= SP_RAND_CTRL) && (number <= PC_BY_CC_7)) {
    uint8_t old_value = s_special_cc_values[synth][number - SP_RAND_CTRL];
    s_special_cc_values[synth][number - SP_RAND_CTRL] = value;
    if ((old_value <= 63) && (value >= 64)) {
      programChange(synth, (number == SP_RAND_CTRL) ? 127 : (number - PC_BY_CC_0));
    }
    return;
  }
  processPendingProgramChanges();
  switch (synth) {
  case 0: g_synth      .control_change(number, value); break;
  case 1: g_sub_1_synth.control_change(number, value); break;
  case 2: g_sub_2_synth.control_change(number, value); break;
  case 3: g_sub_3_synth.control_change(number, value); break;
  }
}

#if defined(PRA32_U2_ENABLE_LAYERING)
// The program for the synth #k of the Layering: the program + k, wrapping around in the user
// programs (#0-15) and in the presets (#16-31) each; #127 Random Control is for all the synths too
static uint8_t __not_in_flash_func(layeringProgramNumber)(uint8_t number, uint32_t k) {
  if (number > PROGRAM_NUMBER_MAX) {
    return number;
  }
  return static_cast<uint8_t>((number & ~USER_PROGRAM_NUMBER_MAX) | ((number + k) & USER_PROGRAM_NUMBER_MAX));
}
#endif  // defined(PRA32_U2_ENABLE_LAYERING)

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
// The Round Robin (see README.md): the note each synth plays, how many times it is on (as the synth counts
// a note, e.g. by 2 keys), the note it played last, and when it started
static const uint8_t RR_NOTE_NONE = 0xFF;
static uint8_t  s_rr_note[4]      = { RR_NOTE_NONE, RR_NOTE_NONE, RR_NOTE_NONE, RR_NOTE_NONE };
static uint8_t  s_rr_note_count[4];
static uint8_t  s_rr_asgn_note[4] = { RR_NOTE_NONE, RR_NOTE_NONE, RR_NOTE_NONE, RR_NOTE_NONE };
static uint32_t s_rr_age[4];
static uint32_t s_rr_count = 0;
// The synth of the last note of each group
static uint32_t s_rr_last[6] = { 3, 3, 3, 3, 3, 3 };

// The index of a group (the first and the last synth): 0: 0-1, 1: 0-3, 3: 1-3, 5: 2-3
static INLINE uint32_t roundRobinGroupIndex(uint32_t first, uint32_t last) {
  return (first << 1) + (last == 3);
}

// The Sustain Pedal of each group, and the synths held by it (not free until it is released)
static boolean s_rr_pedal[6];
static boolean s_rr_sustained[4];

static void __not_in_flash_func(noteOnOf)(uint32_t synth, uint8_t pitch, uint8_t velocity) {
  switch (synth) {
  case 0: g_synth      .note_on(pitch, velocity); break;
  case 1: g_sub_1_synth.note_on(pitch, velocity); break;
  case 2: g_sub_2_synth.note_on(pitch, velocity); break;
  case 3: g_sub_3_synth.note_on(pitch, velocity); break;
  }
}

static uint8_t __not_in_flash_func(voiceAsgnModeOf)(uint32_t synth) {
  switch (synth) {
  case 1:  return g_sub_1_synth.voice_asgn_mode();
  case 2:  return g_sub_2_synth.voice_asgn_mode();
  case 3:  return g_sub_3_synth.voice_asgn_mode();
  default: return g_synth      .voice_asgn_mode();
  }
}

static void __not_in_flash_func(pitchBendOf)(uint32_t synth, int bend) {
  switch (synth) {
  case 0: g_synth      .pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7); break;
  case 1: g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7); break;
  case 2: g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7); break;
  case 3: g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7); break;
  }
}

static void __not_in_flash_func(afterTouchPolyOf)(uint32_t synth, uint8_t note, uint8_t pressure) {
  switch (synth) {
  case 0: g_synth      .after_touch_poly(note, pressure); break;
  case 1: g_sub_1_synth.after_touch_poly(note, pressure); break;
  case 2: g_sub_2_synth.after_touch_poly(note, pressure); break;
  case 3: g_sub_3_synth.after_touch_poly(note, pressure); break;
  }
}

static void __not_in_flash_func(afterTouchChannelOf)(uint32_t synth, uint8_t pressure) {
  switch (synth) {
  case 0: g_synth      .after_touch_channel(pressure); break;
  case 1: g_sub_1_synth.after_touch_channel(pressure); break;
  case 2: g_sub_2_synth.after_touch_channel(pressure); break;
  case 3: g_sub_3_synth.after_touch_channel(pressure); break;
  }
}

static void __not_in_flash_func(noteOffOf)(uint32_t synth, uint8_t pitch, uint8_t velocity) {
  switch (synth) {
  case 0: g_synth      .note_off(pitch, velocity); break;
  case 1: g_sub_1_synth.note_off(pitch, velocity); break;
  case 2: g_sub_2_synth.note_off(pitch, velocity); break;
  case 3: g_sub_3_synth.note_off(pitch, velocity); break;
  }
}

// Not in Poly, where the Sub Synths are not processed. A synth taken from its oldest note has the note
// turned off first, so that it (in Mono) does not go back to the note
static void __not_in_flash_func(roundRobinNoteOn)(uint32_t first, uint32_t last, uint8_t pitch, uint8_t velocity) {
  if (g_synth_is_in_polyphonic_mode) {
    return;
  }
  uint8_t asgn_mode = voiceAsgnModeOf(first);
  uint32_t count = last - first + 1;
  uint32_t chosen = 4;
  for (uint32_t s = first; s <= last; s++) {
    if (s_rr_note[s] == pitch) {
      chosen = s;
    }
  }
  if ((chosen == 4) && ((asgn_mode == 3) || (asgn_mode == 4))) {
    // Reuse the note-off synth that played the note last
    for (uint32_t s = first; s <= last; s++) {
      if ((s_rr_asgn_note[s] == pitch) && (s_rr_note[s] == RR_NOTE_NONE)) {
        chosen = s;
      }
    }
  }
  if (chosen == 4) {
    // From the next to the last in turn (1 and 3), or from the smallest number (2 and 4)
    uint32_t start = 0;
    if ((asgn_mode == 1) || (asgn_mode == 3)) {
      uint32_t group_last = s_rr_last[roundRobinGroupIndex(first, last)];
      start = ((group_last >= first) && (group_last <= last)) ? (group_last - first + 1) : 0;
    }
    for (uint32_t i = 0; (i < count) && (chosen == 4); i++) {
      uint32_t s = first + ((start + i) % count);
      if (s_rr_note[s] == RR_NOTE_NONE) {
        chosen = s;
      }
    }
  }
  if (chosen == 4) {
    chosen = first;
    for (uint32_t s = first + 1; s <= last; s++) {
      if (s_rr_age[s] < s_rr_age[chosen]) {
        chosen = s;
      }
    }
    // As many Note Offs as the Note Ons, so that the synth does not keep the note
    for (uint32_t n = 0; n < s_rr_note_count[chosen]; n++) {
      noteOffOf(chosen, s_rr_note[chosen], 64);
    }
    s_rr_note_count[chosen] = 0;
  }
  noteOnOf(chosen, pitch, velocity);
  if (s_rr_note[chosen] != pitch) {
    // A new note: the oldest is taken first, as in Poly (a note played again on its synth stays as old as it was)
    s_rr_note_count[chosen] = 0;
    s_rr_age[chosen] = ++s_rr_count;
  }
  if (s_rr_note_count[chosen] < 255) {
    s_rr_note_count[chosen]++;
  }
  s_rr_note[chosen] = pitch;
  s_rr_sustained[chosen] = false;
  s_rr_asgn_note[chosen] = pitch;
  s_rr_last[roundRobinGroupIndex(first, last)] = chosen;
}

static void __not_in_flash_func(roundRobinNoteOff)(uint32_t first, uint32_t last, uint8_t pitch, uint8_t velocity) {
  boolean pedal = s_rr_pedal[roundRobinGroupIndex(first, last)];
  for (uint32_t s = first; s <= last; s++) {
    if ((s_rr_note[s] == pitch) && (s_rr_note_count[s] > 0)) {
      // Free when the last Note On of the note is turned off (or held by the Sustain Pedal)
      noteOffOf(s, pitch, velocity);
      s_rr_note_count[s]--;
      if (s_rr_note_count[s] == 0) {
        if (pedal) {
          s_rr_sustained[s] = true;
        } else {
          s_rr_note[s] = RR_NOTE_NONE;
        }
      }
    }
  }
}

// All the synths free, when the Sub Synths have their notes turned off (as the Main Synth goes in and out of Poly)
static void __not_in_flash_func(roundRobinReset)() {
  for (uint32_t s = 0; s < 4; s++) {
    s_rr_note[s] = RR_NOTE_NONE;
    s_rr_note_count[s] = 0;
    s_rr_sustained[s] = false;
  }
}

// The other messages go to all the synths of the group
static void __not_in_flash_func(roundRobinControlChange)(uint32_t first, uint32_t last, uint8_t number, uint8_t value) {
  if (number == SUSTAIN_PEDAL) {
    s_rr_pedal[roundRobinGroupIndex(first, last)] = (value >= 64);
  }
  for (uint32_t s = first; s <= last; s++) {
    controlChange(s, number, value);
    boolean released = (number == SUSTAIN_PEDAL) && (value < 64) && s_rr_sustained[s];
    if ((number >= ALL_SOUND_OFF) || released) {
      s_rr_note[s] = RR_NOTE_NONE;
      s_rr_note_count[s] = 0;
      s_rr_sustained[s] = false;
    }
  }
  if (number >= ALL_SOUND_OFF) {
    s_rr_pedal[roundRobinGroupIndex(first, last)] = false;
  }
}

// The group (the first and the last synth) of a channel; false if none
static boolean __not_in_flash_func(roundRobinGroup)(byte channel, uint32_t& first, uint32_t& last) {
  if ((channel - 1) == ((g_midi_ch - 4) & 0x0F)) {
    first = 0;
    last  = 3;
    return true;
  } else if ((channel - 1) == ((g_midi_ch - 5) & 0x0F)) {
    first = 1;
    last  = 3;
    return true;
  } else if ((channel - 1) == ((g_midi_ch + 6) & 0x0F)) {
    first = 0;
    last  = 1;
    return true;
  } else if ((channel - 1) == ((g_midi_ch + 7) & 0x0F)) {
    first = 2;
    last  = 3;
    return true;
  }
  return false;
}

// The channel of the groups 0-1 and 2-3 layered
static boolean __not_in_flash_func(roundRobinLayered)(byte channel) {
  return ((channel - 1) == ((g_midi_ch + 8) & 0x0F));
}

// The synths of a channel for Pitch Bend and After Touch; false if none
static boolean __not_in_flash_func(roundRobinSynths)(byte channel, uint32_t& first, uint32_t& last) {
  if (roundRobinGroup(channel, first, last)) {
    return true;
  } else if (roundRobinLayered(channel)) {
    first = 0;
    last  = 3;
    return true;
  }
  return false;
}
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)

void __not_in_flash_func(handleNoteOn)(byte channel, byte pitch, byte velocity)
{
  processPendingProgramChanges();

  if ((channel - 1) == g_midi_ch) {
    g_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.note_on(pitch, velocity);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.note_on(pitch, velocity);
    g_sub_1_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.note_on(pitch, velocity);
    g_sub_3_synth.note_on(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.note_on(pitch, velocity);
    g_sub_1_synth.note_on(pitch, velocity);
    g_sub_2_synth.note_on(pitch, velocity);
    g_sub_3_synth.note_on(pitch, velocity);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  uint32_t first, last;
  if (roundRobinGroup(channel, first, last)) {
    roundRobinNoteOn(first, last, pitch, velocity);
  } else if (roundRobinLayered(channel)) {
    roundRobinNoteOn(0, 1, pitch, velocity);
    roundRobinNoteOn(2, 3, pitch, velocity);
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}

void __not_in_flash_func(handleNoteOff)(byte channel, byte pitch, byte velocity)
{
  processPendingProgramChanges();

  if ((channel - 1) == g_midi_ch) {
    g_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.note_off(pitch, velocity);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.note_off(pitch, velocity);
    g_sub_1_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.note_off(pitch, velocity);
    g_sub_3_synth.note_off(pitch, velocity);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.note_off(pitch, velocity);
    g_sub_1_synth.note_off(pitch, velocity);
    g_sub_2_synth.note_off(pitch, velocity);
    g_sub_3_synth.note_off(pitch, velocity);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  uint32_t first, last;
  if (roundRobinGroup(channel, first, last)) {
    roundRobinNoteOff(first, last, pitch, velocity);
  } else if (roundRobinLayered(channel)) {
    roundRobinNoteOff(0, 1, pitch, velocity);
    roundRobinNoteOff(2, 3, pitch, velocity);
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}

void __not_in_flash_func(handleControlChange)(byte channel, byte number, byte value)
{
  if ((channel - 1) == g_midi_ch) {
    controlChange(0, number, value);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    controlChange(1, number, value);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    controlChange(2, number, value);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    controlChange(3, number, value);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    controlChange(0, number, value);
    controlChange(1, number, value);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    controlChange(2, number, value);
    controlChange(3, number, value);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    controlChange(0, number, value);
    controlChange(1, number, value);
    controlChange(2, number, value);
    controlChange(3, number, value);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  uint32_t first, last;
  if (roundRobinGroup(channel, first, last)) {
    roundRobinControlChange(first, last, number, value);
  } else if (roundRobinLayered(channel)) {
    roundRobinControlChange(0, 1, number, value);
    roundRobinControlChange(2, 3, number, value);
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}

void __not_in_flash_func(handleProgramChange)(byte channel, byte number)
{
  // A program is processed later (see processPendingProgramChanges())
  if ((channel - 1) == g_midi_ch) {
    programChange(0, number);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    programChange(1, number);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    programChange(2, number);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    programChange(3, number);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    programChange(0, number);
    programChange(1, layeringProgramNumber(number, 1));
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    programChange(2, number);
    programChange(3, layeringProgramNumber(number, 1));
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    programChange(0, number);
    programChange(1, layeringProgramNumber(number, 1));
    programChange(2, layeringProgramNumber(number, 2));
    programChange(3, layeringProgramNumber(number, 3));
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  // The same program for all the synths of the group, as the voices of a polyphonic synth
  uint32_t first, last;
  if (roundRobinGroup(channel, first, last)) {
    for (uint32_t s = first; s <= last; s++) {
      programChange(s, number);
    }
  } else if (roundRobinLayered(channel)) {
    // As with the Layering: + 0 for 0-1, and + 1 for 2-3
    programChange(0, number);
    programChange(1, number);
    programChange(2, layeringProgramNumber(number, 1));
    programChange(3, layeringProgramNumber(number, 1));
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}

void __not_in_flash_func(handlePitchBend)(byte channel, int bend)
{
  processPendingProgramChanges();

  if ((channel - 1) == g_midi_ch) {
    g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_1_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_2_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
    g_sub_3_synth.pitch_bend((bend + 8192) & 0x7F, (bend + 8192) >> 7);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  uint32_t first, last;
  if (roundRobinSynths(channel, first, last)) {
    for (uint32_t s = first; s <= last; s++) {
      pitchBendOf(s, bend);
    }
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}

void __not_in_flash_func(handleAfterTouchPoly)(byte channel, byte note, byte pressure)
{
  processPendingProgramChanges();

  if ((channel - 1) == g_midi_ch) {
    g_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.after_touch_poly(note, pressure);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.after_touch_poly(note, pressure);
    g_sub_1_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.after_touch_poly(note, pressure);
    g_sub_3_synth.after_touch_poly(note, pressure);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.after_touch_poly(note, pressure);
    g_sub_1_synth.after_touch_poly(note, pressure);
    g_sub_2_synth.after_touch_poly(note, pressure);
    g_sub_3_synth.after_touch_poly(note, pressure);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  uint32_t first, last;
  if (roundRobinSynths(channel, first, last)) {
    for (uint32_t s = first; s <= last; s++) {
      afterTouchPolyOf(s, note, pressure);
    }
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}

void __not_in_flash_func(handleAfterTouchChannel)(byte channel, byte pressure)
{
  processPendingProgramChanges();

  if ((channel - 1) == g_midi_ch) {
    g_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch + 1) & 0x0F)) {
    g_sub_1_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch + 2) & 0x0F)) {
    g_sub_2_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch + 3) & 0x0F)) {
    g_sub_3_synth.after_touch_channel(pressure);
#if defined(PRA32_U2_ENABLE_LAYERING)
  } else if ((channel - 1) == ((g_midi_ch - 3) & 0x0F)) {
    g_synth.after_touch_channel(pressure);
    g_sub_1_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch - 2) & 0x0F)) {
    g_sub_2_synth.after_touch_channel(pressure);
    g_sub_3_synth.after_touch_channel(pressure);
  } else if ((channel - 1) == ((g_midi_ch - 1) & 0x0F)) {
    g_synth.after_touch_channel(pressure);
    g_sub_1_synth.after_touch_channel(pressure);
    g_sub_2_synth.after_touch_channel(pressure);
    g_sub_3_synth.after_touch_channel(pressure);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  }

#if defined(PRA32_U2_ENABLE_ROUND_ROBIN)
  uint32_t first, last;
  if (roundRobinSynths(channel, first, last)) {
    for (uint32_t s = first; s <= last; s++) {
      afterTouchChannelOf(s, pressure);
    }
  }
#endif  // defined(PRA32_U2_ENABLE_ROUND_ROBIN)
}
