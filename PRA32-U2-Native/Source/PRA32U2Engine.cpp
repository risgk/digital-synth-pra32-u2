// The only translation unit that includes the PRA32-U2 core (Digital-Synth-PRA32-U2/*.h).
// The core has global variables and functions in its headers, so do not include it elsewhere

#define PRA32_U2_ENABLE_POLY_ON_1_CORE

#if defined(PRA32_U2_ENGINE_M)
// PRA32-U2/M/P Web (PRA32_U2_ENGINE_M) has the panel parameters and the Step Sequencer of PRA32-U2/M/P,
// as PRA32-U2/M/P without the panel (no keys, no knobs, and no display), controlled by the Panel Channel
// (the Basic Channel + 14); the Native (without PRA32_U2_ENGINE_M) does not
#define PRA32_U2_USE_CONTROL_PANEL
#define PRA32_U2_USE_PANEL_CHANNEL
#define PRA32_U2_ENABLE_LAYERING
#define PRA32_U2_NUMBER_OF_SYNTHS              (4 + 3)      // The 4 synths, the Voice Group Mode (+ 4 and + 5), and the Layering (+ 15)
#define PRA32_U2_ANALOG_INPUT_DENOMINATOR      (480)        // Not used (no knobs)
#if !defined(PRA32_U2_VERSION)
#define PRA32_U2_VERSION                       "v?.?.?    "  // On the Info page (not shown without the display)
#endif  // !defined(PRA32_U2_VERSION)
#endif  // defined(PRA32_U2_ENGINE_M)

#include "PRA32U2NativeCompat.h"

uint8_t g_midi_ch = 0;  // Used by the core (only for the panel); the engine receives all channels

#include "pra32-u2-common.h"
#include "pra32-u2-synth.h"
#include "pra32-u2-value-display.h"  // The values shown as they work (also without the panel)

#include "PRA32U2Engine.h"

#include <algorithm>
#include <array>

#if defined(PRA32_U2_ENGINE_M)
// PRA32-U2/M (PRA32-U2/M/P Web): the 4 synths (the Main Synth and the Sub Synths 1-3), as "Digital-Synth-PRA32-U2-M.ino",
// with the MIDI messages by "pra32-u2-m-midi.h" (the Basic Channel 1, the Layering, and the Round Robin).
// They are global variables, as "pra32-u2-m-midi.h" uses them, so there is one of them at most in a process
typedef uint8_t byte;

PRA32_U2_Synth<false, false, true, 0, false> g_synth;
PRA32_U2_Synth<true,  false, true, 1, true>  g_sub_1_synth;
PRA32_U2_Synth<true,  false, true, 2, true>  g_sub_2_synth;
PRA32_U2_Synth<true,  false, true, 3, true>  g_sub_3_synth;
boolean g_synth_is_in_polyphonic_mode = true;

#include "pra32-u2-control-panel.h"
#include "pra32-u2-m-midi.h"
#endif  // defined(PRA32_U2_ENGINE_M)

