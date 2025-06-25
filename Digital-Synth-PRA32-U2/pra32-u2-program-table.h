#pragma once

const uint8_t PROGRAM_NUMBER_DEFAULT = 8;

// Preset                                         #0   #1   #2   #3   #4   #5   #6   #7     #8   #9   #10  #11  #12  #13  #14  #15  
const uint8_t g_preset_table_OSC_1_WAVE     [] = {127, 127, 25 , 0  , 127, 76 , 127, 0  ,   127, 127, 25 , 0  , 127, 76 , 127, 0  };
const uint8_t g_preset_table_OSC_1_SHAPE    [] = {0  , 0  , 32 , 64 , 0  , 0  , 0  , 0  ,   0  , 0  , 32 , 64 , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_OSC_1_MORPH    [] = {64 , 64 , 127, 127, 64 , 108, 64 , 0  ,   64 , 64 , 127, 127, 64 , 108, 64 , 0  };
const uint8_t g_preset_table_MIXER_SUB_OSC  [] = {127, 64 , 127, 64 , 127, 64 , 64 , 64 ,   127, 64 , 127, 64 , 127, 64 , 64 , 64 };

const uint8_t g_preset_table_OSC_2_WAVE     [] = {0  , 0  , 25 , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 25 , 0  , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_OSC_2_COARSE   [] = {64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ,   64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 };
const uint8_t g_preset_table_OSC_2_PITCH    [] = {109, 72 , 72 , 72 , 72 , 72 , 72 , 64 ,   109, 72 , 72 , 72 , 72 , 72 , 72 , 64 };
const uint8_t g_preset_table_MIXER_OSC_MIX  [] = {64 , 64 , 0  , 0  , 64 , 64 , 0  , 0  ,   64 , 64 , 0  , 0  , 64 , 64 , 0  , 0  };

const uint8_t g_preset_table_FILTER_CUTOFF  [] = {115, 103, 115, 103, 55 , 79 , 127, 127,   115, 103, 115, 103, 55 , 79 , 127, 127};
const uint8_t g_preset_table_FILTER_RESO    [] = {64 , 32 , 32 , 64 , 64 , 32 , 0  , 0  ,   64 , 32 , 32 , 64 , 64 , 32 , 0  , 0  };
const uint8_t g_preset_table_FILTER_EG_AMT  [] = {64 , 16 , 64 , 64 , 112, 64 , 64 , 64 ,   64 , 16 , 64 , 64 , 112, 64 , 64 , 64 };
const uint8_t g_preset_table_FILTER_KEY_TRK [] = {64 , 64 , 64 , 64 , 64 , 64 , 0  , 0  ,   64 , 64 , 64 , 64 , 64 , 64 , 0  , 0  };

const uint8_t g_preset_table_EG_ATTACK      [] = {32 , 96 , 32 , 32 , 32 , 32 , 32 , 0  ,   32 , 96 , 32 , 32 , 32 , 32 , 32 , 0  };
const uint8_t g_preset_table_EG_DECAY       [] = {32 , 96 , 96 , 64 , 96 , 100, 32 , 0  ,   32 , 96 , 96 , 64 , 96 , 100, 32 , 0  };
const uint8_t g_preset_table_EG_SUSTAIN     [] = {127, 0  , 0  , 127, 0  , 0  , 127, 127,   127, 0  , 0  , 127, 0  , 0  , 127, 127};
const uint8_t g_preset_table_EG_RELEASE     [] = {32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ,   32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  };

const uint8_t g_preset_table_EG_OSC_AMT     [] = {64 , 64 , 80 , 64 , 64 , 72 , 64 , 64 ,   64 , 64 , 80 , 64 , 64 , 72 , 64 , 64 };
const uint8_t g_preset_table_EG_OSC_DST     [] = {0  , 0  , 127, 0  , 0  , 127, 0  , 0  ,   0  , 0  , 127, 0  , 0  , 127, 0  , 0  };
const uint8_t g_preset_table_VOICE_MODE     [] = {127, 0  , 76 , 0  , 76 , 0  , 0  , 0  ,   127, 0  , 76 , 0  , 76 , 0  , 0  , 0  };
const uint8_t g_preset_table_PORTAMENTO     [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };

const uint8_t g_preset_table_LFO_WAVE       [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_LFO_RATE       [] = {80 , 80 , 80 , 80 , 80 , 80 , 80 , 80 ,   80 , 80 , 80 , 80 , 80 , 80 , 80 , 80 };
const uint8_t g_preset_table_LFO_DEPTH      [] = {0  , 0  , 0  , 0  , 0  , 0  , 127, 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 127, 0  };
const uint8_t g_preset_table_LFO_FADE_TIME  [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };

const uint8_t g_preset_table_LFO_OSC_AMT    [] = {96 , 64 , 72 , 64 , 64 , 64 , 64 , 64 ,   96 , 64 , 72 , 64 , 64 , 64 , 64 , 64 };
const uint8_t g_preset_table_LFO_OSC_DST    [] = {0  , 0  , 127, 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 127, 0  , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_LFO_FILTER_AMT [] = {64 , 88 , 64 , 88 , 88 , 88 , 64 , 64 ,   64 , 88 , 64 , 88 , 88 , 88 , 64 , 64 };
const uint8_t g_preset_table_AMP_GAIN       [] = {90 , 90 , 90 , 90 , 90 , 127, 90 , 90 ,   90 , 90 , 90 , 90 , 90 , 127, 90 , 90 };

const uint8_t g_preset_table_AMP_ATTACK     [] = {32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ,   32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  };
const uint8_t g_preset_table_AMP_DECAY      [] = {32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ,   32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  };
const uint8_t g_preset_table_AMP_SUSTAIN    [] = {127, 127, 127, 127, 127, 127, 127, 127,   127, 127, 127, 127, 127, 127, 127, 127};
const uint8_t g_preset_table_AMP_RELEASE    [] = {32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  ,   32 , 32 , 32 , 32 , 32 , 32 , 32 , 0  };

const uint8_t g_preset_table_FILTER_MODE    [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_EG_AMP_MOD     [] = {127, 0  , 0  , 127, 127, 127, 127, 0  ,   127, 0  , 0  , 127, 127, 127, 127, 0  };
const uint8_t g_preset_table_REL_EQ_DECAY   [] = {127, 127, 0  , 127, 127, 0  , 127, 0  ,   127, 127, 0  , 127, 127, 0  , 127, 0  };
const uint8_t g_preset_table_P_BEND_RANGE   [] = {12 , 12 , 12 , 12 , 12 , 12 , 12 , 12 ,   12 , 12 , 12 , 12 , 12 , 12 , 12 , 12 };

const uint8_t g_preset_table_BTH_FILTER_AMT [] = {64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ,   64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 };
const uint8_t g_preset_table_BTH_AMP_MOD    [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_EG_VEL_SENS    [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };
const uint8_t g_preset_table_AMP_VEL_SENS   [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };

const uint8_t g_preset_table_VOICE_ASGN_MODE[] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };




const uint8_t g_preset_table_CHORUS_MIX     [] = {127, 127, 127, 127, 127, 127, 127, 0  ,   127, 127, 127, 127, 127, 127, 127, 0  };
const uint8_t g_preset_table_CHORUS_RATE    [] = {64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ,   64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 };
const uint8_t g_preset_table_CHORUS_DEPTH   [] = {64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 ,   64 , 64 , 64 , 64 , 64 , 64 , 64 , 64 };


const uint8_t g_preset_table_DELAY_FEEDBACK [] = {64 , 64 , 64 , 64 , 64 , 64 , 64 , 0  ,   64 , 64 , 64 , 64 , 64 , 64 , 64 , 0  };
const uint8_t g_preset_table_DELAY_TIME     [] = {93 , 93 , 93 , 93 , 93 , 93 , 93 , 93 ,   93 , 93 , 93 , 93 , 93 , 93 , 93 , 93 };
const uint8_t g_preset_table_DELAY_MODE     [] = {0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  ,   0  , 0  , 0  , 0  , 0  , 0  , 0  , 0  };

