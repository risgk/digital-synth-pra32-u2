#pragma once

#include "pra32-u2-common.h"
#include "pra32-u2-control-panel-font-table.h"
#include "pra32-u2-control-panel-page-table.h"
#include "pra32-u2-value-display.h"

#if defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
#include "hardware/i2c.h"
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)

#include <cstdio>
#include <cstring>

static volatile uint32_t s_current_synth = 0;
static volatile uint32_t s_current_program[PRA32_U2_NUMBER_OF_SYNTHS] = {};
static volatile uint32_t s_current_page_group   = PAGE_GROUP_DEFAULT;
static volatile uint32_t s_current_page_index[] = { PAGE_INDEX_DEFAULT_A, PAGE_INDEX_DEFAULT_B, PAGE_INDEX_DEFAULT_C, PAGE_INDEX_DEFAULT_D };

static volatile int32_t  s_adc_current_value[3];
static volatile uint8_t  s_adc_control_value[3];
static volatile uint8_t  s_adc_control_target[3] = { 0xFF, 0xFF, 0xFF };
static volatile boolean  s_adc_control_catched[3];

#if defined(PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT)
static          uint32_t s_prev_key_current_value;
static          uint32_t s_next_key_current_value;
static          uint32_t s_play_key_current_value;
#if defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)
static          boolean  s_play_key_with_shift = false;  // Shift + Play (Panic)
static          boolean  s_play_key_with_next  = false;  // Next + Play (the Step Sequencer started or stopped)
#endif  // defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)

static          uint32_t s_prog_minus_key_current_value;
static          uint32_t s_prog_plus_key_current_value;
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT)

// Panel Play Mode [Key|K+S|Seq] (index 0-2): s_play_mode 0: Key (Key Mode), 1: Seq (Seq Mode), 2: K+S (Key + Seq Mode:
// the Step Sequencer, and the Play key that plays as in Key Mode)
static          uint8_t  s_play_mode                = 0;
static          boolean  s_key_held                 = false;  // the Play key (or Panel Play) playing, in Key and K+S
static          int8_t   s_panel_transpose          = 0;
static          int8_t   s_panel_pit_ofst           = 0;
static          int8_t   s_seq_pit_ofst             = 0;
static          int8_t   s_seq_transpose            = 0;  // Seq Transpose, added to Panel Transpose (the Step Sequencer)
static          uint8_t  s_seq_step_clock_candidate = 12;
static          uint8_t  s_seq_step_clock           = 12;
static          uint8_t  s_seq_gate_time            = 6;
static          int32_t  s_seq_last_step            = 7;
static          uint8_t  s_seq_mode                 = 0;  // Seq Direction: 0: Forward, 1: Reverse, 2: Bounce
// Seq Pattern [Std|4x4]: 4x4 plays Seq Pitch/Velo 0-3 (the pattern), shifted by Seq Pitch/Velo 4-7 in turn (the shifts,
// added to the offsets, and to the velocity); Seq Num Steps is the length of each shift
static          boolean  s_seq_pattern_4x4          = false;
static          uint8_t  s_seq_shift                = 3;  // 0-3: the shift by Seq Pitch/Velo 4-7 (4x4)
static          int8_t   s_seq_mode_dir             = +1;
static          uint8_t  s_seq_on_steps             = 127;
static          uint8_t  s_seq_act_steps            = 127;

static          uint32_t s_index_scale              = 0;

enum PlayingStatus {
  PlayingStatus_Stop = 0,
  PlayingStatus_Playing,
  PlayingStatus_Seq,
};

static          uint32_t s_playing_status = PlayingStatus_Stop;

static          int32_t  s_seq_step               = 31;
static          uint32_t s_seq_sub_step           = 23;
static          uint32_t s_seq_count              = 0;
static          uint32_t s_seq_count_increment    = 0;
static          bool     s_seq_clock_src_external = false;  // the clock now: External (Ext, or Aut after a clock)
// Seq Clock Src Aut: Int at first, Ext at a MIDI Clock, and Int when Seq Tempo is changed (Ext again at the next MIDI
// Clock), so that the internal clock plays when they no longer come (e.g. a DAW stopped)
static          uint8_t  s_seq_clock_src_mode = 0;  // 0: Aut, 1: Int, 2: Ext
static          uint8_t  s_seq_tempo_last     = 0xFF;  // for the changes of Seq Tempo (not the first value)

// The notes that the panel plays: [0] by the Play key (Key and K+S), and [1] by the Step Sequencer (Seq and K+S),
// each on its channel (the channel of the Note On kept, for its Note Off)
static const    uint32_t NOTE_KEY = 0;
static const    uint32_t NOTE_SEQ = 1;
static volatile uint8_t  s_panel_play_note_pitch[2]    = { 60, 60 };
static volatile uint8_t  s_panel_play_note_velocity[2] = { 127, 127 };
static volatile bool     s_panel_play_note_gate[2]     = { false, false };
static volatile bool     s_panel_play_note_trigger[2]  = { false, false };

static          uint8_t  s_panel_playing_note_pitch[2] = { 0xFF, 0xFF };
static          uint8_t  s_panel_playing_note_ch[2]    = { 1, 1 };

static volatile int32_t s_display_draw_counter         = -1;

static char s_display_buffer[8][21 + 1] = {
  "                     ",
  "                     ",
  "                     ",
  "                     ",
  "                     ",
  "                     ",
  "                     ",
  "                     ",
};

static char s_hex_chars[] PRA32_U2_TABLE_IN_RAM(s_hex_chars) = "0123456789ABCDEF";

// The text formatting on the secondary core (between the samples) is done inline,
// in the RAM, instead of by std::sprintf() and std::strcpy() in the flash, which
// would delay the audio processing by XIP cache misses; the text tables are also
// in the RAM (PRA32_U2_TABLE_IN_RAM)

// Same as std::sprintf(text, "%3d", value), or "%+3d" with plus_sign
static INLINE void PRA32_U2_ControlPanel_format_decimal(char* text, int32_t value, bool plus_sign = false) {
  char     buff[12];
  uint32_t pos       = sizeof(buff) - 1;
  uint32_t abs_value = (value < 0) ? (0U - static_cast<uint32_t>(value)) : static_cast<uint32_t>(value);
  buff[pos] = '\0';
  do {
    buff[--pos] = '0' + (abs_value % 10);
    abs_value /= 10;
  } while (abs_value != 0);
  if (value < 0) {
    buff[--pos] = '-';
  } else if (plus_sign) {
    buff[--pos] = '+';
  }
  while (pos > sizeof(buff) - 1 - 3) {
    buff[--pos] = ' ';
  }
  while ((*text++ = buff[pos++]) != '\0') {
    ;
  }
}

// Same as std::sprintf(text, "x%02X", value)
static INLINE void PRA32_U2_ControlPanel_format_hex(char* text, uint8_t value) {
  text[0] = 'x';
  text[1] = s_hex_chars[value >> 4];
  text[2] = s_hex_chars[value & 0x0F];
  text[3] = '\0';
}

// Same as std::strcpy(text, src) for a text of 3 characters or less, padded with '\0' to 4 characters
static INLINE void PRA32_U2_ControlPanel_copy_text(char* text, const char src[5]) {
  text[0] = src[0];
  text[1] = src[1];
  text[2] = src[2];
  text[3] = src[3];
}



extern void handleNoteOn(byte channel, byte pitch, byte velocity);
extern void handleNoteOff(byte channel, byte pitch, byte velocity);
extern void handleControlChange(byte channel, byte number, byte value);
extern void handleProgramChange(byte channel, byte number);
extern uint8_t getCurrentControllerValue(byte channel, byte number);
extern void getRandUint8Rrray(byte channel, uint8_t array[8]);
extern void writeParametersToProgram(byte channel, byte number);
extern byte getTargetMIDICh(byte synth);
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
extern boolean isChannelRouted(byte channel);
extern uint32_t getProgramStep(byte channel);
extern uint32_t getFirstSynth(byte channel);
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
extern void getCurrentValues(byte channel, uint8_t values[128 + 128]);
extern void setValues(byte channel, boolean panel, const uint8_t values[128 + 128]);
extern void getProgramValues(byte channel, byte number, uint8_t values[128 + 128]);
extern void writeValuesToProgram(byte channel, byte number, const uint8_t values[128 + 128]);

// Whether the panel can select a synth (channel) now: one that goes to a synth (e.g. only the Main Synth, + 0, while
// Voice Group Mode is Off; a group without the Main Synth too while the Main Synth is in Poly, for its parameters)
static INLINE boolean PRA32_U2_ControlPanel_target_usable(uint32_t synth) {
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  return (synth == 0) || isChannelRouted(getTargetMIDICh(synth));
#else
  static_cast<void>(synth);
  return true;
#endif
}

// Prev key + Next key: the next synth (channel) that the panel can select, or the previous one with the Shift key
// (Shift + Prev + Next), skipping those that it cannot select now
static __attribute__((unused)) void PRA32_U2_ControlPanel_step_target() {
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  uint32_t step = 1;
#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
  if (digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL) {
    step = PRA32_U2_NUMBER_OF_SYNTHS - 1;  // back by one
  }
#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
  uint32_t synth = s_current_synth;
  for (uint32_t i = 0; i < PRA32_U2_NUMBER_OF_SYNTHS; ++i) {
    synth = (synth + step) % PRA32_U2_NUMBER_OF_SYNTHS;
    if (PRA32_U2_ControlPanel_target_usable(synth)) {
      break;
    }
  }
  s_current_synth = synth;
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
}

static void PRA32_U2_ControlPanel_update_program_display();

// The synth whose program the panel shows for the channel selected: the synth, or the first synth of a group (or of the
// Layering), which takes the program + 0 of a Program Change on the channel
static INLINE uint32_t PRA32_U2_ControlPanel_program_synth() {
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  return getFirstSynth(getTargetMIDICh(s_current_synth));
#else
  return 0;
#endif
}

// Prog - key and Prog + key: the previous or the next user program of the synth (channel) selected, by the number of the
// programs that the channel sets (e.g. 4 on the Layering of all, as the program + 0 to + 3 for the synths), to the multiple
// of it (e.g. #5 -> #8 or #4), so that the programs of the synths layered stay in the same sets (e.g. #0-3, #4-7, ...);
// from a Factory Preset (#16-31), to #0 (Prog +), or to the last set (Prog -, e.g. #12-15)
static __attribute__((unused)) void PRA32_U2_ControlPanel_step_program(boolean up) {
  uint32_t step = 1;
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  step = getProgramStep(getTargetMIDICh(s_current_synth));  // 1, 2, or 4 (a divisor of USER_PROGRAM_NUMBER_MAX + 1)
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  uint32_t synth          = PRA32_U2_ControlPanel_program_synth();
  uint32_t program_number = s_current_program[synth];
  if (program_number > USER_PROGRAM_NUMBER_MAX) {
    program_number = up ? 0 : ((USER_PROGRAM_NUMBER_MAX + 1) - step);
  } else {
    uint32_t aligned = program_number & ~(step - 1);
    if (up) {
      program_number = aligned + step;
    } else if (aligned != program_number) {
      program_number = aligned;
    } else {
      program_number = aligned - step;
    }
    program_number &= USER_PROGRAM_NUMBER_MAX;  // #15 -> #0, and #0 -> #15 (or #12, #14)
  }
  s_current_program[synth] = program_number;  // (also by the Program Change, when the synth takes it)
  handleProgramChange(getTargetMIDICh(s_current_synth), program_number);
  PRA32_U2_ControlPanel_update_program_display();
}

// Undo Read and Undo Write (Page D-17): what the last Read (or Init Panel Prms) and the last Write of
// the panel replaced, kept until the next one (not in the flash); undoing one swaps it back in, so
// that a second Undo redoes it
struct PRA32_U2_ControlPanelUndo {
  uint8_t values[128 + 128];  // Indexed by the control number
  boolean valid;
  boolean panel;              // The panel parameters (Read Panel Prms, Init Panel Prms, Write Panel Prms)
  byte    channel;            // The MIDI channel of the synth (Undo Read)
  byte    program_number;     // #0-15, or 128 for the panel parameters (Undo Write)
};
static PRA32_U2_ControlPanelUndo s_undo_read;
static PRA32_U2_ControlPanelUndo s_undo_write;

// The values of a channel that goes to no synth (e.g. by Voice Group Mode) are not got: 255 left at Voice Mode (or at
// Panel Scale, for the panel parameters in the flash) tells it, and the Undo is not kept nor done then. The panel
// parameters are of the Main Synth, whose channel is taken at the time (the Basic Channel may have moved)
static const uint8_t PRA32_U2_CONTROL_PANEL_UNDO_CHECK = PANEL_SCALE;

static boolean PRA32_U2_ControlPanel_get_current_values(byte channel, uint8_t values[128 + 128]) {
  values[VOICE_MODE] = 255;
  getCurrentValues(channel, values);
  return values[VOICE_MODE] <= 127;
}

static boolean PRA32_U2_ControlPanel_get_program_values(byte channel, byte program_number, uint8_t values[128 + 128]) {
  uint8_t check = (program_number == 128) ? PRA32_U2_CONTROL_PANEL_UNDO_CHECK : VOICE_MODE;
  values[check] = 255;
  getProgramValues(channel, program_number, values);
  return values[check] <= 127;
}

static void PRA32_U2_ControlPanel_keep_for_undo_read(byte channel, boolean panel) {
  s_undo_read.channel = channel;
  s_undo_read.panel   = panel;
  s_undo_read.valid   = PRA32_U2_ControlPanel_get_current_values(channel, s_undo_read.values);
}

static void PRA32_U2_ControlPanel_keep_for_undo_write(byte channel, byte program_number) {
  s_undo_write.channel        = channel;
  s_undo_write.program_number = program_number;
  s_undo_write.panel          = (program_number == 128);
  s_undo_write.valid          = PRA32_U2_ControlPanel_get_program_values(channel, program_number, s_undo_write.values);
}