namespace {

// The names of the parameters, the same as PRA32-U2 Editor (the parameters and their order are s_program_table_parameters of the core)
const PRA32U2Engine::ParameterInfo s_parameter_names[] = {
  { OSC_1_WAVE     , "Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]"    },
  { MIXER_SUB_OSC  , "Mixer Noise/Sub Osc [N to S]"              },
  { OSC_1_SHAPE    , "Osc 1 Shape"                            },
  { OSC_1_MORPH    , "Osc 1 Morph"                            },

  { OSC_2_WAVE     , "Osc 2 Wave [Saw|Sqr|Tri|Sin|O1|Nos]"    },
  { MIXER_OSC_MIX  , "Mixer Osc Mix [1 to 2]"                    },
  { OSC_2_COARSE   , "Osc 2 Coarse [- to +]"                     },
  { OSC_2_PITCH    , "Osc 2 Pitch [- to +]"                      },

  { FILTER_CUTOFF  , "Filter Cutoff"                          },
  { FILTER_RESO    , "Filter Resonance"                       },
  { FILTER_EG_AMT  , "Filter EG Amt [- to +]"                    },
  { FILTER_KEY_TRK , "Filter Key Track [- to +]"                 },

  { EG_ATTACK      , "EG Attack"                              },
  { EG_DECAY       , "EG Decay"                               },
  { EG_SUSTAIN     , "EG Sustain"                             },
  { EG_RELEASE     , "EG Release"                             },

  { EG_OSC_AMT     , "EG Mod Amt [- to +]"                       },
  { EG_OSC_DST     , "EG Mod Dst [P|P|2P|2P|F|1S]"            },
  { VOICE_MODE     , "Voice Mode [Pol|Pol|Mon|Mon|LP|Lgt]"    },
  { PORTAMENTO     , "Portamento Time"                        },

  { LFO_WAVE       , "LFO Wave [Tri|Sin|Nos|Saw|S&H|Sqr]"     },
  { LFO_FADE_TIME  , "LFO Fade Time"                          },
  { LFO_RATE       , "LFO Rate"                               },
  { LFO_DEPTH      , "LFO Depth"                              },

  { LFO_OSC_AMT    , "LFO Mod Amt [- to +]"                      },
  { LFO_OSC_DST    , "LFO Mod Dst [P|P|2P|2P|F|1S]"           },
  { LFO_FILTER_AMT , "LFO Filter Amt [- to +]"                   },
  { AMP_GAIN       , "Amp Gain"                               },

  { FILTER_MODE    , "Filter Mode [LP-BR-HP-BP-LP]"           },
  { FILTER_DRIVE   , "Filter Drive"                           },
  { EG_AMP_MOD     , "EG Amp Mod [Off|On]"                    },
  { REL_EQ_DECAY   , "EG/Amp Rel = Dec [Off|On]"              },

  { AMP_ATTACK     , "Amp Attack"                             },
  { AMP_DECAY      , "Amp Decay"                              },
  { AMP_SUSTAIN    , "Amp Sustain"                            },
  { AMP_RELEASE    , "Amp Release"                            },

  { OSC_DRIFT      , "Osc/Filter Drift"                       },
  { P_BEND_RANGE   , "Pitch Bend Range"                       },
  { COARSE_TUNE    , "Coarse Tune [- to +]"                      },
  { FINE_TUNE      , "Fine Tune [- to +]"                        },

  { A_D_VEL_SENS   , "EG Att/Dec Velo Sens [- to +]"             },
  { REL_VEL_SENS   , "EG Rel Velo Sens [- to +]"                 },
  { EG_VEL_SENS    , "EG Level Velo Sens"                     },
  { AMP_VEL_SENS   , "Amp Level Velo Sens"                    },

  { A_D_KEY_TRK    , "EG Att/Dec Key Track [- to +]"             },
  { VOICE_ASGN_MODE, "Voice Assign Mode [1|1|3|3|4|2]"        },
  { VOICE_GRP_MODE , "Voice Group Mode [Off|Ly2|RLy|RR2|RR4|RR3]" },
  { LFO_TRIG_MODE  , "LFO Trig Mode [Aut|Off|On|VOn|VOf|VAu]" },

  { BTH_FILTER_AMT , "Breath Filter Amt [- to +]"                },
  { BTH_AMP_MOD    , "Breath Amp Mode [Off|Q|L|LO|QO|Opn]"    },
  { AFT_T_LFO_AMT  , "After Touch LFO Amt"                    },
  { PORTA_MODE     , "Portamento Mode [Sw|Aut]"               },
  { OSC_SAW_W_MODE , "Osc Saw Wave Mode [Str|Cur]"            },
  { STRETCH_TUNE   , "Stretch Tune [- to +]"                     },
  { PAN            , "Pan"                                    },

  { DISTORTION     , "Distortion Drive"                       },
  { FX_ROUTING     , "FX Routing [Dst|Cho|Dly|Dly|Cmp|Dry]"   },

  { CHORUS_MIX     , "Chorus Level"                           },
  { CHORUS_RATE    , "Chorus Rate"                            },
  { CHORUS_DEPTH   , "Chorus Depth"                           },

  { DELAY_LEVEL    , "Delay Level"                            },
  { DELAY_MODE     , "Delay Mode [Ste|PP|Rev]"                },
  { DELAY_TIME     , "Delay Time"                             },
  { DELAY_FEEDBACK , "Delay Feedback"                         },
};

const int NUM_ALL_PARAMETERS = static_cast<int>(sizeof(s_program_table_parameters) / sizeof(s_program_table_parameters[0]));
#if defined(PRA32_U2_ENGINE_M)
const int NUM_PARAMETERS = NUM_ALL_PARAMETERS;
#else  // defined(PRA32_U2_ENGINE_M)
const int NUM_PARAMETERS = NUM_ALL_PARAMETERS - 1;  // Not Voice Group Mode, which only PRA32-U2/M uses
#endif  // defined(PRA32_U2_ENGINE_M)

static_assert(sizeof(s_parameter_names) / sizeof(s_parameter_names[0]) == NUM_ALL_PARAMETERS,
              "s_parameter_names must have the names of all the parameters of s_program_table_parameters");

const char* parameterNameFor(uint8_t controlNumber) {
  for (const PRA32U2Engine::ParameterInfo& info : s_parameter_names) {
    if (info.controlNumber == controlNumber) {
      return info.name;
    }
  }
  return "(Unknown)";  // Should not happen
}

// The parameters in the order of s_program_table_parameters
const std::array<PRA32U2Engine::ParameterInfo, NUM_PARAMETERS>& parameterInfos() {
  static const std::array<PRA32U2Engine::ParameterInfo, NUM_PARAMETERS> infos = [] {
    std::array<PRA32U2Engine::ParameterInfo, NUM_PARAMETERS> result {};
    int n = 0;
    for (int i = 0; i < NUM_ALL_PARAMETERS; ++i) {
      const uint8_t controlNumber = s_program_table_parameters[i];
#if !defined(PRA32_U2_ENGINE_M)
      if (controlNumber == VOICE_GRP_MODE) {
        continue;
      }
#endif  // !defined(PRA32_U2_ENGINE_M)
      result[static_cast<size_t>(n++)] = { controlNumber, parameterNameFor(controlNumber) };
    }
    return result;
  }();
  return infos;
}

const float OUTPUT_SCALE = 1.0f / static_cast<float>(1 << 23);  // The output of the core is 24-bit

}  // namespace

