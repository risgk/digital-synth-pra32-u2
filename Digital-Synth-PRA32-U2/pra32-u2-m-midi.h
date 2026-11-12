#pragma once

// PRA32-U2/M: the MIDI messages of the 4 synths (the Main Synth and the Sub Synths 1-3) by the MIDI channels:
// each synth on Basic Channel + 0 to + 3, the Layering, and the Round Robin (see README.md); the Program Changes
// kept and processed later. Used by "Digital-Synth-PRA32-U2-M.ino" (and its Customization Examples),
// "PRA32-U2-M-Demo", and PRA32-U2/M/P Web, so that they handle the MIDI messages in the same way.
// Defined before it is included: g_midi_ch (the Basic Channel - 1), g_synth, g_sub_1_synth, g_sub_2_synth,
// g_sub_3_synth, g_synth_is_in_polyphonic_mode, and PRA32_U2_ENABLE_LAYERING if used (the Layering and the Round Robin)
// (and PRA32_U2_NUMBER_OF_SYNTHS, after "pra32-u2-control-panel.h", with PRA32_U2_USE_CONTROL_PANEL).
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

// The program for the synth #k of a layered group: the program + k, wrapping around in the user
// programs (#0-15) and in the presets (#16-31) each; #127 Random Control is for all the synths too
static uint8_t __not_in_flash_func(layeringProgramNumber)(uint8_t number, uint32_t k) {
  if (number > PROGRAM_NUMBER_MAX) {
    return number;
  }
  return static_cast<uint8_t>((number & ~USER_PROGRAM_NUMBER_MAX) | ((number + k) & USER_PROGRAM_NUMBER_MAX));
}

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

#if defined(PRA32_U2_ENABLE_LAYERING)
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

#endif  // defined(PRA32_U2_ENABLE_LAYERING)

// The Voice Group Mode (Control Change #80): what the Basic Channel + 4 and + 5 do (see README.md), by the
// value of the Main Synth, a parameter of its programs (those of the Sub Synths are not used):
// 0: Off (none), 1: Ly2 (+ 4: the Layering of the synths 0-1, + 5: 2-3), 2: RLy (+ 4: the Round Robin of 0-1 and
// of 2-3, layered), 3: RR2 (+ 4: the Round Robin of 0-1, + 5: 2-3), 4: RR4 (+ 4: 0-3), and 5: RR3 (+ 4: 1-3, the Presets),
// in the order of the notes at once on + 4; the Basic Channel + 15 is the Layering of the synths 0-3 too, while it is not
// Off. While the Main Synth is in Poly (the Sub Synths are not processed), a group has only the Main Synth (none without it).
// Off: the Basic Channel alone (not + 1 to + 3 either), so that the other channels are free (e.g. for a sequencer)
#if defined(PRA32_U2_ENABLE_LAYERING)  // none without the Layering
static uint8_t s_group_mode = 0;  // Off, and 8 while the Main Synth is in Poly

// The Voice Group Mode by the value of the Main Synth (0-127, 6 ways), with whether the Main Synth is in Poly, at each MIDI
// message (after a Program Change too); the notes of the synths turned off when it changes
static void __not_in_flash_func(updateGroupMode)() {
  uint8_t mode = ((g_synth.current_controller_value(VOICE_GRP_MODE) * 10) + 128) >> 8;
  if (g_synth.is_in_polyphonic_mode()) {
    mode |= 8;
  }
  if (mode == s_group_mode) {
    return;
  }
  boolean poly_changed = ((mode ^ s_group_mode) & 8) != 0;
  s_group_mode = mode;
  for (uint32_t s = 0; s < 4; s++) {
    controlChange(s, ALL_NOTES_OFF, 0);
  }

  // The Sub Synths reset at once when the Main Synth goes in or out of Poly (as the sketch does after the MIDI messages),
  // so that a note right after it (e.g. by a Program Change before it at once) sounds
  if (poly_changed) {
    g_synth_is_in_polyphonic_mode = ((mode & 8) != 0);
    g_sub_1_synth.all_notes_off(true);
    g_sub_2_synth.all_notes_off(true);
    g_sub_3_synth.all_notes_off(true);
  }
  roundRobinReset();
  for (uint32_t i = 0; i < 6; i++) {
    s_rr_pedal[i] = false;
  }
}
#endif  // defined(PRA32_U2_ENABLE_LAYERING)