// A Write by MIDI (Write Parameters to Program, #106, on the channel of a synth; e.g. by PRA32-U2 Editor), before the
// synth writes the program (255: none): kept for Undo Write too, as a Write of the panel (with the Layering, the last synth);
// in the flash, out of line (rare)
static __attribute__((noinline, unused)) void PRA32_U2_ControlPanel_on_write_by_midi(byte channel, uint8_t program_number) {
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  if (program_number != 255) {
    PRA32_U2_ControlPanel_keep_for_undo_write(channel, program_number);
  }
#else
  (void) channel;
  (void) program_number;
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

static void PRA32_U2_ControlPanel_undo_read() {
  if (s_undo_read.valid == false) {
    return;
  }
  byte channel = s_undo_read.panel ? getTargetMIDICh(0) : s_undo_read.channel;
  uint8_t current_values[128 + 128];
  if (PRA32_U2_ControlPanel_get_current_values(channel, current_values) == false) {
    return;
  }
  setValues(channel, s_undo_read.panel, s_undo_read.values);
  std::memcpy(s_undo_read.values, current_values, sizeof(current_values));
}

static void PRA32_U2_ControlPanel_undo_write() {
  if (s_undo_write.valid == false) {
    return;
  }
  byte channel = s_undo_write.panel ? getTargetMIDICh(0) : s_undo_write.channel;
  uint8_t program_values[128 + 128];
  if (PRA32_U2_ControlPanel_get_program_values(channel, s_undo_write.program_number, program_values) == false) {
    return;
  }
  writeValuesToProgram(channel, s_undo_write.program_number, s_undo_write.values);
  std::memcpy(s_undo_write.values, program_values, sizeof(program_values));
}


static INLINE uint8_t PRA32_U2_ControlPanel_get_index_scale()
{
  uint8_t controller_value = g_synth.current_controller_value(PANEL_SCALE    );
  uint8_t index_scale = ((controller_value * 10) + 128) >> 8;
//if (controller_value < 6) { index_scale = controller_value; }
  return index_scale;
}

// The pitch by Panel Scale: the pitch value (1-4 lowest, 124-127 highest) and the offsets (Panel Pitch Ofst, Seq Pitch
// Ofst, and Seq Pitch 4-7 of Seq Pattern 4x4, each -60 to +60) added, in 24 steps an octave; in signed integers and by
// the floor, so that a sum below the lowest goes on down by the octaves (not round to a wrong note), and clamped to 0-127
// (in the RAM, but noinline: one copy for the Step Sequencer, the Play key, and the display)
static __attribute__((noinline)) uint8_t __not_in_flash_func(PRA32_U2_ControlPanel_calc_scaled_pitch)(uint32_t index_scale, uint8_t pitch, int panel_pit_ofst, int seq_pit_ofst, int shift_ofst = 0)
{
  static const uint8_t ary_scales[6][25] PRA32_U2_TABLE_IN_RAM(panel_scale) = {
    { 60, 60, 62, 62, 62, 62, 64, 64, 64, 65, 65, 65,    // Major
      67, 67, 67, 67, 69, 69, 69, 69, 71, 71, 71, 72, 72 },
    { 60, 60, 62, 62, 62, 63, 63, 63, 65, 65, 65, 65,    // Natural Minor
      67, 67, 67, 68, 68, 68, 70, 70, 70, 70, 72, 72, 72 },
    { 60, 60, 62, 62, 62, 63, 63, 63, 65, 65, 65, 65,    // Ascending Melodic Minor
      67, 67, 67, 67, 69, 69, 69, 69, 71, 71, 71, 72, 72 },
    { 60, 60, 62, 62, 62, 62, 64, 64, 64, 64, 64, 67,    // Major Pentatonic
      67, 67, 67, 67, 69, 69, 69, 69, 69, 72, 72, 72, 72 },
    { 60, 60, 60, 63, 63, 63, 63, 63, 65, 65, 65, 65,    // Minor Pentatonic
      67, 67, 67, 67, 67, 70, 70, 70, 70, 70, 72, 72, 72 },
    { 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 66,    // Chromatic
      66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72 },
  };

  if (pitch == 0) {
    return 0xFF;
  }

  if (pitch < 4) {
    pitch = 4;
  } else if (pitch > 124) {
    pitch = 124;
  }

  if (panel_pit_ofst < -60) { panel_pit_ofst = -60; } else if (panel_pit_ofst > +60) { panel_pit_ofst = +60; }
  if (seq_pit_ofst   < -60) { seq_pit_ofst   = -60; } else if (seq_pit_ofst   > +60) { seq_pit_ofst   = +60; }
  if (shift_ofst     < -60) { shift_ofst     = -60; } else if (shift_ofst     > +60) { shift_ofst     = +60; }

  int32_t t            = ((pitch + panel_pit_ofst + seq_pit_ofst + shift_ofst + 3) * 2) + 1;
  int32_t step         = ((t >= 0) ? (t / 5) : -((-t + 4) / 5)) - 3 + 24;           // floor(t / 5) - 3 + 24
  int32_t index_octave = (step >= 0) ? (step / 24) : -((-step + 23) / 24);         // floor(step / 24)
  int32_t index_pitch  = step - (index_octave * 24);

  if (index_scale > 5) {
    index_scale = 5;
  }

  int32_t new_pitch = ary_scales[index_scale][index_pitch] + (index_octave * 12) - 24;

  if (new_pitch < 0) {
    new_pitch = 0;
  } else if (new_pitch > 127) {
    new_pitch = 127;
  }

  return static_cast<uint8_t>(new_pitch);
}

static INLINE uint8_t PRA32_U2_ControlPanel_calc_transposed_pitch(uint8_t pitch, int32_t transpose)
{
  int32_t new_pitch = pitch + transpose;

  if (new_pitch < 0) {
    new_pitch = 0;
  } else if (new_pitch > 127) {
    new_pitch = 127;
  }

  return static_cast<uint8_t>(new_pitch);
}

static INLINE void PRA32_U2_ControlPanel_calc_value_display_pitch(uint8_t pitch, char value_display_text[5])
{
  uint8_t index_scale = PRA32_U2_ControlPanel_get_index_scale();
  uint8_t new_pitch   = PRA32_U2_ControlPanel_calc_scaled_pitch(
                          index_scale, pitch, g_synth.current_controller_value(PANEL_PIT_OFST ) - 64,
                                              g_synth.current_controller_value(SEQ_PIT_OFST   ) - 64);

  if (new_pitch == 0xFF) {
    value_display_text[0] = 'O';
    value_display_text[1] = 'f';
    value_display_text[2] = 'f';
    value_display_text[3] = '\0';
    return;
  }

  new_pitch = PRA32_U2_ControlPanel_calc_transposed_pitch(
    new_pitch, g_synth.current_controller_value(PANEL_TRANSPOSE ) - 64);

  static const char ary[12][5] PRA32_U2_TABLE_IN_RAM(panel_text) = { " C", "C#", " D", "D#", " E", " F", "F#", " G", "G#", " A", "A#", " B" };

  uint32_t quotient  = new_pitch / 12;
  uint32_t remainder = new_pitch % 12;

  value_display_text[0] = ary[remainder][0];
  value_display_text[1] = ary[remainder][1];

  if (quotient == 0) {
    value_display_text[2] = '-';
  } else {
    value_display_text[2] = s_hex_chars[quotient - 1];
  }
}

static INLINE uint32_t PRA32_U2_ControlPanel_calc_bpm(uint8_t tempo_control_value) {
  int32_t bpm = (tempo_control_value * 2) - 8;

  if (bpm > 240) {
    bpm = 240;
  }

  if (bpm < 30) {
    bpm = 30;
  }

  return bpm;
}


static INLINE boolean PRA32_U2_ControlPanel_mode_has_seq() { return s_play_mode != 0; }  // Seq and K+S
static INLINE boolean PRA32_U2_ControlPanel_mode_has_key() { return s_play_mode != 1; }  // Key and K+S

// The channel of the Step Sequencer: the channel that the panel selects + Seq Ch Ofst [+0-+F] (mod 16)
static INLINE uint8_t PRA32_U2_ControlPanel_seq_ch() {
  uint32_t ofst = g_synth.current_controller_value(SEQ_CH_OFST    ) >> 3;
  return (((getTargetMIDICh(s_current_synth) - 1) + ofst) & 0x0F) + 1;
}

// The right end of the top line (columns 18-20), right-aligned: "*" while the Play key plays (Key and K+S), the shift
// (4-7, Seq Pattern 4x4), and the step (0-3 in 4x4, 0-7 in Std) while the Step Sequencer plays (e.g. "*53")
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_update_play_display)() {
  char text[3];
  uint32_t length = 0;

  if (s_key_held && PRA32_U2_ControlPanel_mode_has_key()) {
    text[length++] = '*';
  }

  if (s_playing_status == PlayingStatus_Seq) {
    if (s_seq_pattern_4x4) {
      text[length++] = s_hex_chars[4 + s_seq_shift];
      text[length++] = s_hex_chars[s_seq_step & 0x03];
    } else {
      text[length++] = s_hex_chars[s_seq_step & 0x07];
    }
  }

  for (uint32_t i = 0; i < 3; ++i) {
    s_display_buffer[0][18 + i] = (i >= 3 - length) ? text[i - (3 - length)] : ' ';
  }
}

// A number (0-99) in decimal, left-aligned in 2 columns (e.g. "5 " and "16")
static __attribute__((noinline)) void PRA32_U2_ControlPanel_put_number_left(char* text, uint32_t number) {
  if (number >= 10) {
    text[0] = '0' + (number / 10);
    text[1] = '0' + (number % 10);
  } else {
    text[0] = '0' + number;
    text[1] = ' ';
  }
}

// The channel of the Step Sequencer ("S" and the MIDI channel, 1-16, columns 14-16 of the top line, on the left of the
// step), in Seq and K+S, if not that of the panel
static __attribute__((noinline)) void PRA32_U2_ControlPanel_update_seq_ch_display() {
  if (PRA32_U2_ControlPanel_mode_has_seq() && ((g_synth.current_controller_value(SEQ_CH_OFST    ) >> 3) != 0)) {
    s_display_buffer[0][14] = 'S';
    PRA32_U2_ControlPanel_put_number_left(&s_display_buffer[0][15], PRA32_U2_ControlPanel_seq_ch());
  } else {
    std::memcpy(&s_display_buffer[0][14], "   ", 3);
  }
}

// The channel that the panel selects (PRA32-U2/P: the Basic Channel), as "Ch" and the MIDI channel (1-16), and what it
// is (PRA32-U2/M/P: "[Sy0]" to "[Sy3]" for the synths, Voice Group Mode for "+ 4" and "+ 5", and "[All]" for the Layering),
// columns 0-9 of the second line
static __attribute__((noinline)) void PRA32_U2_ControlPanel_update_ch_display() {
  char last[10];
  std::memcpy(last, &s_display_buffer[1][0], 10);
  std::memcpy(&s_display_buffer[1][0], "Ch        ", 10);
  PRA32_U2_ControlPanel_put_number_left(&s_display_buffer[1][3], getTargetMIDICh(s_current_synth));
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  static const char s_names[PRA32_U2_NUMBER_OF_SYNTHS][4] = {
    "Sy0", "Sy1", "Sy2", "Sy3",
#if (PRA32_U2_NUMBER_OF_SYNTHS > 4)
    "",    "",    "All",  // (the groups by Voice Group Mode below)
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 4)
  };
  static const char s_group_names[6][4] = {"Off", "Ly2", "RLy", "RR2", "RR4", "RR3"};
  const char* name = s_names[s_current_synth];
  if ((s_current_synth == 4) || (s_current_synth == 5)) {
    name = s_group_names[((g_synth.current_controller_value(VOICE_GRP_MODE) * 10) + 128) >> 8];
  }
  s_display_buffer[1][5] = '[';
  std::memcpy(&s_display_buffer[1][6], name, 3);
  s_display_buffer[1][9] = ']';
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  if (std::memcmp(last, &s_display_buffer[1][0], 10) != 0) {
    // Drawn from the start again (as after a page changed), as this line is not drawn again by itself (it changes seldom)
    PRA32_U2_COMPILER_MEMORY_BARRIER();
    s_display_draw_counter = -1;
  }
}

// The program of the channel that the panel selects (with Prog - key or Prog + key), as "#" and the number of the last
// program that its (first) synth took (0-31; not #127, Randomize Synth, which is not a program), columns 0-2 of the top line
static void PRA32_U2_ControlPanel_update_program_display() {
#if defined(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN) || defined(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN)
  char last[3];
  std::memcpy(last, &s_display_buffer[0][0], 3);
  s_display_buffer[0][0] = '#';
  PRA32_U2_ControlPanel_put_number_left(&s_display_buffer[0][1], s_current_program[PRA32_U2_ControlPanel_program_synth()]);
  if (std::memcmp(last, &s_display_buffer[0][0], 3) != 0) {
    // Drawn from the start again (as after a page changed), as this line is not drawn again by itself (it changes seldom)
    PRA32_U2_COMPILER_MEMORY_BARRIER();
    s_display_draw_counter = -1;
  }
#endif  // defined(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN) || defined(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN)
}

static volatile boolean s_program_display_requested = false;

// A synth took a program (#0-31, by a Program Change from MIDI, the panel, or the Prog keys): the program shown for it (in the
// RAM, as the synths call it; the display is updated by PRA32_U2_ControlPanel_update_control(), in the loop)
void __not_in_flash_func(PRA32_U2_ControlPanel_on_program_change)(uint32_t synth_id, uint8_t program_number) {
  if (synth_id < PRA32_U2_NUMBER_OF_SYNTHS) {
    s_current_program[synth_id] = program_number;
    s_program_display_requested = true;
  }
}

static INLINE void PRA32_U2_ControlPanel_update_page() {
  s_adc_control_catched[0] = false;
  s_adc_control_catched[1] = false;
  s_adc_control_catched[2] = false;

  PRA32_U2_ControlPanelPage current_page = g_control_panel_page_table[s_current_page_group][s_current_page_index[s_current_page_group]];

  if (s_play_mode == 1) {  // Seq Mode
    std::memcpy(current_page.control_target_c_name_line_0, "Seq       ", 10);
    std::memcpy(current_page.control_target_c_name_line_1, "Pitch Ofst", 10);
    current_page.control_target_c = SEQ_PIT_OFST   ;
  }

  std::memcpy(&s_display_buffer[2][ 0], current_page.page_name_line_0            , 10);  // (the channel on the line above it)
  std::memcpy(&s_display_buffer[3][ 0], current_page.page_name_line_1            , 10);

  std::memcpy(&s_display_buffer[5][ 0], current_page.control_target_a_name_line_0, 10);
  std::memcpy(&s_display_buffer[6][ 0], current_page.control_target_a_name_line_1, 10);
  s_adc_control_target[0]             = current_page.control_target_a;

  std::memcpy(&s_display_buffer[5][11], current_page.control_target_b_name_line_0, 10);
  std::memcpy(&s_display_buffer[6][11], current_page.control_target_b_name_line_1, 10);
  s_adc_control_target[1]             = current_page.control_target_b;

#if defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)
  std::memcpy(&s_display_buffer[1][11], current_page.control_target_c_name_line_0, 10);
  std::memcpy(&s_display_buffer[2][11], current_page.control_target_c_name_line_1, 10);
  s_adc_control_target[2]             = current_page.control_target_c;
#endif  // defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)

  PRA32_U2_ControlPanel_update_seq_ch_display();
  PRA32_U2_ControlPanel_update_ch_display();
  PRA32_U2_ControlPanel_update_program_display();

  // The secondary core draws the display from the start again, after the characters above are written
  PRA32_U2_COMPILER_MEMORY_BARRIER();
  s_display_draw_counter = -1;
}

static INLINE uint8_t PRA32_U2_ControlPanel_adc_control_value_candidate(uint32_t adc_number) {
  volatile int32_t adc_control_value_candidate;

#if defined(PRA32_U2_USE_CONTROL_PANEL)

#if defined(PRA32_U2_ANALOG_INPUT_REVERSED)
  adc_control_value_candidate = (127 - (s_adc_current_value[adc_number] / PRA32_U2_ANALOG_INPUT_DENOMINATOR));
#else  // defined(PRA32_U2_ANALOG_INPUT_REVERSED)
  adc_control_value_candidate = (s_adc_current_value[adc_number] / PRA32_U2_ANALOG_INPUT_DENOMINATOR);
#endif  // defined(PRA32_U2_ANALOG_INPUT_REVERSED)

#else  // defined(PRA32_U2_USE_CONTROL_PANEL)
  static_cast<void>(adc_number);
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)

  if (adc_control_value_candidate > 127) {
    adc_control_value_candidate = 127;
  }

  if (adc_control_value_candidate < 0) {
    adc_control_value_candidate = 0;
  }

  return adc_control_value_candidate;
}

// Panel MIDI Tx [Off|Clk|All|Nte|C+N|A+N] (index 0-5): what the panel transmits by UART MIDI and USB MIDI: MIDI Clock
// (Clk, All, C+N, and A+N; in Seq Mode), MIDI Start and Stop (All and A+N; in Seq Mode), and the notes that the panel
// plays (Nte, C+N, and A+N; on the channel that the panel selects, e.g. that of a group, as it is)
static INLINE uint32_t PRA32_U2_ControlPanel_midi_tx_index() {
  return ((g_synth.current_controller_value(PANEL_MIDI_TX  ) * 10) + 128) >> 8;
}

static INLINE boolean PRA32_U2_ControlPanel_midi_tx_clock() {
  uint32_t index = PRA32_U2_ControlPanel_midi_tx_index();
  return (index != 0) && (index != 3);
}