struct PRA32U2Engine::Impl {
  PRA32_U2_Synth<false, false, true> synth;  // The same as Digital-Synth-PRA32-U2.ino
#if !defined(PRA32_U2_ENGINE_M)
  // PRA32-U2 Native: the synths of the Synth IDs 1-3 too (as the Sub Synths 1-3 of PRA32-U2/M, with their own drift,
  // start phases, LFOs, and noise), so that the instances layered in the host are not all the same
  PRA32_U2_Synth<false, false, true, 1> synth_1;
  PRA32_U2_Synth<false, false, true, 2> synth_2;
  PRA32_U2_Synth<false, false, true, 3> synth_3;
  int      synth_id = 0;
#endif  // !defined(PRA32_U2_ENGINE_M)
  bool     m = false;   // PRA32-U2/M (with PRA32_U2_ENGINE_M only)
  uint32_t frame = 0;   // In a loop of 64 frames of "Digital-Synth-PRA32-U2-M.ino"

  // The synth of the Synth ID (0 without PRA32-U2 Native)
  template <typename F>
  void with_synth(F f) {
#if !defined(PRA32_U2_ENGINE_M)
    switch (synth_id) {
    case 1: f(synth_1); return;
    case 2: f(synth_2); return;
    case 3: f(synth_3); return;
    default: break;
    }
#endif  // !defined(PRA32_U2_ENGINE_M)
    f(synth);
  }
};

int PRA32U2Engine::getNumParameters() {
  return NUM_PARAMETERS;
}

const PRA32U2Engine::ParameterInfo& PRA32U2Engine::getParameterInfo(int index) {
  return parameterInfos()[static_cast<size_t>(index)];
}

// The same as the "markers" of the sliders of PRA32-U2 Editor (pra32-u2-editor.html)
int PRA32U2Engine::getMarkers(uint8_t controlNumber, const uint8_t** markerValues) {
  static const uint8_t MARKERS_6[]   = { 0, 26, 51, 77, 102, 127 };
  static const uint8_t MARKERS_3[]   = { 0, 64, 127 };
  static const uint8_t MARKERS_5[]   = { 0, 32, 64, 96, 127 };
  static const uint8_t MARKERS_2[]   = { 0, 127 };
  static const uint8_t MARKERS_C[]   = { 64 };  // The center, for [- to +], [N to S], and Pan
  static const uint8_t MARKERS_6_C[] = { 0, 26, 51, 64, 77, 102, 127 };  // The 6 markers and the center

  switch (controlNumber) {
  case OSC_1_WAVE      :
  case OSC_2_WAVE      :
  case EG_OSC_DST      :
  case VOICE_MODE      :
  case LFO_WAVE        :
  case LFO_TRIG_MODE   :
  case LFO_OSC_DST     :
  case VOICE_ASGN_MODE :
  case BTH_AMP_MOD     :
  case FX_ROUTING      :
  case VOICE_GRP_MODE  :
    *markerValues = MARKERS_6;
    return 6;
  case FILTER_MODE     :  // LP, BR, HP, BP, and LP
    *markerValues = MARKERS_5;
    return 5;
  case OSC_1_MORPH     :  // Can be used as a 6-way switch (the markers of the sliders do not pull the values)
    *markerValues = MARKERS_6_C;
    return 7;
  case DELAY_MODE      :
    *markerValues = MARKERS_3;
    return 3;
  case EG_AMP_MOD      :
  case REL_EQ_DECAY    :
  case OSC_SAW_W_MODE  :
  case PORTA_MODE      :
    *markerValues = MARKERS_2;
    return 2;
  case OSC_1_SHAPE     :  // The center (e.g. the pulse width 50% with Pls)
  case MIXER_SUB_OSC   :
  case OSC_2_COARSE    :
  case OSC_2_PITCH     :
  case FILTER_EG_AMT   :
  case FILTER_KEY_TRK  :
  case EG_OSC_AMT      :
  case LFO_OSC_AMT     :
  case LFO_FILTER_AMT  :
  case A_D_VEL_SENS    :
  case REL_VEL_SENS    :
  case A_D_KEY_TRK     :
  case STRETCH_TUNE    :
  case COARSE_TUNE     :
  case FINE_TUNE       :
  case BTH_FILTER_AMT  :
  case PAN             :
    *markerValues = MARKERS_C;
    return 1;
  default:
    *markerValues = nullptr;
    return 0;
  }
}

