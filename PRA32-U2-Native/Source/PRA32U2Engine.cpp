// The only translation unit that includes the PRA32-U2 core (Digital-Synth-PRA32-U2/*.h).
// The core has global variables and functions in its headers, so do not include it elsewhere

#define PRA32_U2_ENABLE_POLY_ON_1_CORE

#include "PRA32U2NativeCompat.h"

uint8_t g_midi_ch = 0;  // Used by the core (only for the panel); the engine receives all channels

#include "pra32-u2-common.h"
#include "pra32-u2-synth.h"

#include "PRA32U2Engine.h"

#include <array>

namespace {

// The names of the parameters, the same as PRA32-U2 Editor (the parameters and their order are s_program_table_parameters of the core)
const PRA32U2Engine::ParameterInfo s_parameter_names[] = {
  { OSC_1_WAVE     , "Osc 1 Wave [Saw|Sqr|Tri|Sin|WT|Pls]"    },
  { MIXER_SUB_OSC  , "Mixer Noise/Sub Osc [N|S]"              },
  { OSC_1_SHAPE    , "Osc 1 Shape"                            },
  { OSC_1_MORPH    , "Osc 1 Morph"                            },

  { OSC_2_WAVE     , "Osc 2 Wave [Saw|Sqr|Tri|Sin|O1|Nos]"    },
  { MIXER_OSC_MIX  , "Mixer Osc Mix [1|2]"                    },
  { OSC_2_COARSE   , "Osc 2 Coarse [-|+]"                     },
  { OSC_2_PITCH    , "Osc 2 Pitch [-|+]"                      },

  { FILTER_CUTOFF  , "Filter Cutoff"                          },
  { FILTER_RESO    , "Filter Resonance"                       },
  { FILTER_EG_AMT  , "Filter EG Amt [-|+]"                    },
  { FILTER_KEY_TRK , "Filter Key Track [-|+]"                 },

  { EG_ATTACK      , "EG Attack"                              },
  { EG_DECAY       , "EG Decay"                               },
  { EG_SUSTAIN     , "EG Sustain"                             },
  { EG_RELEASE     , "EG Release"                             },

  { EG_OSC_AMT     , "EG Mod Amt [-|+]"                       },
  { EG_OSC_DST     , "EG Mod Dst [P|P|2P|2P|F|1S]"            },
  { VOICE_MODE     , "Voice Mode [Pol|Pol|Mon|Mon|LP|Lgt]"    },
  { PORTAMENTO     , "Portamento"                             },

  { LFO_WAVE       , "LFO Wave [Tri|Sin|Nos|Saw|S&H|Sqr]"     },
  { LFO_FADE_TIME  , "LFO Fade Time"                          },
  { LFO_RATE       , "LFO Rate"                               },
  { LFO_DEPTH      , "LFO Depth"                              },

  { LFO_OSC_AMT    , "LFO Mod Amt [-|+]"                      },
  { LFO_OSC_DST    , "LFO Mod Dst [P|P|2P|2P|F|1S]"           },
  { LFO_FILTER_AMT , "LFO Filter Amt [-|+]"                   },
  { AMP_GAIN       , "Amp Gain"                               },

  { AMP_ATTACK     , "Amp Attack"                             },
  { AMP_DECAY      , "Amp Decay"                              },
  { AMP_SUSTAIN    , "Amp Sustain"                            },
  { AMP_RELEASE    , "Amp Release"                            },

  { FILTER_MODE    , "Filter Mode [LP|BP|HP]"                 },
  { P_BEND_RANGE   , "Pitch Bend Range"                       },
  { EG_AMP_MOD     , "EG Amp Mod [Off|On]"                    },
  { REL_EQ_DECAY   , "EG/Amp Rel = Dec [Off|On]"              },

  { A_D_VEL_SENS   , "EG Att/Dec Velo Sens [-|+]"             },
  { REL_VEL_SENS   , "EG Rel Velo Sens [-|+]"                 },
  { EG_VEL_SENS    , "EG Level Velo Sens"                     },
  { AMP_VEL_SENS   , "Amp Level Velo Sens"                    },

  { A_D_KEY_TRK    , "EG Att/Dec Key Track [-|+]"             },
  { VOICE_ASGN_MODE, "Voice Assign Mode [1|1|3|3|4|2]"        },
  { PAN            , "Pan"                                    },
  { STRETCH_TUNE   , "Stretch Tune [-|+]"                     },

  { OSC_DRIFT      , "Osc/Filter Drift"                       },
  { OSC_SAW_W_MODE , "Osc Saw Wave Mode [Str|Cur]"            },
  { COARSE_TUNE    , "Coarse Tune [-|+]"                      },
  { FINE_TUNE      , "Fine Tune [-|+]"                        },

  { BTH_FILTER_AMT , "Breath Filter Amt [-|+]"                },
  { BTH_AMP_MOD    , "Breath Amp Mode [Off|Q|L|LO|QO|Opn]"    },
  { AFT_T_LFO_AMT  , "After Touch LFO Amt"                    },

  { CHORUS_MIX     , "Chorus Level"                           },
  { FX_ROUTING     , "FX Routing [Cho|Dly|Byp]"               },
  { CHORUS_RATE    , "Chorus Rate"                            },
  { CHORUS_DEPTH   , "Chorus Depth"                           },

  { DELAY_LEVEL    , "Delay Level"                            },
  { DELAY_MODE     , "Delay Mode [S|P|R]"                     },
  { DELAY_TIME     , "Delay Time"                             },
  { DELAY_FEEDBACK , "Delay Feedback"                         },
};

const int NUM_PARAMETERS = static_cast<int>(sizeof(s_program_table_parameters) / sizeof(s_program_table_parameters[0]));

static_assert(sizeof(s_parameter_names) / sizeof(s_parameter_names[0]) == NUM_PARAMETERS,
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
    for (int i = 0; i < NUM_PARAMETERS; ++i) {
      const uint8_t controlNumber = s_program_table_parameters[i];
      result[static_cast<size_t>(i)] = { controlNumber, parameterNameFor(controlNumber) };
    }
    return result;
  }();
  return infos;
}