static INLINE boolean PRA32_U2_ControlPanel_midi_tx_start_stop() {
  uint32_t index = PRA32_U2_ControlPanel_midi_tx_index();
  return (index == 2) || (index == 5);
}

// The notes transmitted (2 at most for each of the Play key and the Step Sequencer, while a note is changed to another),
// and their channels, so that each Note Off goes where its Note On went (even after the channel or Panel MIDI Tx is changed)
static const uint32_t NUM_TX_NOTES = 4;
static uint8_t s_tx_note_pitch[NUM_TX_NOTES] = { 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t s_tx_note_ch[NUM_TX_NOTES]    = { 0, 0, 0, 0 };

static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_tx_note_off)(uint8_t channel, uint8_t pitch) {
  if (pitch > 127) {
    return;
  }

  for (uint32_t i = 0; i < NUM_TX_NOTES; ++i) {
    if ((s_tx_note_pitch[i] == pitch) && (s_tx_note_ch[i] == channel)) {
#if defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)
      USB_MIDI.sendNoteOff(pitch, 64, s_tx_note_ch[i]);
#endif  // defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)

#if defined(PRA32_U2_USE_UART_MIDI)
      UART_MIDI.sendNoteOff(pitch, 64, s_tx_note_ch[i]);
#endif  // defined(PRA32_U2_USE_UART_MIDI)
      s_tx_note_pitch[i] = 0xFF;
    }
  }
}

static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_tx_note_on)(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  if (PRA32_U2_ControlPanel_midi_tx_index() < 3) {  // Off, Clk, or All
    return;
  }

  uint32_t i = 0;
  while ((i < NUM_TX_NOTES - 1) && (s_tx_note_pitch[i] <= 127)) {
    ++i;
  }
  if (s_tx_note_pitch[i] <= 127) {  // none free (not expected): the last one off
    PRA32_U2_ControlPanel_tx_note_off(s_tx_note_ch[i], s_tx_note_pitch[i]);
  }

#if defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)
  USB_MIDI.sendNoteOn(pitch, velocity, channel);
#endif  // defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)

#if defined(PRA32_U2_USE_UART_MIDI)
  UART_MIDI.sendNoteOn(pitch, velocity, channel);
#endif  // defined(PRA32_U2_USE_UART_MIDI)
  s_tx_note_pitch[i] = pitch;
  s_tx_note_ch[i]    = channel;
}

// The notes that the panel plays: to the synth, and transmitted by Panel MIDI Tx
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_note_on)(uint8_t channel, uint8_t pitch, uint8_t velocity) {
  handleNoteOn(channel, pitch, velocity);
  PRA32_U2_ControlPanel_tx_note_on(channel, pitch, velocity);
}

static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_note_off)(uint8_t channel, uint8_t pitch) {
  handleNoteOff(channel, pitch, 64);
  PRA32_U2_ControlPanel_tx_note_off(channel, pitch);
}

// The notes that the panel plays, off (e.g. before the channel is changed), each on the channel of its Note On
static void PRA32_U2_ControlPanel_notes_off() {
  for (uint32_t n = 0; n < 2; ++n) {
    if (s_panel_playing_note_pitch[n] <= 127) {
      PRA32_U2_ControlPanel_note_off(s_panel_playing_note_ch[n], s_panel_playing_note_pitch[n]);
    }
  }
}

// (in the RAM, as loop(), but noinline: one copy, not to grow the functions that call it)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_process_note_off_on_of)(uint32_t n, uint8_t channel) {
  uint8_t playing_pitch = s_panel_playing_note_pitch[n];
  uint8_t playing_ch    = s_panel_playing_note_ch[n];

  if (playing_pitch <= 127) {
    if (s_panel_play_note_gate[n] == false) {
      PRA32_U2_ControlPanel_note_off(playing_ch, playing_pitch);
      s_panel_playing_note_pitch[n] = 0xFF;
    } else {
      uint8_t pitch = s_panel_play_note_pitch[n];
      if (s_panel_play_note_trigger[n] && (pitch <= 127)) {
        s_panel_play_note_trigger[n] = false;

        boolean same = (pitch == playing_pitch) && (channel == playing_ch);
        if (same) {
          PRA32_U2_ControlPanel_tx_note_off(playing_ch, playing_pitch);  // transmitted: off and then on (the same note)
        }
        PRA32_U2_ControlPanel_note_on(channel, pitch, s_panel_play_note_velocity[n]);
        handleNoteOff(playing_ch, playing_pitch, 64);
        if (!same) {
          PRA32_U2_ControlPanel_tx_note_off(playing_ch, playing_pitch);
        }
        s_panel_playing_note_pitch[n] = pitch;
        s_panel_playing_note_ch[n]    = channel;
      } else {
        s_panel_play_note_trigger[n] = false;
      }
    }
  } else {
    uint8_t pitch = s_panel_play_note_pitch[n];
    if (s_panel_play_note_gate[n] && (pitch <= 127)) {
      s_panel_play_note_trigger[n] = false;

      PRA32_U2_ControlPanel_note_on(channel, pitch, s_panel_play_note_velocity[n]);
      s_panel_playing_note_pitch[n] = pitch;
      s_panel_playing_note_ch[n]    = channel;
    } else {
      s_panel_play_note_trigger[n] = false;
    }
  }
}

static INLINE boolean PRA32_U2_ControlPanel_process_note_off_on() {
  PRA32_U2_ControlPanel_process_note_off_on_of(NOTE_KEY, getTargetMIDICh(s_current_synth));
  PRA32_U2_ControlPanel_process_note_off_on_of(NOTE_SEQ, PRA32_U2_ControlPanel_seq_ch());
  return false;
}

// The pitch of the Play key (Key and K+S): by the panel parameters now (in Key Mode, kept for the Step Sequencer too, as before)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_update_key_pitch)() {
  uint8_t  new_pitch         = g_synth.current_controller_value(PANEL_PLAY_PIT );
  uint8_t  new_velocity      = g_synth.current_controller_value(PANEL_PLAY_VELO);
  uint32_t index_scale       = PRA32_U2_ControlPanel_get_index_scale();
  int8_t   panel_pit_ofst    = g_synth.current_controller_value(PANEL_PIT_OFST ) - 64;
  int8_t   panel_transpose   = g_synth.current_controller_value(PANEL_TRANSPOSE) - 64;

  if (s_play_mode == 0) {  // Key Mode
    s_index_scale     = index_scale;
    s_panel_pit_ofst  = panel_pit_ofst;
    s_panel_transpose = panel_transpose;
    s_seq_pit_ofst    = 0;
  }

  new_pitch = PRA32_U2_ControlPanel_calc_scaled_pitch(index_scale, new_pitch, panel_pit_ofst, 0);

  if (new_pitch == 0xFF) {
    s_panel_play_note_pitch[NOTE_KEY] = 0xFF;
    s_panel_play_note_gate[NOTE_KEY]  = false;
    return;
  }

  new_pitch = PRA32_U2_ControlPanel_calc_transposed_pitch(new_pitch, panel_transpose);

  s_panel_play_note_velocity[NOTE_KEY] = new_velocity;

  if (s_panel_play_note_pitch[NOTE_KEY] != new_pitch) {
    s_panel_play_note_pitch[NOTE_KEY] = new_pitch;
    if (s_key_held) {
      s_panel_play_note_gate[NOTE_KEY]    = true;
      s_panel_play_note_trigger[NOTE_KEY] = true;
    }
  }
}

// The pitch of the Step Sequencer (Seq and K+S): by the panel parameters at the start of the loop
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_update_seq_pitch)(bool progress_seq_step) {
  uint32_t index = s_seq_step & (s_seq_pattern_4x4 ? 0x03 : 0x07);
  uint8_t new_pitch    = g_synth.current_controller_value(SEQ_PITCH_0      + index);
  int32_t new_velocity = g_synth.current_controller_value(SEQ_VELO_0       + index);
  if ((index != 0) && ((1 << (index - 1)) & s_seq_on_steps) == 0) {
    new_velocity = 0;
  }

  int32_t shift_ofst = 0;
  if (s_seq_pattern_4x4) {
    // The shift of the velocity -63 to +63 (0 and 1: -63), to 1-127; a step of the velocity 0 (or not On) stays silent
    // (computed whichever, so that the steps take the same time)
    shift_ofst = g_synth.current_controller_value(SEQ_PITCH_4 + s_seq_shift) - 64;
    int32_t velo_shift   = maximum(static_cast<int32_t>(g_synth.current_controller_value(SEQ_VELO_4 + s_seq_shift)) - 64, -63);
    int32_t shifted_velo = clamp(new_velocity + velo_shift, 1, 127);
    new_velocity = (new_velocity > 0) ? shifted_velo : 0;
  }

  new_pitch = PRA32_U2_ControlPanel_calc_scaled_pitch(s_index_scale, new_pitch, s_panel_pit_ofst, s_seq_pit_ofst, shift_ofst);

  if (new_pitch == 0xFF) {
    s_panel_play_note_pitch[NOTE_SEQ] = 0xFF;
    s_panel_play_note_gate[NOTE_SEQ]  = false;
    return;
  }

  new_pitch = PRA32_U2_ControlPanel_calc_transposed_pitch(new_pitch, s_panel_transpose + s_seq_transpose);

  s_panel_play_note_velocity[NOTE_SEQ] = new_velocity;
  s_panel_play_note_pitch[NOTE_SEQ]    = new_pitch;

  if ((s_playing_status == PlayingStatus_Seq) && progress_seq_step) {
    s_panel_play_note_gate[NOTE_SEQ]    = true;
    s_panel_play_note_trigger[NOTE_SEQ] = true;
  }
}

static INLINE void PRA32_U2_ControlPanel_update_pitch(bool progress_seq_step) {
  if (PRA32_U2_ControlPanel_mode_has_key()) {
    PRA32_U2_ControlPanel_update_key_pitch();
  }
  if (PRA32_U2_ControlPanel_mode_has_seq()) {
    PRA32_U2_ControlPanel_update_seq_pitch(progress_seq_step);
  }
}

// MIDI Clock by Panel MIDI Tx (of the internal clock, and that received with Ext, passed on); Seq Rx St/Sp is for
// receiving MIDI Start and Stop
static INLINE void PRA32_U2_ControlPanel_seq_clock() {
  if (PRA32_U2_ControlPanel_midi_tx_clock()) {
#if defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)
    USB_MIDI.sendRealTime(midi::Clock);
#endif  // defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)

#if defined(PRA32_U2_USE_UART_MIDI)
    UART_MIDI.sendRealTime(midi::Clock);
#endif  // defined(PRA32_U2_USE_UART_MIDI)
  }

  if (s_playing_status != PlayingStatus_Seq) {
    return;
  }

  ++s_seq_sub_step;

  if (s_seq_sub_step >= 24) {
    s_seq_sub_step = 0;
    s_seq_step_clock = s_seq_step_clock_candidate;
  }

  if ((s_seq_sub_step % s_seq_step_clock) == 0) {
    bool update_scale = false;

    do {
      if (s_seq_mode == 0) {  // Forward
        ++s_seq_step;

        if (s_seq_step > s_seq_last_step) {
          s_seq_step = 0;
          update_scale = true;
        }

        s_seq_mode_dir = +1;
      } else if (s_seq_mode == 1) {  // Reverse
        --s_seq_step;

        if (s_seq_step < 0) {
          s_seq_step = s_seq_last_step;
          update_scale = true;
        }

        s_seq_mode_dir = -1;
      } else {  // Bounce
        if (s_seq_mode_dir > 0) {
          ++s_seq_step;

          if (s_seq_step > s_seq_last_step) {
            s_seq_step = s_seq_last_step;
            update_scale = true;
            s_seq_mode_dir = -1;
          } else {
            s_seq_mode_dir = +1;
          }
        } else {
          --s_seq_step;

          if (s_seq_step < 0) {
            s_seq_step = 0;
            update_scale = true;
            s_seq_mode_dir = +1;
          } else {
            s_seq_mode_dir = -1;
          }
        }
      }
    } while (((s_seq_step & (s_seq_pattern_4x4 ? 0x03 : 0x07)) != 0) &&
             (((1 << ((s_seq_step & (s_seq_pattern_4x4 ? 0x03 : 0x07)) - 1)) & s_seq_act_steps) == 0));

    if (update_scale && s_seq_pattern_4x4) {
      // The next shift: Seq Pitch/Velo 4 always, and 5-7 if their steps are active (Seq Act Steps)
      do {
        s_seq_shift = (s_seq_shift + 1) & 0x03;
      } while ((s_seq_shift != 0) && (((1 << (s_seq_shift + 3)) & s_seq_act_steps) == 0));
    }

    PRA32_U2_ControlPanel_update_play_display();

    if (update_scale) {
      s_index_scale     = PRA32_U2_ControlPanel_get_index_scale();
      s_panel_pit_ofst  = g_synth.current_controller_value(PANEL_PIT_OFST ) - 64;
      s_panel_transpose = g_synth.current_controller_value(PANEL_TRANSPOSE) - 64;
      s_seq_pit_ofst    = g_synth.current_controller_value(SEQ_PIT_OFST   ) - 64;
      s_seq_transpose   = g_synth.current_controller_value(SEQ_TRANSPOSE  ) - 64;
    }

    PRA32_U2_ControlPanel_update_pitch(true);
  } else {
    uint32_t seq_gate_time_corrected = static_cast<uint32_t>(s_seq_gate_time) << 0;

    switch (s_seq_step_clock) {
    case 6:
      break;
    case 12:
      seq_gate_time_corrected = static_cast<uint32_t>(s_seq_gate_time) << 1;
      break;
    case 24:
      seq_gate_time_corrected = static_cast<uint32_t>(s_seq_gate_time) << 2;
      break;
    }

    if ((s_seq_sub_step % s_seq_step_clock) >= seq_gate_time_corrected) {
      s_panel_play_note_gate[NOTE_SEQ] = false;
    }
  }
}

static INLINE void PRA32_U2_ControlPanel_seq_start() {
  if (PRA32_U2_ControlPanel_midi_tx_start_stop()) {
#if defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)
    USB_MIDI.sendRealTime(midi::Start);
#endif  // defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)

#if defined(PRA32_U2_USE_UART_MIDI)
    UART_MIDI.sendRealTime(midi::Start);
#endif  // defined(PRA32_U2_USE_UART_MIDI)
  }

  s_playing_status = PlayingStatus_Seq;

  if (s_seq_mode == 0) {  // Forward
    s_seq_step = 31;
    s_seq_mode_dir = +1;
  } else if (s_seq_mode == 1) {  // Reverse
    s_seq_step = 0;
    s_seq_mode_dir = -1;
  } else {  // Bounce
    s_seq_step = 0;
    s_seq_mode_dir = -1;
  }

  s_seq_sub_step = 23;
  s_seq_shift = 3;  // to 0 (Seq Pitch/Velo 4) at the first step
  s_panel_play_note_gate[NOTE_SEQ] = false;
}

static INLINE void PRA32_U2_ControlPanel_seq_stop() {
  if (PRA32_U2_ControlPanel_midi_tx_start_stop()) {
#if defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)
    USB_MIDI.sendRealTime(midi::Stop);
#endif  // defined(PRA32_U2_USE_USB_MIDI) && !defined(PRA32_U2_DISABLE_USB_MIDI_TRANSMITTION)

#if defined(PRA32_U2_USE_UART_MIDI)
    UART_MIDI.sendRealTime(midi::Stop);
#endif  // defined(PRA32_U2_USE_UART_MIDI)
  }

  s_playing_status = PlayingStatus_Stop;
  PRA32_U2_ControlPanel_update_play_display();
  s_panel_play_note_gate[NOTE_SEQ] = false;
}