// The same as setRandomChorus() of PRA32-U2 Editor (pra32-u2-editor.html)
bool PRA32U2Engine::isRandomizedAsFx(uint8_t controlNumber) {
  switch (controlNumber) {
  case DISTORTION      :
  case CHORUS_MIX      :
  case CHORUS_RATE     :
  case CHORUS_DEPTH    :
  case DELAY_LEVEL     :
  case DELAY_TIME      :
  case DELAY_FEEDBACK  :
  case DELAY_MODE      :
    return true;
  default:
    return false;
  }
}

// The same as updateDisplayOfControl() of PRA32-U2 Editor (pra32-u2-editor.html)
std::string PRA32U2Engine::getValueText(uint8_t controlNumber, uint8_t value, uint8_t osc1Wave, uint8_t relatedValue) {
  const int v = value;
  std::string dispValue = std::to_string(v);

  auto bipolar = [](int x) { return (x >= 64) ? ("+" + std::to_string(x - 64)) : std::to_string(x - 64); };
  // Two sides as it works, 2 values a step: -64 (0), -62 (1-2), ..., 0 (63-64), ..., +64 (127), e.g. "N64", "0", "S64"
  auto twoSides = [](int x, const char* minus, const char* plus) {
    const int amount = ((x - 63) >= 0) ? (((x - 63) / 2) * 2) : -((((63 - x) + 1) / 2) * 2);
    return (amount == 0) ? std::string("0") : (amount > 0) ? (plus + std::to_string(amount)) : (minus + std::to_string(-amount));
  };
  auto select6 = [](int x, const char* const (&labels)[6]) { return std::string(labels[((x * 10) + 128) >> 8]); };
  auto select3 = [](int x, const char* const (&labels)[3]) { return std::string(labels[((x * 4) + 128) >> 8]); };
  auto select2 = [](int x, const char* const (&labels)[2]) { return std::string(labels[((x * 2) + 128) >> 8]); };

  static const char* const OSC_1_WAVES[6]  = { "Saw", "Sqr", "Tri", "Sin", "WT", "Pls" };
  static const char* const OSC_2_WAVES[6]  = { "Saw", "Sqr", "Tri", "Sin", "O1", "Nos" };
  static const char* const MOD_DSTS[6]     = { "P", "P", "2P", "2P", "F", "1S" };
  static const char* const VOICE_MODES[6]  = { "Pol", "Pol", "Mon", "Mon", "LP", "Lgt" };
  static const char* const LFO_WAVES[6]    = { "Tri", "Sin", "Nos", "Saw", "S&H", "Sqr" };
  static const char* const FILTER_MODES[9] = { "LP", "L-R", "BR", "R-H", "HP", "H-B", "BP", "B-L", "LP" };
  static const char* const OFF_ON[2]       = { "Off", "On" };
  static const char* const SAW_W_MODES[2]  = { "Str", "Cur" };
  static const char* const PORTA_MODES[2]  = { "Sw", "Aut" };
  static const char* const BTH_AMP_MODS[6] = { "Off", "Q", "L", "LO", "QO", "Opn" };
  static const char* const LFO_TRIG_MODES[6] = { "Aut", "Off", "On", "VOn", "VOf", "VAu" };
  static const char* const FX_ROUTINGS[6]  = { "Dst", "Cho", "Dly", "Dly", "Cmp", "Dry" };
  static const char* const ASGN_MODES[6]   = { "1", "1", "3", "3", "4", "2" };
  static const char* const DELAY_MODES[3]  = { "Ste", "PP", "Rev" };
  static const char* const GROUP_MODES[6]  = { "Off", "Ly2", "RLy", "RR2", "RR4", "RR3" };

  // Osc 2 Pitch (osc2Pitch), and EG Mod Amt and LFO Mod Amt to the pitch (P and 2P), as pitchDispValue() of PRA32-U2 Editor
  // (3 characters at most): the cents, rounded, and the semitones, with "." for .5, with the sign but where it does not fit
  auto pitchText = [](int x, bool osc2Pitch) {
    static const int SEMITONES[8] = { 30, 36, 42, 48, 60, 72, 96, 120 };  // 2.5 to 10 octaves
    const int n = x - 64;
    int a = std::abs(n);
    const std::string sign = (n > 0) ? "+" : "-";
    if (osc2Pitch) {
      a = std::min(a, 55);
      if (a == 0) { return std::string("0"); }
      if (a <= 32) { return sign + std::to_string(((a * 25) + 8) / 16); }  // 1.5625 cents a value
      const int halves = a - 31;  // 0.5 semitones a value: 2 (1 semitone) to 24 (12 semitones)
      if (halves & 1) { return (((halves / 2) < 10) ? sign : std::string()) + std::to_string(halves / 2) + "."; }
      return sign + std::to_string(halves / 2);
    }
    a = std::min(a, 63);
    if (a == 0) { return std::string("0"); }
    if (a <= 31) { return sign + std::to_string(((a * 25) + 4) / 8); }  // 3.125 cents a value
    const int semitones = (a <= 55) ? (a - 31) : SEMITONES[a - 56];  // 1 (100 cents) to 120 (10 octaves)
    return ((semitones < 100) ? sign : std::string()) + std::to_string(semitones);
  };

  switch (controlNumber) {
  case OSC_2_COARSE    : dispValue = bipolar(std::min(std::max(v, 64 - 60), 64 + 60)); break;  // -60 to +60, as it works
  case OSC_2_PITCH     : return std::to_string(v) + " [" + pitchText(v, true) + "]";  // shown even if the same as the value
  case EG_OSC_AMT      :
  case LFO_OSC_AMT     :  // As the pitch, if EG Mod Dst or LFO Mod Dst is P or 2P; as the value otherwise
    if ((((relatedValue * 10) + 128) >> 8) <= 3) {
      return std::to_string(v) + " [" + pitchText(v, false) + "]";
    }
    dispValue = bipolar(v);
    break;
  case COARSE_TUNE     : dispValue = bipolar(v);                     break;
  case FILTER_CUTOFF   :
  case FINE_TUNE       :
  case FILTER_RESO     :
  case FILTER_DRIVE    :
  case DISTORTION      :
  case STRETCH_TUNE    :
  case FILTER_EG_AMT   :
  case LFO_FILTER_AMT  :
  case BTH_FILTER_AMT  :
  case FILTER_KEY_TRK  :
  case A_D_VEL_SENS    :
  case REL_VEL_SENS    :
  case A_D_KEY_TRK     :
  case EG_VEL_SENS     :
  case AMP_VEL_SENS    :
  case AMP_GAIN        :
  case EG_SUSTAIN      :
  case AMP_SUSTAIN     :
  case MIXER_OSC_MIX   :
  case LFO_DEPTH       :
  case CHORUS_MIX      :
  case DELAY_LEVEL     :
  case DELAY_FEEDBACK  : {  // As the panel (the same functions): e.g. "262", "1k2", "20k" (Hz), "100" (-100 cents), "5.7" (Q), "+6" (dB), "50%"
    char text[5] = {};
    if (controlNumber == FILTER_CUTOFF) {
      PRA32_U2_ControlPanel_calc_value_display_cutoff(value, text);
    } else if (controlNumber == FINE_TUNE) {
      PRA32_U2_ControlPanel_calc_value_display_fine_tune(value, text);
    } else if (!PRA32_U2_ControlPanel_calc_value_display_percent(controlNumber, value, text)) {
      PRA32_U2_ControlPanel_calc_value_display_filter_tune(controlNumber, value, text);
    }
    std::string trimmed(text);
    trimmed.erase(0, trimmed.find_first_not_of(' '));
    return std::to_string(v) + " [" + trimmed + "]";  // shown even if the same as the value, as the panel
  }
  case EG_ATTACK       :
  case AMP_ATTACK      :
  case EG_DECAY        :
  case EG_RELEASE      :
  case AMP_DECAY       :
  case AMP_RELEASE     : {  // As timeDispValue() of PRA32-U2 Editor: the milliseconds without the unit, and the seconds with "."
    static const uint32_t MANTISSAS[32] = {
      10000, 10746, 11548, 12409, 13335, 14330, 15399, 16548, 17783, 19110, 20535, 22067, 23714, 25483, 27384, 29427,
      31623, 33982, 36517, 39242, 42170, 45316, 48697, 52330, 56234, 60430, 64938, 69783, 74989, 80584, 86596, 93057
    };
    static const uint32_t DECADES[4] = { 1, 10, 100, 1000 };
    const bool decayRelease = (controlNumber != EG_ATTACK) && (controlNumber != AMP_ATTACK);
    const uint32_t t = MANTISSAS[v & 0x1F] * (decayRelease ? 3 : 1) * DECADES[(v >> 5) & 0x03];  // in 1/10000 ms
    std::string text;
    if (t < 9995000) {
      text = std::to_string((t + 5000) / 10000);
    } else {
      const uint32_t tenths = (t + 500000) / 1000000;
      text = (tenths < 100) ? (std::to_string(tenths / 10) + "." + std::to_string(tenths % 10))
                            : (std::to_string((t + 5000000) / 10000000) + ".");
    }
    return std::to_string(v) + " [" + text + "]";  // shown even if the same as the value, as the panel
  }
  case PORTAMENTO      :
  case LFO_FADE_TIME   :
  case LFO_RATE        :
  case CHORUS_RATE     :
  case CHORUS_DEPTH    : {  // As the panel (the same function, PRA32_U2_ControlPanel_calc_value_display_time_rate())
    char text[5] = {};
    PRA32_U2_ControlPanel_calc_value_display_time_rate(controlNumber, value, relatedValue, text);
    std::string trimmed(text);
    trimmed.erase(0, trimmed.find_first_not_of(' '));
    return std::to_string(v) + " [" + trimmed + "]";  // shown even if the same as the value, as the panel
  }
  case PAN             : dispValue = (v > 64) ? ("R" + std::to_string(v - 64)) : (v == 64) ? std::string("C")
                                                  : ("L" + std::to_string(std::min(64 - v, 63)));  break;  // L63 (0 and 1) to R63
  case MIXER_SUB_OSC   : dispValue = twoSides(v, "N", "S");                   break;  // N64 to 0 to S64, as it works
  case OSC_1_SHAPE     :
  case OSC_1_MORPH     : {  // by Osc 1 Wave, as osc1DispValue() of PRA32-U2 Editor (3 characters at most)
    const int wave = ((osc1Wave * 10) + 128) >> 8;
    std::string text;
    if (controlNumber == OSC_1_SHAPE) {
      if (wave == 5) {  // 0% (0) to 50% (64) to 100% (127, shown as "100")
        const int width = (v >= 127) ? 100 : (((100 * v) + 64) / 128);
        text = std::to_string(width) + ((width < 100) ? "%" : "");
      }
    } else if (wave == 3) {
      const int halves = ((v + 2) >> 2) + 1;  // the ratio (0.5 to 16.5) * 2: "0.5" to "9.5", and "10" to "16." ("." for .5)
      text = ((halves >> 1) < 10) ? (std::to_string(halves >> 1) + "." + ((halves & 1) ? "5" : "0"))
                                  : (std::to_string(halves >> 1) + ((halves & 1) ? "." : ""));
    } else if (wave == 4) {
      text = "WT" + std::to_string(((v * 10) + 128) >> 8);
    } else if (wave == 5) {
      text = twoSides(v, "P", "S");
    }
    if (!text.empty()) {
      return std::to_string(v) + " [" + text + "]";  // shown even if the same as the value, as the panel
    }
    break;
  }
  case OSC_1_WAVE      : dispValue = select6(v, OSC_1_WAVES);        break;
  case OSC_2_WAVE      : dispValue = select6(v, OSC_2_WAVES);        break;
  case VOICE_GRP_MODE  : dispValue = select6(v, GROUP_MODES);        break;
  case EG_OSC_DST      :
  case LFO_OSC_DST     : dispValue = select6(v, MOD_DSTS);           break;
  case VOICE_MODE      : dispValue = select6(v, VOICE_MODES);        break;
  case LFO_WAVE        : dispValue = select6(v, LFO_WAVES);          break;
  case FILTER_MODE     : {  // LP, BR, HP, BP, and LP at 0, 32, 64, 96, and 127 (taken as 128); the two ends in between
    int x = (v >= 127) ? 128 : v;
    dispValue = std::string(FILTER_MODES[((x % 32) == 0) ? (x / 16) : ((x / 32) * 2 + 1)]);
    break;
  }
  case EG_AMP_MOD      :
  case REL_EQ_DECAY    : dispValue = select2(v, OFF_ON);             break;
  case OSC_SAW_W_MODE  : dispValue = select2(v, SAW_W_MODES);        break;
  case PORTA_MODE      : dispValue = select2(v, PORTA_MODES);        break;
  case LFO_TRIG_MODE   : dispValue = select6(v, LFO_TRIG_MODES);      break;
  case BTH_AMP_MOD     : dispValue = select6(v, BTH_AMP_MODS);       break;
  case FX_ROUTING      : dispValue = select6(v, FX_ROUTINGS);        break;
  case VOICE_ASGN_MODE : dispValue = select6(v, ASGN_MODES);         break;
  case DELAY_MODE      : dispValue = select3(v, DELAY_MODES);        break;
  case DELAY_TIME      :
    if (v < 5) {
      dispValue = std::to_string(v + 1);
    } else if (v < 27) {
      dispValue = std::to_string((v * 2) - 4);
    } else if (v < 114) {
      dispValue = std::to_string((((v * 20) + 3) / 6) - 40);
    } else {
      dispValue = "340";
    }
    break;
  default:
    break;
  }

  if (dispValue == std::to_string(v)) {
    return std::to_string(v);
  }
  return std::to_string(v) + " [" + dispValue + "]";
}