// What a channel does: none, a synth, a Layering, a Round Robin, or two Round Robins layered (0-1 and 2-3),
// with the first and the last synth
enum ChannelRoute { ROUTE_NONE, ROUTE_SYNTH, ROUTE_LAYERING, ROUTE_ROUND_ROBIN, ROUTE_ROUND_ROBIN_LAYERED };

// The route of the Basic Channel + offset, by Voice Group Mode (mode: 0-5) and whether the Main Synth is in Poly
static ChannelRoute __not_in_flash_func(routeOf)(uint32_t offset, uint32_t mode, boolean poly, uint32_t& first, uint32_t& last) {
  static_cast<void>(mode);
  static_cast<void>(poly);
#if defined(PRA32_U2_ENABLE_LAYERING)
  if ((offset >= 1) && (offset <= 3) && (mode == 0)) {  // Off: the Basic Channel alone, the others free (e.g. for a sequencer)
    return ROUTE_NONE;
  }
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  if (offset <= 3) {
    first = offset;
    last  = offset;
    return ROUTE_SYNTH;
  }
#if defined(PRA32_U2_ENABLE_LAYERING)
  ChannelRoute route = ROUTE_NONE;
  if ((offset == 15) && (mode != 0)) {  // not while Voice Group Mode is Off
    first = 0;
    last  = 3;
    route = ROUTE_LAYERING;
  } else if ((offset == 4) || (offset == 5)) {
    switch (mode) {
    case 1:  // Ly2 (x 2)
      first = (offset - 4) * 2;
      last  = first + 1;
      route = ROUTE_LAYERING;
      break;
    case 2:  // RLy (RR2 layered)
      if (offset == 4) { first = 0; last = 3; route = ROUTE_ROUND_ROBIN_LAYERED; }
      break;
    case 3:  // RR2 (x 2)
      first = (offset - 4) * 2;
      last  = first + 1;
      route = ROUTE_ROUND_ROBIN;
      break;
    case 4:  // RR4
      if (offset == 4) { first = 0; last = 3; route = ROUTE_ROUND_ROBIN; }
      break;
    case 5:  // RR3
      if (offset == 4) { first = 1; last = 3; route = ROUTE_ROUND_ROBIN; }
      break;
    }
  }
  if ((route != ROUTE_NONE) && poly) {  // the Main Synth in Poly: the Main Synth alone, if in the group
    if (first != 0) {
      return ROUTE_NONE;
    }
    last = 0;
    return ROUTE_SYNTH;
  }
  return route;
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  return ROUTE_NONE;
}

static ChannelRoute __not_in_flash_func(channelRoute)(byte channel, uint32_t& first, uint32_t& last) {
#if defined(PRA32_U2_ENABLE_LAYERING)
  updateGroupMode();
  return routeOf(((channel - 1) - g_midi_ch) & 0x0F, s_group_mode & 7, (s_group_mode & 8) != 0, first, last);
#else  // defined(PRA32_U2_ENABLE_LAYERING)
  return routeOf(((channel - 1) - g_midi_ch) & 0x0F, 0, false, first, last);
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
}

#if defined(PRA32_U2_USE_CONTROL_PANEL)
// Whether a channel goes to a synth now (for the panel, which selects only such a channel), by the values of the
// Main Synth at once (not by the state kept at the MIDI messages)
boolean isChannelRouted(byte channel)
{
  uint32_t first, last;
  uint32_t mode = ((g_synth.current_controller_value(VOICE_GRP_MODE) * 10) + 128) >> 8;
  return routeOf(((channel - 1) - g_midi_ch) & 0x0F, mode, g_synth.is_in_polyphonic_mode(), first, last) != ROUTE_NONE;
}
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)