static INLINE void PRA32_U2_ControlPanel_seq_start_stop() {
  if (s_playing_status == PlayingStatus_Stop) {
    PRA32_U2_ControlPanel_seq_start();
  } else {
    PRA32_U2_ControlPanel_seq_stop();
  }
}

// The Play key (or Panel Play) in Key and K+S: the note on while pressed
static INLINE void PRA32_U2_ControlPanel_key_play(boolean on) {
  s_key_held = on;
  if (s_play_mode == 0) {  // Key Mode
    s_playing_status = on ? PlayingStatus_Playing : PlayingStatus_Stop;
  }
  PRA32_U2_ControlPanel_update_play_display();
  s_panel_play_note_gate[NOTE_KEY]    = on;
  s_panel_play_note_trigger[NOTE_KEY] = on;
}

static INLINE void PRA32_U2_ControlPanel_update_control_seq() {
  if (PRA32_U2_ControlPanel_mode_has_seq() && (s_seq_clock_src_external == false)) {  // Seq and K+S
    s_seq_count += s_seq_count_increment;

    if (s_seq_count >= 7680000 * 2) {
      s_seq_count -= 7680000 * 2;

      PRA32_U2_ControlPanel_seq_clock();
    }
  }
}

// Does an operation (128 + 64 and on: Read, Write, Undo, etc.), from the panel or from the Panel Channel
static __attribute__((noinline, unused)) void PRA32_U2_ControlPanel_execute_operation(uint8_t control_target) {
  if        ((control_target >= RD_PROGRAM_0) && (control_target <= RD_PROGRAM_31)) {
    PRA32_U2_ControlPanel_keep_for_undo_read(getTargetMIDICh(s_current_synth), false);
    handleProgramChange(getTargetMIDICh(s_current_synth), control_target - RD_PROGRAM_0);
  } else if ((control_target >= WR_PROGRAM_0) && (control_target <= WR_PROGRAM_15)) {
    uint8_t program_number_to_write = control_target - WR_PROGRAM_0;
    PRA32_U2_ControlPanel_keep_for_undo_write(getTargetMIDICh(s_current_synth), program_number_to_write);
    writeParametersToProgram(getTargetMIDICh(s_current_synth), program_number_to_write);
  } else if (control_target == RD_PANEL_PRMS) {  // the panel parameters are of the Main Synth (as Write), whatever the panel selects
    PRA32_U2_ControlPanel_keep_for_undo_read(getTargetMIDICh(0), true);
    handleProgramChange(getTargetMIDICh(0), 128);
  } else if (control_target == IN_PANEL_PRMS) {
    PRA32_U2_ControlPanel_keep_for_undo_read(getTargetMIDICh(0), true);
    handleProgramChange(getTargetMIDICh(0), 129);
  } else if (control_target == WR_PANEL_PRMS) {
    PRA32_U2_ControlPanel_keep_for_undo_write(getTargetMIDICh(0), 128);
    g_synth.write_parameters_to_program(128);
  } else if (control_target == UNDO_READ) {
    PRA32_U2_ControlPanel_undo_read();
  } else if (control_target == UNDO_WRITE) {
    PRA32_U2_ControlPanel_undo_write();
  } else if (control_target == SEQ_RAND_PITCH) {
    uint8_t array[8] = {};
    getRandUint8Rrray(getTargetMIDICh(s_current_synth), array);
    for (uint32_t i = 0; i < (s_seq_pattern_4x4 ? 4u : 8u); ++i) {  // 4x4: the pattern (0-3), not the shifts
      g_synth.control_change(SEQ_PITCH_0 + i, array[i] & 0x7Fu);
    }
  } else if (control_target == SEQ_RAND_VELO) {
    uint8_t array[8] = {};
    getRandUint8Rrray(getTargetMIDICh(s_current_synth), array);
    for (uint32_t i = 0; i < 8; ++i) {
      g_synth.control_change(SEQ_VELO_0  + i, array[i] & 0x7Fu);
    }
  } else if (control_target == PANIC_OP) {
    for (uint32_t i = 0; i < NUM_TX_NOTES; ++i) {  // the notes transmitted, off
      PRA32_U2_ControlPanel_tx_note_off(s_tx_note_ch[i], s_tx_note_pitch[i]);
    }
    handleControlChange(getTargetMIDICh(s_current_synth), ALL_SOUND_OFF  , 0);
    handleControlChange(getTargetMIDICh(s_current_synth), RESET_ALL_CTRLS, 0);
    uint8_t seq_ch = PRA32_U2_ControlPanel_seq_ch();
    if (PRA32_U2_ControlPanel_mode_has_seq() && (seq_ch != getTargetMIDICh(s_current_synth))) {  // that of the Step Sequencer too
      handleControlChange(seq_ch, ALL_SOUND_OFF  , 0);
      handleControlChange(seq_ch, RESET_ALL_CTRLS, 0);
    }
  } else if (control_target == RAND_SYNTH_OP) {
    handleProgramChange(getTargetMIDICh(s_current_synth), 127);
  }
}

// The knobs of General Purpose Controller 1-4 (Pages B-24 and B-25), with Assign Mode Rl2 or Rl1: the parameter of Assign #16-#19
// at once (the knob as an absolute value, not as a relative one); the others: the control target itself
static uint8_t PRA32_U2_ControlPanel_absolute_target(uint8_t control_target) {
  if ((control_target >= GEN_PURPOSE_1) && (control_target <= GEN_PURPOSE_4)) {
    uint8_t channel = getTargetMIDICh(s_current_synth);
    uint8_t dst     = getCurrentControllerValue(channel, ASSIGN_16  + (control_target - GEN_PURPOSE_1));
    if ((PRA32_U2_assign_mode(getCurrentControllerValue(channel, ASSIGN_MODE)) >= 2) && !PRA32_U2_assign_dst_off(dst)) {
      return dst;
    }
  }
  return control_target;
}

static INLINE boolean PRA32_U2_ControlPanel_update_control_adc(uint32_t adc_number) {
#if !defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)
  if (adc_number == 2) {
    return false;
  }
#endif  // defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)

  uint8_t adc_control_value_new = PRA32_U2_ControlPanel_adc_control_value_candidate(adc_number);

  if (s_adc_control_value[adc_number] != adc_control_value_new) {
    uint8_t adc_control_value_old = s_adc_control_value[adc_number];
    s_adc_control_value[adc_number] = adc_control_value_new;

#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
    uint32_t shift_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
    if (shift_key_pressed) {
      if        ((adc_control_value_old <= 64) && (adc_control_value_new > 64)) {
        adc_control_value_new = 64;
      } else if ((adc_control_value_old >= 64) && (adc_control_value_new < 64)) {
        adc_control_value_new = 64;
      }
    }
#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)

    uint8_t current_controller_value = adc_control_value_new;
    if        (s_adc_control_target[adc_number] < 128) {
      current_controller_value = getCurrentControllerValue(getTargetMIDICh(s_current_synth), s_adc_control_target[adc_number]);
    } else if (s_adc_control_target[adc_number] < 128 + 64) {
      current_controller_value = g_synth.current_controller_value(s_adc_control_target[adc_number]);
    }

    if ((adc_control_value_old <= current_controller_value) &&
        (adc_control_value_new >= current_controller_value)) {
      s_adc_control_catched[adc_number] = true;
    }

    if ((adc_control_value_old >= current_controller_value) &&
        (adc_control_value_new <= current_controller_value)) {
      s_adc_control_catched[adc_number] = true;
    }

    // General Purpose Controller 1-4 (Pages B-24 and B-25) at once, as they come to the values of the parameters themselves
    // (Assign #16-#19), so that the knob is not turned to their own values first
    if ((s_adc_control_target[adc_number] >= GEN_PURPOSE_1) && (s_adc_control_target[adc_number] <= GEN_PURPOSE_4)) {
      s_adc_control_catched[adc_number] = true;
    }


    if (s_adc_control_target[adc_number] < 128) {
      if (s_adc_control_catched[adc_number]) {
        handleControlChange(getTargetMIDICh(s_current_synth), PRA32_U2_ControlPanel_absolute_target(s_adc_control_target[adc_number]),
                            adc_control_value_new);
      }
    } else if (s_adc_control_target[adc_number] < 128 + 64) {
      if (s_adc_control_catched[adc_number]) {
        g_synth.control_change(s_adc_control_target[adc_number], adc_control_value_new);
      }
    } else {
      // An operation (Read, Write, Undo, etc.): ready at 0-31 [Rdy], and done at 96-127 [Exe]
      static boolean s_ready_to_execute[64] = {};
      uint32_t index = s_adc_control_target[adc_number] - (128 + 64);
      if (index < 64) {
        if (s_adc_control_value[adc_number] <= 31) {
          s_ready_to_execute[index] = true;
        } else if (s_ready_to_execute[index] && (s_adc_control_value[adc_number] >= 96)) {
          PRA32_U2_ControlPanel_execute_operation(s_adc_control_target[adc_number]);
          s_ready_to_execute[index] = false;
        }
      }
    }

    return true;
  }

  return false;
}

#if defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
// Queues a short write to the OLED display into the I2C TX FIFO (16 bytes) and
// returns without waiting for the transfer (about 180 us for 8 bytes at 400 kHz),
// so that the secondary core is not blocked during the audio processing;
// the target address must have been set by a preceding i2c_write_blocking()
static INLINE void PRA32_U2_ControlPanel_write_to_oled_display(const uint8_t* src, size_t len) {
  i2c_hw_t* hw = i2c_get_hw(PRA32_U2_OLED_DISPLAY_I2C);

  // Reading this register clears a previous abort (e.g. NACK), which holds the TX FIFO flushed
  static_cast<void>(hw->clr_tx_abrt);

  // Waits only if the previous transfers are still in the TX FIFO (e.g. during the initialization)
  while (i2c_get_write_available(PRA32_U2_OLED_DISPLAY_I2C) < len) {
    ;
  }

  for (size_t i = 0; i < len; ++i) {
    hw->data_cmd = src[i] | ((i == (len - 1)) ? I2C_IC_DATA_CMD_STOP_BITS : 0);
  }
}
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)

static INLINE void PRA32_U2_ControlPanel_set_draw_position(uint8_t x, uint8_t y) {
#if defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
  uint8_t commands[] = {0x00,  static_cast<uint8_t>(0xB0 + y),
                               static_cast<uint8_t>(0x10 + ((x * 6) >> 4)),
                               static_cast<uint8_t>(0x00 + ((x * 6) & 0x0F))};
  PRA32_U2_ControlPanel_write_to_oled_display(commands, sizeof(commands));
#else  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
  static_cast<void>(x);
  static_cast<void>(y);
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
}

static INLINE void PRA32_U2_ControlPanel_draw_character(uint8_t c) {
#if defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
  uint8_t data[] = {0x40,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  std::memcpy(&data[1], g_control_panel_font_table[c], 6);
  PRA32_U2_ControlPanel_write_to_oled_display(data, sizeof(data));
#else  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
  static_cast<void>(c);
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
}

// The functions below for the values shown, called by PRA32_U2_ControlPanel_calc_value_display() (on the secondary core,
// between the samples), are in the RAM (__not_in_flash_func), so that they cause no XIP cache misses; noinline, as each
// has more than one caller or would be put in the flash if not inlined (GCC, -Os)

// A value of two sides as it works, 2 controller values a step: -64 (0), -62 (1-2), ..., 0 (63-64), ..., +62 (125-126),
// and +64 (127), shown as the letter of the side and the amount (e.g. "N64", "0", "S64"), right-aligned
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_two_sides)(uint8_t controller_value, char minus, char plus, char value_display_text[5]) {
  int32_t  x      = static_cast<int32_t>(controller_value) - 63;
  int32_t  amount = (x >= 0) ? ((x / 2) * 2) : -(((-x + 1) / 2) * 2);  // floor(x / 2) * 2
  char     buff[4];
  uint32_t length = 0;
  if (amount == 0) {
    buff[length++] = '0';
  } else {
    buff[length++] = (amount > 0) ? plus : minus;
    PRA32_U2_ControlPanel_append_decimal(buff, length, (amount > 0) ? amount : -amount);
  }
  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// Osc 1 Shape and Osc 1 Morph, by Osc 1 Wave of the synth that the panel selects (3 characters at most, right-aligned;
// the same as osc1DispValue() of PRA32-U2 Editor): Shape: the pulse width in % (Pls); Morph: the frequency ratio (Sin,
// e.g. "1.5", "16." for 16.5), the wave table (WT), and the Pulse (P) or the 2nd Saw (S) as it works (Pls)
static __attribute__((noinline)) boolean __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_osc_1)(uint8_t control_target, uint8_t controller_value, char value_display_text[5]) {
  uint32_t wave = ((getCurrentControllerValue(getTargetMIDICh(s_current_synth), OSC_1_WAVE) * 10) + 128) >> 8;
  uint32_t v    = controller_value;
  char     buff[4];
  uint32_t length = 0;

  if (control_target == OSC_1_SHAPE) {
    if (wave != 5) {  // Saw, Sqr, Tri, Sin, and WT
      return PRA32_U2_ControlPanel_calc_value_display_osc_1_shape(wave, v, value_display_text);
    }
    uint32_t width = (v >= 127) ? 100 : (((100 * v) + 64) / 128);  // 0% (0) to 50% (64) to 100% (127)
    PRA32_U2_ControlPanel_append_decimal(buff, length, width);
    if (width < 100) {
      buff[length++] = '%';  // 3 characters at most: "100" for 100%
    }
  } else if (wave <= 2) {  // Saw, Sqr, and Tri: the Multi Saw, the Sync Square, and the Folded Triangle in %, as the Sustain
    return PRA32_U2_ControlPanel_calc_value_display_percent(EG_SUSTAIN, v, value_display_text);
  } else if (wave == 3) {
    uint32_t halves = ((v + 2) >> 2) + 1;  // the ratio (0.5 to 16.5) * 2: "0.5" to "9.5", and "10" to "16." ("." for .5)
    PRA32_U2_ControlPanel_append_decimal(buff, length, halves >> 1);
    if ((halves >> 1) < 10) {
      buff[length++] = '.';
      buff[length++] = (halves & 1) ? '5' : '0';
    } else if (halves & 1) {
      buff[length++] = '.';
    }
  } else if (wave == 4) {
    buff[length++] = 'W';
    buff[length++] = 'T';
    buff[length++] = '0' + (((v * 10) + 128) >> 8);
  } else if (wave == 5) {  // P64 (Pulse 100%) to 0 (Saw) to S64 (2nd Saw 100%), as it works
    PRA32_U2_ControlPanel_calc_value_display_two_sides(v, 'P', 'S', value_display_text);
    return true;
  } else {
    return false;
  }

  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
  return true;
}