PRA32U2Engine::PRA32U2Engine(bool m)
: m_impl(std::make_unique<Impl>())
{
#if defined(PRA32_U2_ENGINE_M)
  m_impl->m = m;
  if (m) {
    g_synth.initialize();
    g_sub_1_synth.initialize();
    g_sub_2_synth.initialize();
    g_sub_3_synth.initialize();
    PRA32_U2_ControlPanel_setup();
    PRA32_U2_ControlPanel_initialize_parameters();
    return;
  }
#else
  (void) m;
  m_impl->synth_1.initialize();
  m_impl->synth_2.initialize();
  m_impl->synth_3.initialize();
#endif  // defined(PRA32_U2_ENGINE_M)
  m_impl->synth.initialize();
}

#if !defined(PRA32_U2_ENGINE_M)
int PRA32U2Engine::getSynthId() const {
  return m_impl->synth_id;
}

// The current values of the sound parameters and of the performance controllers go to the new synth, and the notes of the
// old one are turned off
void PRA32U2Engine::setSynthId(int synthId) {
  synthId &= 3;
  if (synthId == m_impl->synth_id) {
    return;
  }
  uint8_t values[128] = {};
  m_impl->with_synth([&](auto& s) {
    for (uint8_t number = 0; number < 128; ++number) {
      values[number] = s.current_controller_value(number);
    }
    s.all_notes_off(true);
  });
  m_impl->synth_id = synthId;
  m_impl->with_synth([&](auto& s) {
    for (int i = 0; i < NUM_ALL_PARAMETERS; ++i) {
      const uint8_t number = s_program_table_parameters[i];
      s.control_change(number, values[number]);
    }
    static const uint8_t PERFORMANCE_CONTROLLERS[] = { MODULATION, BTH_CONTROLLER, EXPRESSION, PORTAMENTO_SW };
    for (uint8_t number : PERFORMANCE_CONTROLLERS) {
      s.control_change(number, values[number]);
    }
  });
}
#endif  // !defined(PRA32_U2_ENGINE_M)