void __not_in_flash_func(handleNoteOn)(byte channel, byte pitch, byte velocity)
{
  processPendingProgramChanges();

  uint32_t first, last;
  switch (channelRoute(channel, first, last)) {
  case ROUTE_SYNTH:
  case ROUTE_LAYERING:
    for (uint32_t s = first; s <= last; s++) {
      noteOnOf(s, pitch, velocity);
    }
    break;
#if defined(PRA32_U2_ENABLE_LAYERING)
  case ROUTE_ROUND_ROBIN:
    roundRobinNoteOn(first, last, pitch, velocity);
    break;
  case ROUTE_ROUND_ROBIN_LAYERED:
    roundRobinNoteOn(0, 1, pitch, velocity);
    roundRobinNoteOn(2, 3, pitch, velocity);
    break;
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  default:
    break;
  }
}

void __not_in_flash_func(handleNoteOff)(byte channel, byte pitch, byte velocity)
{
  processPendingProgramChanges();

  uint32_t first, last;
  switch (channelRoute(channel, first, last)) {
  case ROUTE_SYNTH:
  case ROUTE_LAYERING:
    for (uint32_t s = first; s <= last; s++) {
      noteOffOf(s, pitch, velocity);
    }
    break;
#if defined(PRA32_U2_ENABLE_LAYERING)
  case ROUTE_ROUND_ROBIN:
    roundRobinNoteOff(first, last, pitch, velocity);
    break;
  case ROUTE_ROUND_ROBIN_LAYERED:
    roundRobinNoteOff(0, 1, pitch, velocity);
    roundRobinNoteOff(2, 3, pitch, velocity);
    break;
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  default:
    break;
  }
}

void __not_in_flash_func(handleControlChange)(byte channel, byte number, byte value)
{
#if defined(PRA32_U2_USE_CONTROL_PANEL) && defined(PRA32_U2_USE_PANEL_CHANNEL)
  if ((channel - 1) == ((g_midi_ch + 14) & 0x0F)) {  // the Panel Channel (see PRA32_U2_ControlPanel_on_panel_ch_control_change())
    processPendingProgramChanges();
    PRA32_U2_ControlPanel_on_panel_ch_control_change(number, value);
    return;
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL) && defined(PRA32_U2_USE_PANEL_CHANNEL)


  uint32_t first, last;
  switch (channelRoute(channel, first, last)) {
  case ROUTE_SYNTH:
  case ROUTE_LAYERING:
    for (uint32_t s = first; s <= last; s++) {
      controlChange(s, number, value);
    }
    break;
#if defined(PRA32_U2_ENABLE_LAYERING)
  case ROUTE_ROUND_ROBIN:
    roundRobinControlChange(first, last, number, value);
    break;
  case ROUTE_ROUND_ROBIN_LAYERED:
    roundRobinControlChange(0, 1, number, value);
    roundRobinControlChange(2, 3, number, value);
    break;
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
  default:
    break;
  }
}

void __not_in_flash_func(handleProgramChange)(byte channel, byte number)
{
  // A program is processed later (see processPendingProgramChanges())
  uint32_t first, last;
  switch (channelRoute(channel, first, last)) {
  case ROUTE_SYNTH:
    programChange(first, number);
    break;
  case ROUTE_LAYERING:
    // The program + 0, + 1, ... for the synths of the group
    for (uint32_t s = first; s <= last; s++) {
      programChange(s, layeringProgramNumber(number, s - first));
    }
    break;
  case ROUTE_ROUND_ROBIN:
    // The same program for all the synths of the group, as the voices of a polyphonic synth
    for (uint32_t s = first; s <= last; s++) {
      programChange(s, number);
    }
    break;
  case ROUTE_ROUND_ROBIN_LAYERED:
    // As with the Layering: + 0 for 0-1, and + 1 for 2-3
    programChange(0, number);
    programChange(1, number);
    programChange(2, layeringProgramNumber(number, 1));
    programChange(3, layeringProgramNumber(number, 1));
    break;
  default:
    break;
  }
}