// Osc 2 Pitch (osc2_pitch), and EG Mod Amt and LFO Mod Amt to the pitch (P and 2P), as they work (3 characters at most,
// right-aligned; the same as pitchDispValue() of PRA32-U2 Editor): the cents, rounded (e.g. "+2" for 1.5625 cents), and
// the semitones, with "." for .5 (e.g. "+1." for 1.5), with the sign but where it does not fit (e.g. "10." for 10.5);
// Osc 2 Pitch: 1.5625 cents a value up to 50 cents (+-32), and 0.5 semitones up to 12 (+-55); the Amt: 3.125 cents a
// value up to 96.875 cents (+-31), 1 semitone (+-32) to 24 semitones (+-55), and 30 to 120 semitones (+-56 to +-63)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_pitch)(uint8_t controller_value, boolean osc2_pitch, char value_display_text[5]) {
  static const uint8_t ary_semitones[8] PRA32_U2_TABLE_IN_RAM(panel_text) = { 30, 36, 42, 48, 60, 72, 96, 120 };
  int32_t  n = static_cast<int32_t>(controller_value) - 64;
  uint32_t a = (n >= 0) ? n : -n;
  char     buff[4];
  uint32_t length = 0;

  if (osc2_pitch) {
    a = minimum(a, 55u);
    if (a == 0) {
      buff[length++] = '0';
    } else if (a <= 32) {
      buff[length++] = (n > 0) ? '+' : '-';
      PRA32_U2_ControlPanel_append_decimal(buff, length, ((a * 25) + 8) / 16);
    } else {
      uint32_t halves = a - 31;  // 2 (1 semitone) to 24 (12 semitones)
      if (((halves & 1) == 0) || ((halves / 2) < 10)) {
        buff[length++] = (n > 0) ? '+' : '-';
      }
      PRA32_U2_ControlPanel_append_decimal(buff, length, halves / 2);
      if (halves & 1) {
        buff[length++] = '.';
      }
    }
  } else {
    a = minimum(a, 63u);
    if (a == 0) {
      buff[length++] = '0';
    } else if (a <= 31) {
      buff[length++] = (n > 0) ? '+' : '-';
      PRA32_U2_ControlPanel_append_decimal(buff, length, ((a * 25) + 4) / 8);
    } else {
      uint32_t semitones = (a <= 55) ? (a - 31) : ary_semitones[a - 56];
      if (semitones < 100) {
        buff[length++] = (n > 0) ? '+' : '-';
      }
      PRA32_U2_ControlPanel_append_decimal(buff, length, semitones);
    }
  }

  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// Pan: L63 (0 and 1, the same) to C (64) to R63 (127), right-aligned (the same as PRA32-U2 Editor)
static __attribute__((noinline)) void __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_pan)(uint8_t controller_value, char value_display_text[5]) {
  char     buff[4];
  uint32_t length = 0;
  if (controller_value == 64) {
    buff[length++] = 'C';
  } else {
    buff[length++] = (controller_value > 64) ? 'R' : 'L';
    PRA32_U2_ControlPanel_append_decimal(buff, length, (controller_value > 64) ? (controller_value - 64) :
                                                       ((controller_value <= 1) ? 63 : (64 - controller_value)));
  }
  PRA32_U2_ControlPanel_copy_right_aligned(value_display_text, buff, length);
}