PRA32U2Engine::~PRA32U2Engine() = default;

void PRA32U2Engine::handleMidiMessage(const uint8_t* data, int size) {
  if (size < 1) {
    return;
  }

  uint8_t status = data[0] & 0xF0;
  uint8_t data_1 = (size >= 2) ? (data[1] & 0x7F) : 0;
  uint8_t data_2 = (size >= 3) ? (data[2] & 0x7F) : 0;

#if defined(PRA32_U2_ENGINE_M)
  if (m_impl->m) {
    // MIDI Clock, Start, and Stop, for the Step Sequencer with Seq Clock Src Ext, as "Digital-Synth-PRA32-U2-M.ino"
    if (data[0] == 0xF8) {
      PRA32_U2_ControlPanel_on_clock();
      return;
    } else if ((data[0] == 0xFA) || (data[0] == 0xFC)) {
      if (g_synth.current_controller_value(SEQ_RX_ST_SP   ) >= 64) {
        if (data[0] == 0xFA) { PRA32_U2_ControlPanel_on_start(); } else { PRA32_U2_ControlPanel_on_stop(); }
      }
      return;
    }

    // By the channel, as "Digital-Synth-PRA32-U2-M.ino" (a Program Change is processed in one of the next loops)
    byte channel = (data[0] & 0x0F) + 1;
    switch (status) {
    case 0x80: if (size >= 3) { handleNoteOff(channel, data_1, data_2); } break;
    case 0x90:
      if (size >= 3) {
        if (data_2 == 0) { handleNoteOff(channel, data_1, 64); } else { handleNoteOn(channel, data_1, data_2); }
      }
      break;
    case 0xA0: if (size >= 3) { handleAfterTouchPoly(channel, data_1, data_2); } break;
    case 0xB0: if (size >= 3) { handleControlChange(channel, data_1, data_2); } break;
    case 0xC0: if (size >= 2) { handleProgramChange(channel, data_1); } break;
    case 0xD0: if (size >= 2) { handleAfterTouchChannel(channel, data_1); } break;
    case 0xE0: if (size >= 3) { handlePitchBend(channel, ((data_2 << 7) | data_1) - 8192); } break;
    default: break;
    }
    return;
  }
#endif  // defined(PRA32_U2_ENGINE_M)

  switch (status) {
  case 0x80:
    if (size >= 3) { m_impl->with_synth([&](auto& s) { s.note_off(data_1, data_2); }); }
    break;
  case 0x90:
    if (size >= 3) {
      if (data_2 == 0) {
        m_impl->with_synth([&](auto& s) { s.note_off(data_1, 64); });
      } else {
        m_impl->with_synth([&](auto& s) { s.note_on(data_1, data_2); });
      }
    }
    break;
  case 0xA0:
    if (size >= 3) { m_impl->with_synth([&](auto& s) { s.after_touch_poly(data_1, data_2); }); }
    break;
  case 0xB0:
    if (size >= 3) { m_impl->with_synth([&](auto& s) { s.control_change(data_1, data_2); }); }
    break;
  case 0xC0:
    if (size >= 2) { m_impl->with_synth([&](auto& s) { s.program_change(data_1); }); }
    break;
  case 0xD0:
    if (size >= 2) { m_impl->with_synth([&](auto& s) { s.after_touch_channel(data_1); }); }
    break;
  case 0xE0:
    if (size >= 3) { m_impl->with_synth([&](auto& s) { s.pitch_bend(data_1, data_2); }); }
    break;
  default:
    break;
  }
}