const float OUTPUT_SCALE = 1.0f / static_cast<float>(1 << 23);  // The output of the core is 24-bit

}  // namespace

struct PRA32U2Engine::Impl {
  PRA32_U2_Synth<false, false, true> synth;  // The same as Digital-Synth-PRA32-U2.ino
};

int PRA32U2Engine::getNumParameters() {
  return NUM_PARAMETERS;
}

const PRA32U2Engine::ParameterInfo& PRA32U2Engine::getParameterInfo(int index) {
  return parameterInfos()[static_cast<size_t>(index)];
}

// The same as the "markers" of the sliders of PRA32-U2 Editor (pra32-u2-editor.html)
int PRA32U2Engine::getMarkers(uint8_t controlNumber, const uint8_t** markerValues) {
  static const uint8_t MARKERS_6[] = { 0, 26, 51, 77, 102, 127 };
  static const uint8_t MARKERS_3[] = { 0, 64, 127 };
  static const uint8_t MARKERS_2[] = { 0, 127 };

  switch (controlNumber) {
  case OSC_1_WAVE      :
  case OSC_2_WAVE      :
  case EG_OSC_DST      :
  case VOICE_MODE      :
  case LFO_WAVE        :
  case LFO_OSC_DST     :
  case VOICE_ASGN_MODE :
  case BTH_AMP_MOD     :
    *markerValues = MARKERS_6;
    return 6;
  case FILTER_MODE     :
  case FX_ROUTING      :
  case DELAY_MODE      :
    *markerValues = MARKERS_3;
    return 3;
  case EG_AMP_MOD      :
  case REL_EQ_DECAY    :
  case OSC_SAW_W_MODE  :
    *markerValues = MARKERS_2;
    return 2;
  default:
    *markerValues = nullptr;
    return 0;
  }
}