void __not_in_flash_func(handlePitchBend)(byte channel, int bend)
{
  processPendingProgramChanges();

  uint32_t first, last;
  if (channelRoute(channel, first, last) != ROUTE_NONE) {
    for (uint32_t s = first; s <= last; s++) {
      pitchBendOf(s, bend);
    }
  }
}

void __not_in_flash_func(handleAfterTouchPoly)(byte channel, byte note, byte pressure)
{
  processPendingProgramChanges();

  uint32_t first, last;
  if (channelRoute(channel, first, last) != ROUTE_NONE) {
    for (uint32_t s = first; s <= last; s++) {
      afterTouchPolyOf(s, note, pressure);
    }
  }
}

void __not_in_flash_func(handleAfterTouchChannel)(byte channel, byte pressure)
{
  processPendingProgramChanges();

  uint32_t first, last;
  if (channelRoute(channel, first, last) != ROUTE_NONE) {
    for (uint32_t s = first; s <= last; s++) {
      afterTouchChannelOf(s, pressure);
    }
  }
}

// The functions for the control panel (PRA32-U2/M/P, and PRA32-U2/M/P Web with the panel parameters), by the channel
#if defined(PRA32_U2_USE_CONTROL_PANEL)

// The synth of a MIDI channel (the first of a group), for the functions below
template <typename F>
static void forSynthOfChannel(byte channel, F f)
{
  uint32_t first, last;
  if (channelRoute(channel, first, last) == ROUTE_NONE) {
    return;
  }
  switch (first) {
  case 0: f(g_synth      ); break;
  case 1: f(g_sub_1_synth); break;
  case 2: f(g_sub_2_synth); break;
  case 3: f(g_sub_3_synth); break;
  }
}

uint8_t __not_in_flash_func(getCurrentControllerValue)(byte channel, byte number)
{
  uint8_t result = 0;
  forSynthOfChannel(channel, [&](auto& synth) { result = synth.current_controller_value(number); });
  return result;
}

void __not_in_flash_func(getRandUint8Rrray)(byte channel, uint8_t array[8])
{
  forSynthOfChannel(channel, [&](auto& synth) { synth.get_rand_uint8_array(array); });
}

void __not_in_flash_func(writeParametersToProgram)(byte channel, byte number)
{
  processPendingProgramChanges();
  forSynthOfChannel(channel, [&](auto& synth) { synth.write_parameters_to_program(number); });
}

// The MIDI channel of a synth that the panel selects: the 4 synths, the Basic Channel + 4 and + 5
// (the Voice Group Mode), and + 15 (the Layering); see README.md
byte __not_in_flash_func(getTargetMIDICh)(byte synth)
{
  if (synth < 4) {
    return (((g_midi_ch + synth) & 0x0F) + 1);
  }
  static const int8_t s_offsets[PRA32_U2_NUMBER_OF_SYNTHS - 4 + 1] = {
#if defined(PRA32_U2_ENABLE_LAYERING)
    4, 5, 15,
#endif  // defined(PRA32_U2_ENABLE_LAYERING)
    0,
  };
  return (((g_midi_ch + s_offsets[synth - 4]) & 0x0F) + 1);
}

// For Undo Read and Undo Write of the control panel
void getCurrentValues(byte channel, uint8_t values[128 + 128])
{
  processPendingProgramChanges();
  forSynthOfChannel(channel, [&](auto& synth) { synth.get_current_values(values); });
}

void setValues(byte channel, boolean panel, const uint8_t values[128 + 128])
{
  processPendingProgramChanges();
  forSynthOfChannel(channel, [&](auto& synth) { synth.set_values(panel, values); });
}

void getProgramValues(byte channel, byte number, uint8_t values[128 + 128])
{
  forSynthOfChannel(channel, [&](auto& synth) { synth.get_program_values(number, values); });
}

void writeValuesToProgram(byte channel, byte number, const uint8_t values[128 + 128])
{
  forSynthOfChannel(channel, [&](auto& synth) { synth.write_values_to_program(number, values); });
}

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