// (For PRA32-U2/M, those of the Main Synth)
void PRA32U2Engine::controlChange(uint8_t controlNumber, uint8_t value) {
#if defined(PRA32_U2_ENGINE_M)
  if (m_impl->m) {
    g_synth.control_change(controlNumber & 0x7F, value & 0x7F);
    return;
  }
#endif  // defined(PRA32_U2_ENGINE_M)
  m_impl->with_synth([&](auto& s) { s.control_change(controlNumber & 0x7F, value & 0x7F); });
}

uint8_t PRA32U2Engine::getOsc1Wave() const {
  return getControllerValue(OSC_1_WAVE);
}

uint8_t PRA32U2Engine::getRelatedValue(uint8_t controlNumber) const {
  return (controlNumber == EG_OSC_AMT)  ? getControllerValue(EG_OSC_DST)  :
         (controlNumber == LFO_OSC_AMT) ? getControllerValue(LFO_OSC_DST) :
         (controlNumber == LFO_RATE)    ? getControllerValue(LFO_WAVE)    : 0;
}

uint8_t PRA32U2Engine::getControllerValue(uint8_t controlNumber) const {
#if defined(PRA32_U2_ENGINE_M)
  if (m_impl->m) {
    return g_synth.current_controller_value(controlNumber & 0x7F);
  }
#endif  // defined(PRA32_U2_ENGINE_M)
  uint8_t value = 0;
  m_impl->with_synth([&](auto& s) { value = s.current_controller_value(controlNumber & 0x7F); });
  return value;
}