// The same as updateDisplayOfControl() of PRA32-U2 Editor (pra32-u2-editor.html)
std::string PRA32U2Engine::getValueText(uint8_t controlNumber, uint8_t value) {
  const int v = value;
  std::string dispValue = std::to_string(v);

  auto bipolar = [](int x) { return (x >= 64) ? ("+" + std::to_string(x - 64)) : std::to_string(x - 64); };
  auto select6 = [](int x, const char* const (&labels)[6]) { return std::string(labels[((x * 10) + 128) >> 8]); };
  auto select3 = [](int x, const char* const (&labels)[3]) { return std::string(labels[((x * 4) + 128) >> 8]); };
  auto select2 = [](int x, const char* const (&labels)[2]) { return std::string(labels[((x * 2) + 128) >> 8]); };

  static const char* const OSC_1_WAVES[6]  = { "Saw", "Sqr", "Tri", "Sin", "WT", "Pls" };
  static const char* const OSC_2_WAVES[6]  = { "Saw", "Sqr", "Tri", "Sin", "O1", "Nos" };
  static const char* const MOD_DSTS[6]     = { "P", "P", "2P", "2P", "F", "1S" };
  static const char* const VOICE_MODES[6]  = { "Pol", "Pol", "Mon", "Mon", "LP", "Lgt" };
  static const char* const LFO_WAVES[6]    = { "Tri", "Sin", "Nos", "Saw", "S&H", "Sqr" };
  static const char* const FILTER_MODES[3] = { "LP", "BP", "HP" };
  static const char* const OFF_ON[2]       = { "Off", "On" };
  static const char* const SAW_W_MODES[2]  = { "Str", "Cur" };
  static const char* const BTH_AMP_MODS[6] = { "Off", "Q", "L", "LO", "QO", "Opn" };
  static const char* const FX_ROUTINGS[3]  = { "Cho", "Dly", "Byp" };
  static const char* const ASGN_MODES[6]   = { "1", "1", "3", "3", "4", "2" };
  static const char* const DELAY_MODES[3]  = { "S", "P", "R" };

  switch (controlNumber) {
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
  case PAN             :
  case STRETCH_TUNE    :
  case COARSE_TUNE     :
  case FINE_TUNE       :
  case BTH_FILTER_AMT  : dispValue = bipolar(v);                     break;
  case MIXER_SUB_OSC   : dispValue = (v >= 64) ? ("S" + std::to_string(v - 64)) : ("N" + std::to_string(64 - v)); break;
  case OSC_1_WAVE      : dispValue = select6(v, OSC_1_WAVES);        break;
  case OSC_2_WAVE      : dispValue = select6(v, OSC_2_WAVES);        break;
  case EG_OSC_DST      :
  case LFO_OSC_DST     : dispValue = select6(v, MOD_DSTS);           break;
  case VOICE_MODE      : dispValue = select6(v, VOICE_MODES);        break;
  case LFO_WAVE        : dispValue = select6(v, LFO_WAVES);          break;
  case FILTER_MODE     : dispValue = select3(v, FILTER_MODES);       break;
  case EG_AMP_MOD      :
  case REL_EQ_DECAY    : dispValue = select2(v, OFF_ON);             break;
  case OSC_SAW_W_MODE  : dispValue = select2(v, SAW_W_MODES);        break;
  case BTH_AMP_MOD     : dispValue = select6(v, BTH_AMP_MODS);       break;
  case FX_ROUTING      : dispValue = select3(v, FX_ROUTINGS);        break;
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

PRA32U2Engine::PRA32U2Engine()
: m_impl(std::make_unique<Impl>())
{
  m_impl->synth.initialize();
}

PRA32U2Engine::~PRA32U2Engine() = default;

void PRA32U2Engine::handleMidiMessage(const uint8_t* data, int size) {
  if (size < 1) {
    return;
  }

  uint8_t status = data[0] & 0xF0;
  uint8_t data_1 = (size >= 2) ? (data[1] & 0x7F) : 0;
  uint8_t data_2 = (size >= 3) ? (data[2] & 0x7F) : 0;

  switch (status) {
  case 0x80:
    if (size >= 3) { m_impl->synth.note_off(data_1, data_2); }
    break;
  case 0x90:
    if (size >= 3) {
      if (data_2 == 0) {
        m_impl->synth.note_off(data_1, 64);
      } else {
        m_impl->synth.note_on(data_1, data_2);
      }
    }
    break;
  case 0xA0:
    if (size >= 3) { m_impl->synth.after_touch_poly(data_1, data_2); }
    break;
  case 0xB0:
    if (size >= 3) { m_impl->synth.control_change(data_1, data_2); }
    break;
  case 0xC0:
    if (size >= 2) { m_impl->synth.program_change(data_1); }
    break;
  case 0xD0:
    if (size >= 2) { m_impl->synth.after_touch_channel(data_1); }
    break;
  case 0xE0:
    if (size >= 3) { m_impl->synth.pitch_bend(data_1, data_2); }
    break;
  default:
    break;
  }
}

void PRA32U2Engine::controlChange(uint8_t controlNumber, uint8_t value) {
  m_impl->synth.control_change(controlNumber & 0x7F, value & 0x7F);
}

uint8_t PRA32U2Engine::getControllerValue(uint8_t controlNumber) const {
  return m_impl->synth.current_controller_value(controlNumber & 0x7F);
}

void PRA32U2Engine::process(float& left, float& right) {
  PRA32_U2_StereoSample output = m_impl->synth.process(0, 0);
  left  = static_cast<float>(output.left)  * OUTPUT_SCALE;
  right = static_cast<float>(output.right) * OUTPUT_SCALE;
}