// (in the RAM, but noinline: one copy for the values of the page, not one for each)
static __attribute__((noinline)) boolean __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display)(uint8_t control_target, uint8_t controller_value, char value_display_text[5])
{
  // The % (EG Sustain, Amp Sustain, Mixer Osc Mix, LFO Depth, Chorus Level, Delay Level, Delay Feedback, Filter Key
  // Track, and the Velo Sens and Key Track of the EG), and Amp Gain (dB), before the switch (in a function that is not inlined), which is about 0.5 KB larger (in the RAM) with them
  if (PRA32_U2_ControlPanel_calc_value_display_percent(control_target, controller_value, value_display_text)) {
    return true;
  }

  boolean result = false;

  switch (control_target) {
  case OSC_2_PITCH     :
    PRA32_U2_ControlPanel_calc_value_display_pitch(controller_value, true, value_display_text);
    result = true;
    break;
  case EG_OSC_AMT      :
  case LFO_OSC_AMT     :
    {
      // By EG Mod Dst or LFO Mod Dst: the pitch (P and 2P), the Cutoff as Filter EG Amt (F), and the Shape (1S)
      uint8_t  mod_dst = getCurrentControllerValue(getTargetMIDICh(s_current_synth), (control_target == EG_OSC_AMT) ? EG_OSC_DST : LFO_OSC_DST);
      uint32_t index   = ((mod_dst * 10) + 128) >> 8;
      if (index <= 3) {
        PRA32_U2_ControlPanel_calc_value_display_pitch(controller_value, false, value_display_text);
      } else if (index == 4) {
        PRA32_U2_ControlPanel_calc_value_display_filter_tune(FILTER_EG_AMT, controller_value, value_display_text);
      } else {
        PRA32_U2_ControlPanel_calc_value_display_shape_amt(controller_value, value_display_text);
      }
      result = true;
    }
    break;
  case COARSE_TUNE     :
  case PANEL_TRANSPOSE :
  case SEQ_TRANSPOSE   :
  case PITCH_BEND_MSB  :
    {
      PRA32_U2_ControlPanel_format_decimal(value_display_text, static_cast<int>(controller_value) - 64, true);
      result = true;
    }
    break;
  case OSC_2_COARSE    :  // -60 to +60, as it works
  case PANEL_PIT_OFST  :
  case SEQ_PIT_OFST    :
    {
      int pit_ofst = controller_value - 64;
      if (pit_ofst < -60) {
        pit_ofst = -60;
      } else if (pit_ofst > +60) {
        pit_ofst = +60;
      }

      PRA32_U2_ControlPanel_format_decimal(value_display_text, pit_ofst, true);
      result = true;
    }
    break;
  case OSC_1_SHAPE     :
  case OSC_1_MORPH     :
    result = PRA32_U2_ControlPanel_calc_value_display_osc_1(control_target, controller_value, value_display_text);
    break;
  case FILTER_CUTOFF   :
    PRA32_U2_ControlPanel_calc_value_display_cutoff(controller_value, value_display_text);
    result = true;
    break;
  case FINE_TUNE       :
    PRA32_U2_ControlPanel_calc_value_display_fine_tune(controller_value, value_display_text);
    result = true;
    break;
  case FILTER_RESO     :
  case FILTER_DRIVE    :
  case DISTORTION      :
  case STRETCH_TUNE    :
  case FILTER_EG_AMT   :
  case LFO_FILTER_AMT  :
  case BTH_FILTER_AMT  :
    result = PRA32_U2_ControlPanel_calc_value_display_filter_tune(control_target, controller_value, value_display_text);
    break;
  case PAN             :
    PRA32_U2_ControlPanel_calc_value_display_pan(controller_value, value_display_text);
    result = true;
    break;
  case EG_ATTACK       :
  case AMP_ATTACK      :
  case EG_DECAY        :
  case EG_RELEASE      :
  case AMP_DECAY       :
  case AMP_RELEASE     :
    PRA32_U2_ControlPanel_calc_value_display_time(controller_value, (control_target != EG_ATTACK) && (control_target != AMP_ATTACK), value_display_text);
    result = true;
    break;
  case PORTAMENTO      :
  case LFO_FADE_TIME   :
  case LFO_RATE        :
  case CHORUS_RATE     :
  case CHORUS_DEPTH    :
    result = PRA32_U2_ControlPanel_calc_value_display_time_rate(control_target, controller_value,
               getCurrentControllerValue(getTargetMIDICh(s_current_synth), LFO_WAVE), value_display_text);
    break;
  case MIXER_SUB_OSC   :  // N64 (Noise 100%) to 0 to S64 (Sub Osc 100%), as it works
    PRA32_U2_ControlPanel_calc_value_display_two_sides(controller_value, 'N', 'S', value_display_text);
    result = true;
    break;
  case OSC_1_WAVE      :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Saw","Sqr","Tri","Sin"," WT","Pls"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case OSC_2_WAVE      :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Saw","Sqr","Tri","Sin"," O1","Nos"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case EG_OSC_DST      :
  case LFO_OSC_DST     :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"  P","  P"," 2P"," 2P","  F"," 1S"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case VOICE_MODE      :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Pol","Pol","Mon","Mon"," LP","Lgt"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case LFO_WAVE        :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Tri","Sin","Nos","Saw","S&H","Sqr"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case FILTER_MODE     :
    {
      // LP, BR, HP, BP, and LP at 0, 32, 64, 96, and 127; in between, the two ends (e.g. L-R for LP to BR)
      static const char ary[9][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {" LP","L-R"," BR","R-H"," HP","H-B"," BP","B-L"," LP"};
      uint32_t x = (controller_value >= 127) ? 128 : controller_value;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[((x & 31) == 0) ? (x >> 4) : ((x >> 5) * 2 + 1)]);
      result = true;
    }
    break;
  case EG_AMP_MOD      :
  case REL_EQ_DECAY    :
  case SUSTAIN_PEDAL   :
  case PORTAMENTO_SW   :
  case SEQ_RX_ST_SP    :
    {
      static const char ary[2][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Off"," On"};
      uint32_t index = ((controller_value * 2) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case SEQ_CH_OFST     :
    {
      uint32_t ofst = controller_value >> 3;  // +0 to +15, in decimal (as the channels)
      value_display_text[0] = (ofst >= 10) ? '+' : ' ';
      value_display_text[1] = (ofst >= 10) ? '1' : '+';
      value_display_text[2] = '0' + ((ofst >= 10) ? (ofst - 10) : ofst);
      result = true;
    }
    break;
  case PANEL_MIDI_TX   :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Off","Clk","All","Nte","C+N","A+N"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case OSC_SAW_W_MODE  :
    {
      static const char ary[2][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Str","Cur"};
      uint32_t index = ((controller_value * 2) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case PORTA_MODE      :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"ESw","ESw","LSw","LSw","LAu","EAu"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case BTH_AMP_MOD     :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Off","Q","L","LO","QO","Opn"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case LFO_TRIG_MODE   :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Aut","Off"," On","VOn","VOf","VAu"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case FX_ROUTING      :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Cho","Cho","Dly","Dly","Cmp","Dry"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case VOICE_ASGN_MODE :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"  1","  1","  3","  3","  4","  2"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case DELAY_MODE      :
    {
      static const char ary[3][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Ste"," PP","Rev"};
      uint32_t index = ((controller_value * 4) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case DELAY_TIME      :
    {
      int display_value;

      if (controller_value < 5) {
        display_value = controller_value + 1;
      } else if (controller_value < 27) {
        display_value = (controller_value * 2) -4;
      } else if (controller_value < 114) {
        display_value = (((controller_value * 20) + 3) / 6) - 40;
      } else {
        display_value = 340;
      }

      PRA32_U2_ControlPanel_format_decimal(value_display_text, display_value);
      result = true;
    }
    break;
  case  RD_PROGRAM_0   :
  case  RD_PROGRAM_1   :
  case  RD_PROGRAM_2   :
  case  RD_PROGRAM_3   :
  case  RD_PROGRAM_4   :
  case  RD_PROGRAM_5   :
  case  RD_PROGRAM_6   :
  case  RD_PROGRAM_7   :
  case  RD_PROGRAM_8   :
  case  RD_PROGRAM_9   :
  case  RD_PROGRAM_10  :
  case  RD_PROGRAM_11  :
  case  RD_PROGRAM_12  :
  case  RD_PROGRAM_13  :
  case  RD_PROGRAM_14  :
  case  RD_PROGRAM_15  :
  case  RD_PROGRAM_16  :
  case  RD_PROGRAM_17  :
  case  RD_PROGRAM_18  :
  case  RD_PROGRAM_19  :
  case  RD_PROGRAM_20  :
  case  RD_PROGRAM_21  :
  case  RD_PROGRAM_22  :
  case  RD_PROGRAM_23  :
  case  RD_PROGRAM_24  :
  case  RD_PROGRAM_25  :
  case  RD_PROGRAM_26  :
  case  RD_PROGRAM_27  :
  case  RD_PROGRAM_28  :
  case  RD_PROGRAM_29  :
  case  RD_PROGRAM_30  :
  case  RD_PROGRAM_31  :
  case  WR_PROGRAM_0   :
  case  WR_PROGRAM_1   :
  case  WR_PROGRAM_2   :
  case  WR_PROGRAM_3   :
  case  WR_PROGRAM_4   :
  case  WR_PROGRAM_5   :
  case  WR_PROGRAM_6   :
  case  WR_PROGRAM_7   :
  case  WR_PROGRAM_8   :
  case  WR_PROGRAM_9   :
  case  WR_PROGRAM_10  :
  case  WR_PROGRAM_11  :
  case  WR_PROGRAM_12  :
  case  WR_PROGRAM_13  :
  case  WR_PROGRAM_14  :
  case  WR_PROGRAM_15  :
  case  RD_PANEL_PRMS  :
  case  IN_PANEL_PRMS  :
  case  WR_PANEL_PRMS  :
  case  UNDO_READ      :
  case  UNDO_WRITE     :
  case  SEQ_RAND_PITCH :
  case  SEQ_RAND_VELO  :
  case  PANIC_OP       :
  case  RAND_SYNTH_OP  :
    {
      if        (controller_value <= 31) {
        value_display_text[0] = 'R';
        value_display_text[1] = 'd';
        value_display_text[2] = 'y';
      } else if (controller_value >= 96) {
        value_display_text[0] = 'E';
        value_display_text[1] = 'x';
        value_display_text[2] = 'e';
      } else {
        value_display_text[0] = ' ';
        value_display_text[1] = ' ';
        value_display_text[2] = '-';
      }

      result = true;
    }
    break;
  case  PANEL_PLAY_PIT :
  case  SEQ_PITCH_0    :
  case  SEQ_PITCH_1    :
  case  SEQ_PITCH_2    :
  case  SEQ_PITCH_3    :
    {
      PRA32_U2_ControlPanel_calc_value_display_pitch(controller_value, value_display_text);
      result = true;
    }
    break;
  case  SEQ_PITCH_4    :
  case  SEQ_PITCH_5    :
  case  SEQ_PITCH_6    :
  case  SEQ_PITCH_7    :
    if (s_seq_pattern_4x4) {  // the shifts, added to the offsets (-60 to +60)
      int shift_ofst = controller_value - 64;
      if (shift_ofst < -60) { shift_ofst = -60; } else if (shift_ofst > +60) { shift_ofst = +60; }
      PRA32_U2_ControlPanel_format_decimal(value_display_text, shift_ofst, true);
    } else {
      PRA32_U2_ControlPanel_calc_value_display_pitch(controller_value, value_display_text);
    }
    result = true;
    break;
  case  SEQ_VELO_4     :
  case  SEQ_VELO_5     :
  case  SEQ_VELO_6     :
  case  SEQ_VELO_7     :
    if (s_seq_pattern_4x4) {  // the shifts, added to the velocity (-63 to +63; 0 and 1: -63)
      PRA32_U2_ControlPanel_format_decimal(value_display_text, maximum(static_cast<int32_t>(controller_value) - 64, -63), true);
      result = true;
    }
    break;
  case  SEQ_PATTERN    :
    {
      static const char ary[2][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Std","4x4"};
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[(controller_value >= 64) ? 1 : 0]);
      result = true;
    }
    break;
  case  PANEL_SCALE   :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Maj","Min","Mel","MaP","MiP","Chr"};
      uint32_t index = PRA32_U2_ControlPanel_get_index_scale();
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case PANEL_PLAY_MODE:
    {
      static const char ary[3][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Key","K+S","Seq"};
      uint32_t index = ((controller_value * 4) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case SEQ_TEMPO      :
    {
      uint32_t bpm = PRA32_U2_ControlPanel_calc_bpm(g_synth.current_controller_value(SEQ_TEMPO      ));
      PRA32_U2_ControlPanel_format_decimal(value_display_text, bpm);
      result = true;
    }
    break;
  case SEQ_CLOCK_SRC  :
    {
      uint32_t index = ((controller_value * 4) + 128) >> 8;

      if (index == 0) {
        value_display_text[0] = 'A';
        value_display_text[1] = 'u';
        value_display_text[2] = 't';
      } else if (index == 1) {
        value_display_text[0] = 'I';
        value_display_text[1] = 'n';
        value_display_text[2] = 't';
      } else {
        value_display_text[0] = 'E';
        value_display_text[1] = 'x';
        value_display_text[2] = 't';
      }

      result = true;
    }
    break;
  case SEQ_GATE_TIME  :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"1/6","2/6","3/6","4/6","5/6","6/6"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case SEQ_STEP_NOTE   :
    {
      static const char ary[3][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"  4","  8"," 16"};
      uint32_t index = ((controller_value * 4) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case SEQ_NUM_STEPS  :
    {
      int32_t last_step = g_synth.current_controller_value(SEQ_NUM_STEPS  );
      last_step = (last_step - 2) >> 2;
      if (last_step < 0) { last_step = 0; }
      PRA32_U2_ControlPanel_format_decimal(value_display_text, last_step + 1);
      result = true;
    }
    break;
  case SEQ_DIRECTION  :
    {
      static const char ary[3][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Fwd","Rvs","Bnc"};
      uint32_t index = ((controller_value * 4) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  case SEQ_ON_STEPS   :
    {
      uint8_t on_steps = g_synth.current_controller_value(SEQ_ON_STEPS   );
      PRA32_U2_ControlPanel_format_hex(value_display_text, on_steps);
      result = true;
    }
    break;
  case SEQ_ACT_STEPS  :
    {
      uint8_t act_steps = g_synth.current_controller_value(SEQ_ACT_STEPS  );
      PRA32_U2_ControlPanel_format_hex(value_display_text, act_steps);
      result = true;
    }
    break;
  case PANEL_MIDI_CH  :
    {
      uint8_t midi_ch = g_synth.current_controller_value(PANEL_MIDI_CH  ) >> 3;
      PRA32_U2_ControlPanel_format_decimal(value_display_text, midi_ch + 1);
      result = true;
    }
    break;
  case VOICE_GRP_MODE :
    {
      static const char ary[6][5] PRA32_U2_TABLE_IN_RAM(panel_text) = {"Off","Ly2","RLy","RR2","RR4","RR3"};
      uint32_t index = ((controller_value * 10) + 128) >> 8;
      PRA32_U2_ControlPanel_copy_text(value_display_text, ary[index]);
      result = true;
    }
    break;
  }

  return result;
}



INLINE void PRA32_U2_ControlPanel_setup() {
  // The programs of the synths at the start (#0-3 for the synths 0-3, also by their Program Changes at the start); those
  // of the channels of the groups (+ 4, + 5, and + 15) are of their first synths (e.g. #0, #2, and #0 with Ly2)
  for (uint32_t i = 0; (i < PRA32_U2_NUMBER_OF_SYNTHS) && (i < 4); ++i) {
    s_current_program[i] = (i + PROGRAM_NUMBER_DEFAULT) & USER_PROGRAM_NUMBER_MAX;
  }

#if defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)
  pinMode(PRA32_U2_KEY_INPUT_PREV_KEY_PIN, PRA32_U2_KEY_INPUT_PIN_MODE);
#endif  // defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)
  pinMode(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN, PRA32_U2_KEY_INPUT_PIN_MODE);
#endif  // defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)
  pinMode(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN, PRA32_U2_KEY_INPUT_PIN_MODE);
#endif  // defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN)
  pinMode(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN, PRA32_U2_KEY_INPUT_PIN_MODE);
#endif  // defined(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN)
  pinMode(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN, PRA32_U2_KEY_INPUT_PIN_MODE);
#endif  // defined(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
  pinMode(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN, PRA32_U2_KEY_INPUT_PIN_MODE);
#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)

#if defined(PRA32_U2_USE_CONTROL_PANEL)
  PRA32_U2_ControlPanel_update_page();

#if defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
  adc_init();
  adc_gpio_init(26);
  adc_gpio_init(27);

#if defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)
  adc_gpio_init(28);
#endif  // defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)

#if defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
  i2c_init(PRA32_U2_OLED_DISPLAY_I2C, 400 * 1000);
  i2c_set_slave_mode(PRA32_U2_OLED_DISPLAY_I2C, false, 0);
  gpio_set_function(PRA32_U2_OLED_DISPLAY_I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PRA32_U2_OLED_DISPLAY_I2C_SDA_PIN);
  gpio_set_function(PRA32_U2_OLED_DISPLAY_I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(PRA32_U2_OLED_DISPLAY_I2C_SCL_PIN);

  uint8_t commands_init_0[] = {0x00,  0x81, PRA32_U2_OLED_DISPLAY_CONTRAST,  0xA0, 0xC0,  0x8D, 0x14};
  if (PRA32_U2_OLED_DISPLAY_ROTATION) {
    commands_init_0[3] = 0xA1;
    commands_init_0[4] = 0xC8;
  }

  // This also sets the target address for PRA32_U2_ControlPanel_write_to_oled_display()
  i2c_write_blocking(PRA32_U2_OLED_DISPLAY_I2C, PRA32_U2_OLED_DISPLAY_I2C_ADDRESS, commands_init_0, sizeof(commands_init_0), false);

  // NOTE: Do not call i2c_write_blocking() after this, which disables the I2C to set
  //       the target address and would cut off the transfers still in the TX FIFO
  for (uint8_t y = 0; y <= 7; ++y) {
    for (uint8_t x = 0; x <= 20; ++x) {
      PRA32_U2_ControlPanel_set_draw_position(x, y);
      PRA32_U2_ControlPanel_draw_character(s_display_buffer[y][x]);
    }

    uint8_t commands[] = {0x00,  static_cast<uint8_t>(0xB0 + y), 0x17, 0x0E};
    PRA32_U2_ControlPanel_write_to_oled_display(commands, sizeof(commands));

    uint8_t data[] = {0x40,  0x00, 0x00};
    PRA32_U2_ControlPanel_write_to_oled_display(data, sizeof(data));
  }

  uint8_t commands_init_1[] = {0x00,  0xAF};
  PRA32_U2_ControlPanel_write_to_oled_display(commands_init_1, sizeof(commands_init_1));
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

INLINE void PRA32_U2_ControlPanel_initialize_parameters() {
}

INLINE void PRA32_U2_ControlPanel_update_analog_inputs(uint32_t loop_counter) {
  static_cast<void>(loop_counter);

#if defined(PRA32_U2_USE_CONTROL_PANEL)

#if defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
  // Each input is converted 16 times, once per sample, without waiting for the
  // conversion (2 us): the conversion started at a sample is read at the next
  // sample (20.8 us later), when it has been done. ADC0 at 0x10-0x1F (read at
  // 0x11-0x20), ADC1 at 0x20-0x2F, and ADC2 at 0x30-0x3F (read at 0x31-0x00)
  static int32_t s_adc_total_value = 0;
  const uint32_t phase = loop_counter & 0x3F;
  const uint32_t step  = phase & 0x0F;

  if ((phase >= 0x11) || (phase == 0x00)) {
    s_adc_total_value += adc_hw->result;
  }

  if ((step == 0x00) && (phase != 0x10)) {
    // The 16th conversion of the input has been read: ADC0 at 0x20, ADC1 at 0x30, and ADC2 at 0x00
    const uint32_t adc_number = ((phase - 0x20) >> 4) & 0x03;
    if        (s_adc_current_value[adc_number]                                   >= s_adc_total_value + PRA32_U2_ANALOG_INPUT_THRESHOLD) {
      s_adc_current_value[adc_number] = s_adc_total_value;
    } else if (s_adc_current_value[adc_number] + PRA32_U2_ANALOG_INPUT_THRESHOLD <= s_adc_total_value                                 ) {
      s_adc_current_value[adc_number] = s_adc_total_value;
    }
  }

  if (phase >= 0x10) {
    if (step == 0x00) {
      adc_select_input((phase >> 4) - 1);
      s_adc_total_value = PRA32_U2_ANALOG_INPUT_CORRECTION;
    }
    hw_set_bits(&adc_hw->cs, ADC_CS_START_ONCE_BITS);
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

#if defined(PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT)
INLINE uint32_t PRA32_U2_ControlPanel_debounce_key(uint32_t pin, uint32_t current_value, uint32_t& stable_counter) {
  uint32_t value = digitalRead(pin) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;

  // Accept a new level only after it has been read continuously for PRA32_U2_KEY_PRESS_STABLE_WAIT times (pressed)
  // or PRA32_U2_KEY_RELEASE_STABLE_WAIT times (released), so that a short open while holding a key is not a release
  if (value == current_value) {
    stable_counter = 0;
    return current_value;
  }

  ++stable_counter;
  uint32_t stable_wait = value ? PRA32_U2_KEY_PRESS_STABLE_WAIT : PRA32_U2_KEY_RELEASE_STABLE_WAIT;
  if (stable_counter < stable_wait) {
    return current_value;
  }

  stable_counter = 0;
  return value;
}
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT)

INLINE void PRA32_U2_ControlPanel_update_control() {
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  static uint32_t s_initialize_counter = 0;
  if (s_initialize_counter < 75) {
    ++s_initialize_counter;
#if defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
    s_adc_control_value[0] = PRA32_U2_ControlPanel_adc_control_value_candidate(0);
    s_adc_control_value[1] = PRA32_U2_ControlPanel_adc_control_value_candidate(1);
    s_adc_control_value[2] = PRA32_U2_ControlPanel_adc_control_value_candidate(2);
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
    return;
  }

  if (s_program_display_requested) {
    s_program_display_requested = false;
    PRA32_U2_ControlPanel_update_program_display();
  }

#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  // Back to the Main Synth when the channel selected goes to no synth now (e.g. after Voice Group Mode went Off by a
  // Program Change), checked here, after the MIDI messages, where the synths have taken the changes
  if (!PRA32_U2_ControlPanel_target_usable(s_current_synth)) {
    g_synth.control_change(PANEL_TARGET_CH, 0);
  }
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)

  PRA32_U2_ControlPanel_update_control_seq();

  boolean processed = PRA32_U2_ControlPanel_process_note_off_on();
  if (processed) {
    return;
  }

#if defined(PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT)
  static uint32_t s_prev_key_value_changed_time = 0;
  static uint32_t s_next_key_value_changed_time = 0;
  static uint32_t s_play_key_value_changed_time = 0;

  static uint32_t s_prog_minus_key_value_changed_time = 0;
  static uint32_t s_prog_plus_key_value_changed_time  = 0;

  static uint32_t s_prev_key_long_pressed = false;
  static uint32_t s_next_key_long_pressed = false;

  static uint32_t s_prog_minus_key_long_pressed = false;
  static uint32_t s_prog_plus_key_long_pressed  = false;

  static uint32_t s_prev_key_stable_counter = 0;
  static uint32_t s_next_key_stable_counter = 0;
  static uint32_t s_play_key_stable_counter = 0;

  static uint32_t s_prog_minus_key_stable_counter = 0;
  static uint32_t s_prog_plus_key_stable_counter  = 0;

  static uint32_t s_key_inpuy_counter = 0;
  ++s_key_inpuy_counter;

#if defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)
  uint32_t prev_key_value = PRA32_U2_ControlPanel_debounce_key(PRA32_U2_KEY_INPUT_PREV_KEY_PIN, s_prev_key_current_value, s_prev_key_stable_counter);

  if (s_prev_key_current_value != prev_key_value) {
    s_prev_key_current_value = prev_key_value;
    s_prev_key_value_changed_time = s_key_inpuy_counter;

    if (s_prev_key_current_value == 0) {
      // Prev key released
      if (s_prev_key_long_pressed == false) {
#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
        uint32_t shift_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
        if (shift_key_pressed) {
          if (s_current_page_group == 0) {
            s_current_page_group = NUMBER_OF_PAGE_GROUPS - 1;
          } else {
            --s_current_page_group;
          }

          PRA32_U2_ControlPanel_update_page();
          return;
        }

#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
        if (s_current_page_index[s_current_page_group] == 0) {
          s_current_page_index[s_current_page_group] = g_number_of_pages[s_current_page_group] - 1;
        } else {
          --s_current_page_index[s_current_page_group];
        }

        PRA32_U2_ControlPanel_update_page();
        return;
      }

      s_prev_key_long_pressed = false;
      return;
    }

#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
    if (s_prev_key_current_value == 1) {
      // Prev key pressed
      if (s_prev_key_long_pressed == false) {
#if defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)
        uint32_t next_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
        if (next_key_pressed) {
          PRA32_U2_ControlPanel_notes_off();

          PRA32_U2_ControlPanel_step_target();
          g_synth.control_change(PANEL_TARGET_CH, ((getTargetMIDICh(s_current_synth) - 1) - g_midi_ch) & 0x0F);  // kept by Write Panel Prms

          s_prev_key_long_pressed = true;
          s_next_key_long_pressed = true;

          PRA32_U2_ControlPanel_update_page();
          return;
        }

#endif  // defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)
      }
    }
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  }

  if (s_prev_key_current_value == 1) {
    if (s_prev_key_long_pressed == false) {
      if (s_key_inpuy_counter - s_prev_key_value_changed_time >= PRA32_U2_KEY_LONG_PRESS_WAIT) {
        s_prev_key_long_pressed = true;

        if (s_current_page_group == 0) {
          s_current_page_group = NUMBER_OF_PAGE_GROUPS - 1;
        } else {
          --s_current_page_group;
        }

        PRA32_U2_ControlPanel_update_page();
        return;
      }
    }
  }
#endif  // defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)
  uint32_t next_key_value = PRA32_U2_ControlPanel_debounce_key(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN, s_next_key_current_value, s_next_key_stable_counter);

  if (s_next_key_current_value != next_key_value) {
    s_next_key_current_value = next_key_value;
    s_next_key_value_changed_time = s_key_inpuy_counter;

    if (s_next_key_current_value == 0) {
      // Next key released
      if (s_next_key_long_pressed == false) {
#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
        uint32_t shift_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
        if (shift_key_pressed) {
          if (s_current_page_group == NUMBER_OF_PAGE_GROUPS - 1) {
            s_current_page_group = 0;
          } else {
            ++s_current_page_group;
          }

          PRA32_U2_ControlPanel_update_page();
          return;
        }

#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
        if (s_current_page_index[s_current_page_group] == g_number_of_pages[s_current_page_group] - 1) {
          s_current_page_index[s_current_page_group] = 0;
        } else {
          ++s_current_page_index[s_current_page_group];
        }

        PRA32_U2_ControlPanel_update_page();
        return;
      }

      s_next_key_long_pressed = false;
      return;
    }

#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
    if (s_next_key_current_value == 1) {
      // Next key pressed
      if (s_next_key_long_pressed == false) {
#if defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)
        uint32_t prev_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_PREV_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
        if (prev_key_pressed) {
          PRA32_U2_ControlPanel_notes_off();

          PRA32_U2_ControlPanel_step_target();
          g_synth.control_change(PANEL_TARGET_CH, ((getTargetMIDICh(s_current_synth) - 1) - g_midi_ch) & 0x0F);  // kept by Write Panel Prms

          s_prev_key_long_pressed = true;
          s_next_key_long_pressed = true;

          PRA32_U2_ControlPanel_update_page();
          return;
        }

#endif  // defined(PRA32_U2_KEY_INPUT_PREV_KEY_PIN)
      }
    }
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
  }

  if (s_next_key_current_value == 1) {
    if (s_next_key_long_pressed == false) {
      if (s_key_inpuy_counter - s_next_key_value_changed_time >= PRA32_U2_KEY_LONG_PRESS_WAIT) {
        s_next_key_long_pressed = true;

        if (s_current_page_group == NUMBER_OF_PAGE_GROUPS - 1) {
          s_current_page_group = 0;
        } else {
          ++s_current_page_group;
        }

        PRA32_U2_ControlPanel_update_page();
        return;
      }
    }
  }
#endif  // defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)
  uint32_t play_key_value = PRA32_U2_ControlPanel_debounce_key(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN, s_play_key_current_value, s_play_key_stable_counter);

  if (s_play_key_current_value != play_key_value) {
    s_play_key_current_value = play_key_value;
    s_play_key_value_changed_time = s_key_inpuy_counter;

    if (s_play_key_current_value == 1) {
      // Play key pressed
#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
      if (digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL) {
        s_play_key_with_shift = true;  // Shift + Play: Panic, when released
        return;
      }
#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
#if defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)
      if (digitalRead(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL) {
        s_play_key_with_next = true;     // Next + Play: the Step Sequencer started or stopped (Seq and K+S)
        s_next_key_long_pressed = true;  // not the next page, when the Next key is released
        if (PRA32_U2_ControlPanel_mode_has_seq()) {
          PRA32_U2_ControlPanel_seq_start_stop();
        }
        return;
      }
#endif  // defined(PRA32_U2_KEY_INPUT_NEXT_KEY_PIN)
      if (PRA32_U2_ControlPanel_mode_has_key()) {  // Key and K+S
        PRA32_U2_ControlPanel_key_play(true);
      }
    } else {
      // Play key released
      if (s_play_key_with_shift) {
        s_play_key_with_shift = false;
        PRA32_U2_ControlPanel_execute_operation(PANIC_OP);
        return;
      }
      if (s_play_key_with_next) {
        s_play_key_with_next = false;
        return;
      }
      if (PRA32_U2_ControlPanel_mode_has_key()) {  // Key and K+S
        PRA32_U2_ControlPanel_key_play(false);
      } else {  // Seq Mode
        PRA32_U2_ControlPanel_seq_start_stop();
      }
    }

    return;
  }
#endif  // defined(PRA32_U2_KEY_INPUT_PLAY_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN)
  uint32_t prog_minus_key_value = PRA32_U2_ControlPanel_debounce_key(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN, s_prog_minus_key_current_value, s_prog_minus_key_stable_counter);
  if (s_prog_minus_key_current_value != prog_minus_key_value) {
    s_prog_minus_key_current_value = prog_minus_key_value;
    s_prog_minus_key_value_changed_time = s_key_inpuy_counter;
    if (s_prog_minus_key_current_value == 0) {
      // Prog - key released
      if (s_prog_minus_key_long_pressed == false) {
#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
        uint32_t shift_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
        if (shift_key_pressed) {
          PRA32_U2_ControlPanel_step_program(false);
        }
#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
      }
      s_prog_minus_key_long_pressed = false;
      return;
    }
  }
  if (s_prog_minus_key_current_value == 1) {
    if (s_prog_minus_key_long_pressed == false) {
      if (s_key_inpuy_counter - s_prog_minus_key_value_changed_time >= PRA32_U2_KEY_LONG_PRESS_WAIT) {
        s_prog_minus_key_long_pressed = true;
        PRA32_U2_ControlPanel_step_program(false);
        return;
      }
    }
  }
#endif  // defined(PRA32_U2_KEY_INPUT_PROG_MINUS_KEY_PIN)

#if defined(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN)
  uint32_t prog_plus_key_value = PRA32_U2_ControlPanel_debounce_key(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN, s_prog_plus_key_current_value, s_prog_plus_key_stable_counter);
  if (s_prog_plus_key_current_value != prog_plus_key_value) {
    s_prog_plus_key_current_value = prog_plus_key_value;
    s_prog_plus_key_value_changed_time = s_key_inpuy_counter;
    if (s_prog_plus_key_current_value == 0) {
      // Prog + key released
      if (s_prog_plus_key_long_pressed == false) {
#if defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
        uint32_t shift_key_pressed = digitalRead(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN) == PRA32_U2_KEY_INPUT_ACTIVE_LEVEL;
        if (shift_key_pressed) {
          PRA32_U2_ControlPanel_step_program(true);
        }
#endif  // defined(PRA32_U2_KEY_INPUT_SHIFT_KEY_PIN)
      }
      s_prog_plus_key_long_pressed = false;
      return;
    }
  }
  if (s_prog_plus_key_current_value == 1) {
    if (s_prog_plus_key_long_pressed == false) {
      if (s_key_inpuy_counter - s_prog_plus_key_value_changed_time >= PRA32_U2_KEY_LONG_PRESS_WAIT) {
        s_prog_plus_key_long_pressed = true;
        PRA32_U2_ControlPanel_step_program(true);
        return;
      }
    }
  }
#endif  // defined(PRA32_U2_KEY_INPUT_PROG_PLUS_KEY_PIN)

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_KEY_INPUT)

#if defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
  static uint32_t s_adc_number_to_check = 0;

  boolean updated = PRA32_U2_ControlPanel_update_control_adc(s_adc_number_to_check);
  s_adc_number_to_check = (s_adc_number_to_check + 1) % 3;

  if (updated == false) {
    updated = PRA32_U2_ControlPanel_update_control_adc(s_adc_number_to_check);
    s_adc_number_to_check = (s_adc_number_to_check + 1) % 3;

    if (updated == false) {
      updated = PRA32_U2_ControlPanel_update_control_adc(s_adc_number_to_check);
      s_adc_number_to_check = (s_adc_number_to_check + 1) % 3;
    }
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}


INLINE void PRA32_U2_ControlPanel_on_clock()
{
  if (s_seq_clock_src_mode == 0) {  // Aut: Ext at a MIDI Clock
    s_seq_clock_src_external = true;
  }
  if (s_seq_clock_src_external) {
    if (PRA32_U2_ControlPanel_mode_has_seq()) {  // Seq and K+S
      PRA32_U2_ControlPanel_seq_clock();
    }
  }
}

INLINE void PRA32_U2_ControlPanel_on_start()
{
  if (s_seq_clock_src_external) {
    if (PRA32_U2_ControlPanel_mode_has_seq()) {  // Seq and K+S
      if (s_playing_status == PlayingStatus_Stop) {
        PRA32_U2_ControlPanel_seq_start();
      }
    }
  }
}

INLINE void PRA32_U2_ControlPanel_on_stop()
{
  if (s_seq_clock_src_external) {
    if (PRA32_U2_ControlPanel_mode_has_seq()) {  // Seq and K+S
      if (s_playing_status != PlayingStatus_Stop) {
        PRA32_U2_ControlPanel_seq_stop();
      }
    }
  }
}

// The value shown for a control target of the page: General Purpose Controller 1-4 (Pages B-24 and B-25) show the parameter
// that they control (Assign #16-#19), its value (compared with the knob: "<" or ">" until the knob comes to it, from when
// it controls the parameter, as the knobs of the other pages, with Assign Mode Cat) and its number (e.g. "A>112[ 74]"), or their own values with "[Off]"
static __attribute__((noinline, unused)) uint8_t __not_in_flash_func(PRA32_U2_ControlPanel_current_value_of_target)(uint8_t control_target) {
  uint8_t channel = getTargetMIDICh(s_current_synth);
  uint8_t value   = getCurrentControllerValue(channel, control_target);
  if ((control_target >= GEN_PURPOSE_1) && (control_target <= GEN_PURPOSE_4)) {
    uint8_t dst = getCurrentControllerValue(channel, ASSIGN_16  + (control_target - GEN_PURPOSE_1));
    if (!PRA32_U2_assign_dst_off(dst)) {
      value = getCurrentControllerValue(channel, dst);
    }
  }
  return value;
}

// The value that the knob is compared with ("<", ">", or "="): with Assign Mode Jmp, Rl2, or Rl1, General Purpose Controller 1-4
// control the parameters at once (as absolute values, see PRA32_U2_ControlPanel_absolute_target()), so the knob is always "="
static __attribute__((noinline, unused)) uint8_t __not_in_flash_func(PRA32_U2_ControlPanel_compared_value_of_target)(uint8_t control_target, uint8_t adc_control_value,
                                                                                                                   uint8_t current_controller_value) {
  if ((control_target >= GEN_PURPOSE_1) && (control_target <= GEN_PURPOSE_4) &&
      (PRA32_U2_assign_mode(getCurrentControllerValue(getTargetMIDICh(s_current_synth), ASSIGN_MODE)) != 1)) {
    return adc_control_value;
  }
  return current_controller_value;
}

static __attribute__((noinline, unused)) boolean __not_in_flash_func(PRA32_U2_ControlPanel_calc_value_display_of_target)(uint8_t control_target, uint8_t controller_value,
                                                                                                                  char value_display_text[5]) {
  if ((control_target >= GEN_PURPOSE_1) && (control_target <= GEN_PURPOSE_4)) {
    uint8_t dst = getCurrentControllerValue(getTargetMIDICh(s_current_synth), ASSIGN_16  + (control_target - GEN_PURPOSE_1));
    return PRA32_U2_ControlPanel_calc_value_display(ASSIGN_16, dst, value_display_text);  // its number, or "Off"
  }
  return PRA32_U2_ControlPanel_calc_value_display(control_target, controller_value, value_display_text);
}

INLINE void PRA32_U2_ControlPanel_update_display_buffer(uint32_t loop_counter) {
  static_cast<void>(loop_counter);

// The texts of the display (the values of the page), only for the display or the debug print (not without both,
// e.g. without the panel, to save the RAM of the code)
#if defined(PRA32_U2_USE_CONTROL_PANEL) && (defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY) || defined(PRA32_U2_USE_DEBUG_PRINT))
  if ((loop_counter & 0x7F) == 0x00) {
    char buff[6];

    uint8_t adc_control_target_0 = s_adc_control_target[0];
    if (adc_control_target_0 < 0xFF) {
      uint8_t adc_control_value        = s_adc_control_value[0];
      uint8_t current_controller_value = adc_control_value;
      if        (adc_control_target_0 < 128) {
        current_controller_value = PRA32_U2_ControlPanel_current_value_of_target(adc_control_target_0);
      } else if (adc_control_target_0 < 128 + 64) {
        current_controller_value = g_synth.current_controller_value(adc_control_target_0);
      }

      s_display_buffer[7][ 0] = 'A';
      uint8_t compared_value = PRA32_U2_ControlPanel_compared_value_of_target(adc_control_target_0, adc_control_value, current_controller_value);
      if        (adc_control_value < compared_value) {
        s_display_buffer[7][ 1] = '<';
      } else if (adc_control_value > compared_value) {
        s_display_buffer[7][ 1] = '>';
      } else {
        s_display_buffer[7][ 1] = '=';
      }

      PRA32_U2_ControlPanel_format_decimal(buff, current_controller_value);
      s_display_buffer[7][ 2] = buff[0];
      s_display_buffer[7][ 3] = buff[1];
      s_display_buffer[7][ 4] = buff[2];

      char value_display_text[5] = {};
      boolean exists = PRA32_U2_ControlPanel_calc_value_display_of_target(adc_control_target_0, current_controller_value, value_display_text);
      if (exists) {
        s_display_buffer[7][ 5] = '[';
        s_display_buffer[7][ 6] = value_display_text[0];
        s_display_buffer[7][ 7] = value_display_text[1];
        s_display_buffer[7][ 8] = value_display_text[2];
        s_display_buffer[7][ 9] = ']';
      } else {
        std::memset(&s_display_buffer[7][ 5], ' ', 5);
      }
    } else {
      std::memset(&s_display_buffer[7][ 0], ' ', 10);
    }

    uint8_t adc_control_target_1 = s_adc_control_target[1];
    if (adc_control_target_1 < 0xFF) {
      uint8_t adc_control_value        = s_adc_control_value[1];
      uint8_t current_controller_value = adc_control_value;
      if        (adc_control_target_1 < 128) {
        current_controller_value = PRA32_U2_ControlPanel_current_value_of_target(adc_control_target_1);
      } else if (adc_control_target_1 < 128 + 64) {
        current_controller_value = g_synth.current_controller_value(adc_control_target_1);
      }

      s_display_buffer[7][11] = 'B';
      uint8_t compared_value = PRA32_U2_ControlPanel_compared_value_of_target(adc_control_target_1, adc_control_value, current_controller_value);
      if        (adc_control_value < compared_value) {
        s_display_buffer[7][12] = '<';
      } else if (adc_control_value > compared_value) {
        s_display_buffer[7][12] = '>';
      } else {
        s_display_buffer[7][12] = '=';
      }

      PRA32_U2_ControlPanel_format_decimal(buff, current_controller_value);
      s_display_buffer[7][13] = buff[0];
      s_display_buffer[7][14] = buff[1];
      s_display_buffer[7][15] = buff[2];

      char value_display_text[5] = {};
      boolean exists = PRA32_U2_ControlPanel_calc_value_display_of_target(adc_control_target_1, current_controller_value, value_display_text);
      if (exists) {
        s_display_buffer[7][16] = '[';
        s_display_buffer[7][17] = value_display_text[0];
        s_display_buffer[7][18] = value_display_text[1];
        s_display_buffer[7][19] = value_display_text[2];
        s_display_buffer[7][20] = ']';
      } else {
        std::memset(&s_display_buffer[7][16], ' ', 5);
      }
    } else {
      std::memset(&s_display_buffer[7][11], ' ', 10);
    }

    uint8_t adc_control_target_2 = s_adc_control_target[2];
    if (adc_control_target_2 < 0xFF) {
      uint8_t adc_control_value        = s_adc_control_value[2];
      uint8_t current_controller_value = adc_control_value;
      if        (adc_control_target_2 < 128) {
        current_controller_value = PRA32_U2_ControlPanel_current_value_of_target(adc_control_target_2);
      } else if (adc_control_target_2 < 128 + 64) {
        current_controller_value = g_synth.current_controller_value(adc_control_target_2);
      }

      s_display_buffer[3][11] = 'C';
      uint8_t compared_value = PRA32_U2_ControlPanel_compared_value_of_target(adc_control_target_2, adc_control_value, current_controller_value);
      if        (adc_control_value < compared_value) {
        s_display_buffer[3][12] = '<';
      } else if (adc_control_value > compared_value) {
        s_display_buffer[3][12] = '>';
      } else {
        s_display_buffer[3][12] = '=';
      }

      PRA32_U2_ControlPanel_format_decimal(buff, current_controller_value);
      s_display_buffer[3][13] = buff[0];
      s_display_buffer[3][14] = buff[1];
      s_display_buffer[3][15] = buff[2];

      char value_display_text[5] = {};
      boolean exists = PRA32_U2_ControlPanel_calc_value_display_of_target(adc_control_target_2, current_controller_value, value_display_text);
      if (exists) {
        s_display_buffer[3][16] = '[';
        s_display_buffer[3][17] = value_display_text[0];
        s_display_buffer[3][18] = value_display_text[1];
        s_display_buffer[3][19] = value_display_text[2];
        s_display_buffer[3][20] = ']';
      } else {
        std::memset(&s_display_buffer[3][16], ' ', 5);
      }
    } else {
      std::memset(&s_display_buffer[3][11], ' ', 10);
    }
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL) && (defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY) || defined(PRA32_U2_USE_DEBUG_PRINT))
}

INLINE void PRA32_U2_ControlPanel_update_display(uint32_t loop_counter) {
  static_cast<void>(loop_counter);

#if defined(PRA32_U2_USE_CONTROL_PANEL)

#if defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)
  static uint32_t s_display_draw_position_x = 0;
  static uint32_t s_display_draw_position_y = 0;
  static bool     s_display_draw_position_update = false;

  if ((loop_counter & 0x3F) == 0x00) {
    if (s_display_draw_position_update) {
      PRA32_U2_ControlPanel_draw_character(s_display_buffer[s_display_draw_position_y][s_display_draw_position_x]);
      s_display_draw_position_update = false;
      return;
    }

    ++s_display_draw_counter;

    // From the start (after a page changed, or the channel or the program shown), the lines on the left (0-3 and 5-7)
    // and on the right (5-7 and 1-3) once; and then the values (lines 7 and 3) and the top line on the right, again and again
    if (s_display_draw_counter == (17 * 10)) {
      s_display_draw_counter = (13 * 10);
    }

    // The lines in this order, each 10 characters, + 8 for those on the right (columns 11-20)
    static const uint8_t s_lines[17] PRA32_U2_TABLE_IN_RAM(display_lines) = {
      0, 1, 2, 3, 5, 6, 7,  5 + 8, 6 + 8, 7 + 8, 1 + 8, 2 + 8, 3 + 8,  7, 7 + 8, 3 + 8, 0 + 8,
    };
    uint8_t line = s_lines[s_display_draw_counter / 10];
    s_display_draw_position_x = (s_display_draw_counter % 10) + ((line & 8) ? 11 : 0);
    s_display_draw_position_y = line & 7;

    if ((s_display_draw_position_x == 0) || (s_display_draw_position_x == 11)) {
      s_display_draw_position_update = true;
    }

    if (s_display_draw_position_update) {
      PRA32_U2_ControlPanel_set_draw_position(s_display_draw_position_x, s_display_draw_position_y);
    } else {
      PRA32_U2_ControlPanel_draw_character(s_display_buffer[s_display_draw_position_y][s_display_draw_position_x]);
    }
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_OLED_DISPLAY)

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

// Panel Target Ch (PANEL_TARGET_CH, also by CC #58 on the Panel Channel, and kept by Write Panel Prms): the synth of the
// channel of the Basic Channel + the offset (0-15, e.g. 4 for the group by Voice Group Mode, or 15 for the Layering), one of
// those that the panel selects (the synths, the Round Robin, and the Layering; only the Main Synth while Voice Group Mode is
// Off); another keeps the selection, and the value goes back to it, so that the value is always that of the selection
// (and Write Panel Prms keeps it)
static void PRA32_U2_ControlPanel_select_target(uint8_t offset) {
  for (uint32_t synth = 0; synth < PRA32_U2_NUMBER_OF_SYNTHS; ++synth) {
    if (((((getTargetMIDICh(synth) - 1) - g_midi_ch) & 0x0F) == (offset & 0x0F)) && PRA32_U2_ControlPanel_target_usable(synth)) {
#if (PRA32_U2_NUMBER_OF_SYNTHS > 1)
      if (synth != s_current_synth) {
        PRA32_U2_ControlPanel_notes_off();
        s_current_synth = synth;
        PRA32_U2_ControlPanel_update_page();
      }
#endif  // (PRA32_U2_NUMBER_OF_SYNTHS > 1)
      return;
    }
  }

  g_synth.control_change(PANEL_TARGET_CH, ((getTargetMIDICh(s_current_synth) - 1) - g_midi_ch) & 0x0F);
}

/* INLINE */ void __not_in_flash_func(PRA32_U2_ControlPanel_on_control_change)(uint8_t control_number)
{
  static_cast<void>(control_number);

#if defined(PRA32_U2_USE_CONTROL_PANEL)
  if (s_adc_control_target[0] == control_number) {
    s_adc_control_catched[0] = false;
  }

  if (s_adc_control_target[1] == control_number) {
    s_adc_control_catched[1] = false;
  }

  if (s_adc_control_target[2] == control_number) {
    s_adc_control_catched[2] = false;
  }

  // The parameters of the synths (all but Voice Group Mode, below 128): nothing more (not compared with each below,
  // as a Program Change sets each of them, of each synth)
  if ((control_number < 128) && (control_number != VOICE_GRP_MODE)) {
    return;
  }

  if ((control_number == PANEL_PLAY_PIT ) ||
      (control_number == PANEL_PLAY_VELO) ||
      (control_number == PANEL_SCALE    ) ||
      (control_number == PANEL_PIT_OFST ) ||
      (control_number == PANEL_TRANSPOSE)) {
    PRA32_U2_ControlPanel_update_pitch(false);
  } else if (control_number == PANEL_PLAY_MODE) {
    uint8_t controller_value = g_synth.current_controller_value(PANEL_PLAY_MODE);
    static const uint8_t modes[3] = { 0, 2, 1 };  // Key, K+S, and Seq
    uint8_t mode = modes[((controller_value * 4) + 128) >> 8];

    if (s_play_mode != mode) {
      s_play_mode = mode;

      s_playing_status = PlayingStatus_Stop;
      s_key_held = false;
      PRA32_U2_ControlPanel_update_play_display();
      s_panel_play_note_gate[NOTE_KEY] = false;
      s_panel_play_note_gate[NOTE_SEQ] = false;

      PRA32_U2_ControlPanel_update_pitch(false);
      PRA32_U2_ControlPanel_update_page();
    }
  } else if (control_number == SEQ_TEMPO      ) {
    uint8_t tempo = g_synth.current_controller_value(SEQ_TEMPO      );
    uint32_t bpm = PRA32_U2_ControlPanel_calc_bpm(tempo);
    s_seq_count_increment = bpm * 8192;
    if ((s_seq_clock_src_mode == 0) && (s_seq_tempo_last <= 127) && (tempo != s_seq_tempo_last)) {  // Aut: Int, by the tempo
      s_seq_clock_src_external = false;
    }
    s_seq_tempo_last = tempo;
  } else if (control_number == SEQ_CLOCK_SRC  ) {
    uint32_t controller_value = g_synth.current_controller_value(SEQ_CLOCK_SRC  );
    uint32_t index = ((controller_value * 4) + 128) >> 8;
    if (index != s_seq_clock_src_mode) {
      s_seq_clock_src_mode = index;
      s_seq_clock_src_external = (index == 2);  // Ext (Aut: Int until a MIDI Clock)
    }
  } else if (control_number == SEQ_GATE_TIME  ) {
    uint8_t controller_value = g_synth.current_controller_value(SEQ_GATE_TIME  );
    s_seq_gate_time = (((controller_value * 10) + 128) >> 8) + 1;
  } else if (control_number == SEQ_STEP_NOTE  ) {
    uint8_t ary[3] = {24, 12, 6};
    uint8_t controller_value = g_synth.current_controller_value(SEQ_STEP_NOTE  );
    uint32_t index = ((controller_value * 4) + 128) >> 8;
    s_seq_step_clock_candidate = ary[index];
  } else if (control_number == SEQ_NUM_STEPS  ) {
    int32_t last_step = g_synth.current_controller_value(SEQ_NUM_STEPS  );
    last_step = (last_step - 2) >> 2;
    if (last_step < 0) { last_step = 0; }
    s_seq_last_step = last_step;
  } else if (control_number == SEQ_PATTERN    ) {
    s_seq_pattern_4x4 = (g_synth.current_controller_value(SEQ_PATTERN    ) >= 64);  // Std at 0-63, 4x4 at 64-127
    PRA32_U2_ControlPanel_update_play_display();
  } else if (control_number == SEQ_DIRECTION  ) {
    uint8_t controller_value = g_synth.current_controller_value(SEQ_DIRECTION  );
    uint32_t index = ((controller_value * 4) + 128) >> 8;
    s_seq_mode = index;
  } else if (control_number == SEQ_ON_STEPS   ) {
    s_seq_on_steps = g_synth.current_controller_value(SEQ_ON_STEPS   );
  } else if (control_number == SEQ_ACT_STEPS  ) {
    s_seq_act_steps = g_synth.current_controller_value(SEQ_ACT_STEPS  );
  } else if (control_number == VOICE_GRP_MODE ) {  // Off: back to the Main Synth, the only one that the panel can select then
    if (!PRA32_U2_ControlPanel_target_usable(s_current_synth)) {
      g_synth.control_change(PANEL_TARGET_CH, 0);
    }
    PRA32_U2_ControlPanel_update_ch_display();  // the name of the group (e.g. "[Ly2]")
    s_program_display_requested = true;         // the program of its first synth
  } else if (control_number == SEQ_CH_OFST    ) {
    PRA32_U2_ControlPanel_update_seq_ch_display();
  } else if (control_number == PANEL_TARGET_CH) {
    PRA32_U2_ControlPanel_select_target(g_synth.current_controller_value(PANEL_TARGET_CH));
  } else if (control_number == PANEL_MIDI_CH  ) {
    uint8_t midi_ch = g_synth.current_controller_value(PANEL_MIDI_CH  ) >> 3;
    g_midi_ch = midi_ch;
    PRA32_U2_ControlPanel_update_ch_display();
    PRA32_U2_ControlPanel_update_seq_ch_display();  // the channel of the Step Sequencer moves with it
  }

#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

// A Control Change on the Panel Channel (the Basic Channel + 14), so that the panel parameters and the Step
// Sequencer can be controlled by MIDI (e.g. without the panel); see README-PRA32-U2-P.md. The CC numbers are
// only those that the synth parameters use (the others are left for them), and avoid those that a DAW or a
// keyboard may send by itself (e.g. Bank Select, RPN/NRPN, Data Entry, Modulation, Breath, Pan, Expression,
// the pedals, and the Channel Mode Messages), not to change a parameter, nor to write to the flash, by chance:
// - Parameters (set as by the panel): Seq Pitch 0-7 (#16-23), Seq Velo 0-7 (#70-77), the Panel Parameters
//   (#51-56: Scale, Transpose, Play Mode, Play Pitch, Play Velo, and Pitch Ofst), and the other Step Sequencer Parameters (#85-90 and #92-94, the number + 37,
//   Seq Pitch Ofst #95, Panel MIDI Tx #59, Seq Ch Ofst #60, Seq Transpose #61, and Seq Pattern #62); Panel Target Ch (#58, PRA32-U2/M/P: the channel
//   that the panel selects, as the Basic Channel + the value) is only for the Panel Channel
// - MIDI Basic Ch (#57, 0-15 as 1-16) is only kept, and set by Set Basic Ch (#114, as an operation), so that the channels
//   (the Panel Channel too) do not move by one Control Change; kept in the flash by Write Panel Prms, as by the panel
// - Operations (#102-106, #112, and #113, ready at 0-31, and done at 96-127, as by the panel [Rdy] and [Exe]); not the
//   Read and Write of the programs, which the editor does
// - Panel Key (#110): 64-127 as the Play key pressed, and 0-63 as released, in Key Mode and K+S (Key + Seq Mode);
//   nothing in Seq Mode
// - Panel Play (#111): 64-127 as the Play key pressed (Key Mode) or the Step Sequencer started (Seq and K+S),
//   and 0-63 as released or stopped
#if defined(PRA32_U2_USE_CONTROL_PANEL)
static uint8_t s_panel_ch_basic_ch = 0xFF;  // MIDI Basic Ch (#57) to set by Set Basic Ch (#114), none at first

static uint8_t PRA32_U2_ControlPanel_panel_ch_target(uint8_t number) {
  if ((number >= 16) && (number <= 23)) { return SEQ_PITCH_0 + (number - 16); }
  if ((number >= 70) && (number <= 77)) { return SEQ_VELO_0  + (number - 70); }

  switch (number) {
  case  51: return PANEL_SCALE    ;
  case  52: return PANEL_TRANSPOSE;
  case  53: return PANEL_PLAY_MODE;
  case  54: return PANEL_PLAY_PIT ;
  case  55: return PANEL_PLAY_VELO;
  case  56: return PANEL_PIT_OFST ;
  case  59: return PANEL_MIDI_TX  ;
  case  60: return SEQ_CH_OFST    ;
  case  61: return SEQ_TRANSPOSE  ;
  case  62: return SEQ_PATTERN    ;
  case 102: return WR_PANEL_PRMS  ;  // Write and then Read, as the pages of the panel (C and D)
  case 103: return RD_PANEL_PRMS  ;
  case 104: return IN_PANEL_PRMS  ;
  case 105: return UNDO_WRITE     ;
  case 106: return UNDO_READ      ;
  case 112: return SEQ_RAND_PITCH ;
  case 113: return SEQ_RAND_VELO  ;
  }

  if ((number >= 85) && (number <= 94) && (number != 91)) { return 128 + (number - 37); }  // SEQ_TEMPO to SEQ_RX_ST_SP
  if (number == 95) { return SEQ_PIT_OFST; }  // not #91 (Effects 1 Depth, which a DAW or a keyboard may send)

  return 0xFF;
}
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)

void PRA32_U2_ControlPanel_on_panel_ch_control_change(uint8_t number, uint8_t value)
{
  static_cast<void>(number);
  static_cast<void>(value);

#if defined(PRA32_U2_USE_CONTROL_PANEL)
  if (number == 58) {  // Panel Target Ch (PANEL_TARGET_CH, see PRA32_U2_ControlPanel_select_target())
    g_synth.control_change(PANEL_TARGET_CH, value & 0x0F);
    return;
  }

  if (number == 57) {  // MIDI Basic Ch (0-15), set by Set Basic Ch (#114)
    s_panel_ch_basic_ch = value & 0x0F;
    return;
  }

  if (number == 114) {  // Set Basic Ch: as an operation (ready at 0-31, and done at 96-127); the Panel Channel moves too
    static boolean s_set_basic_ch_ready = false;
    if (value <= 31) {
      s_set_basic_ch_ready = true;
    } else if (s_set_basic_ch_ready && (value >= 96)) {
      s_set_basic_ch_ready = false;
      if ((s_panel_ch_basic_ch <= 15) && (s_panel_ch_basic_ch != g_midi_ch)) {
        PRA32_U2_ControlPanel_notes_off();  // off on the channels before
        g_synth.control_change(PANEL_MIDI_CH, s_panel_ch_basic_ch << 3);
      }
    }
    return;
  }

  if (number == 110) {  // Panel Key: compared with the state now, so that a value repeated does nothing
    boolean on = (value >= 64);
    if (PRA32_U2_ControlPanel_mode_has_key() && (on != s_key_held)) {  // Key and K+S
      PRA32_U2_ControlPanel_key_play(on);
    }
    return;
  }

  if (number == 111) {  // Panel Play: compared with the state now (which the Play key, Panel Play Mode, or
                        // MIDI Start/Stop may have changed), so that a value repeated does nothing
    boolean on = (value >= 64);
    boolean playing = (s_play_mode == 0) ? s_key_held : (s_playing_status != PlayingStatus_Stop);
    if (on != playing) {
      if (s_play_mode == 0) {  // Key Mode
        PRA32_U2_ControlPanel_key_play(on);
      } else {  // Seq and K+S: the Step Sequencer
        if (on) {
          PRA32_U2_ControlPanel_seq_start();
        } else {
          PRA32_U2_ControlPanel_seq_stop();
        }
      }
    }
    return;
  }

  uint8_t control_target = PRA32_U2_ControlPanel_panel_ch_target(number);
  if (control_target == 0xFF) {
    return;
  }

  if (control_target < 128 + 64) {
    g_synth.control_change(control_target, value);
  } else {
    static boolean s_ready_to_execute[128] = {};
    if (value <= 31) {
      s_ready_to_execute[number] = true;
    } else if (s_ready_to_execute[number] && (value >= 96)) {
      PRA32_U2_ControlPanel_execute_operation(control_target);
      s_ready_to_execute[number] = false;
    }
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
}

INLINE void PRA32_U2_ControlPanel_debug_print(uint32_t loop_counter) {
  static_cast<void>(loop_counter);

#if defined(PRA32_U2_USE_DEBUG_PRINT)
#if defined(PRA32_U2_USE_CONTROL_PANEL)
  switch (loop_counter) {
  case  5 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[7;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[0]));
    break;
  case  6 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[8;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[1]));
    break;
  case  7 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[9;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[2]));
    break;
  case  8 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[10;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[3]));
    break;
  case  9 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[11;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[4]));
    break;
  case 10 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[12;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[5]));
    break;
  case 11 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[13;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[6]));
    break;
  case 12 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[14;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(static_cast<char*>(s_display_buffer[7]));
    break;
#if defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
  case 13 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[16;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(s_adc_current_value[0]);
    break;
  case 14 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[17;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(s_adc_current_value[1]);
    break;
  case 15 * 400:
    PRA32_U2_DEBUG_PRINT_SERIAL.print("\e[18;1H\e[K");
    PRA32_U2_DEBUG_PRINT_SERIAL.print(s_adc_current_value[2]);
    break;
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL_ANALOG_INPUT)
  }
#endif  // defined(PRA32_U2_USE_CONTROL_PANEL)
#endif  // defined(PRA32_U2_USE_DEBUG_PRINT)
}