void PRA32U2Engine::process(float& left, float& right) {
#if defined(PRA32_U2_ENGINE_M)
  if (m_impl->m) {
    // A loop of 64 frames, and the synths of each frame in the order of "Digital-Synth-PRA32-U2-M.ino" (on one core);
    // the Sub Synths only while the Main Synth is not in Poly
    if (m_impl->frame == 0) {
      processOnePendingProgramChange();
      PRA32_U2_ControlPanel_update_control();  // The Step Sequencer and the panel play, a loop (as loop() of the sketch)
      boolean mode = g_synth.is_in_polyphonic_mode();
      if (g_synth_is_in_polyphonic_mode != mode) {
        g_synth_is_in_polyphonic_mode = mode;
        g_sub_1_synth.all_notes_off(true);
        g_sub_2_synth.all_notes_off(true);
        g_sub_3_synth.all_notes_off(true);
        roundRobinReset();
      }
    }
    m_impl->frame = (m_impl->frame + 1) & 63;

    PRA32_U2_StereoSample sub_1_output = { 0, 0 };
    PRA32_U2_StereoSample sub_2_output = { 0, 0 };
    PRA32_U2_StereoSample sub_3_output = { 0, 0 };
    if (g_synth_is_in_polyphonic_mode == false) {
      sub_2_output = g_sub_2_synth.process(0, 0);
    }
    PRA32_U2_FxBusSample fx_bus;
    clear_fx_bus(fx_bus);
    g_sub_2_synth.add_to_fx_bus(sub_2_output, fx_bus);
    PRA32_U2_StereoSample main_output = g_synth.process<false, true>(0, 0);
    if (g_synth_is_in_polyphonic_mode == false) {
      sub_3_output = g_sub_3_synth.process(0, 0);
      sub_1_output = g_sub_1_synth.process(0, 0);
    }
    g_sub_3_synth.add_to_fx_bus(sub_3_output, fx_bus);
    g_sub_1_synth.add_to_fx_bus(sub_1_output, fx_bus);
    g_synth.add_to_fx_bus(main_output, fx_bus);
    PRA32_U2_StereoSample output = g_synth.process_fx(fx_bus);
    left  = static_cast<float>(output.left)  * OUTPUT_SCALE;
    right = static_cast<float>(output.right) * OUTPUT_SCALE;
    return;
  }
#endif  // defined(PRA32_U2_ENGINE_M)
  PRA32_U2_StereoSample output;
  m_impl->with_synth([&](auto& s) { output = s.process(0, 0); });
  left  = static_cast<float>(output.left)  * OUTPUT_SCALE;
  right = static_cast<float>(output.right) * OUTPUT_SCALE;
}
