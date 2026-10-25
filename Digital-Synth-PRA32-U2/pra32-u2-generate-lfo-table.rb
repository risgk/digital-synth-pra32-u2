require_relative 'pra32-u2-generate-common'

CONTROLLER_VALUES = (0..DATA_BYTE_MAX)

# LFO Rate controller value -> the phase step per control period (SAMPLING_RATE / 4); 0 stops it
def lfo_rate(controller_value)
  return 0 if controller_value == 0

  ((2.0 ** ((controller_value - 64) / 12.0)) *
   (A4_FREQ * (2.0 ** ((-19 - 69) / 12.0))) * (1 << 24) / (SAMPLING_RATE / 4.0)).floor
end

# LFO Red Noise: 1st-order IIR low-pass on the white noise, run at SAMPLING_RATE / 4.
# The coefficient is a of "acc += (input - acc) * a" in Q31. CC value 1 matches the oscillator
# drift (a = 2 ** -17) and CC value 80 passes the same noise power as the 2-tap moving average
# it replaces (a / (2 - a) == 1 / 2); a saturates to 1, that is to no filtering, near the top.
# The gain restores the output RMS, which a 1-pole otherwise scales by sqrt(a / (2 - a)).
LFO_CONTROL_RATE       = SAMPLING_RATE / 4.0
LFO_RED_NOISE_A_AT_80  = 2.0 / 3.0
LFO_RED_NOISE_FC_MIN   = -Math.log(1.0 - (2.0 ** -17)) * LFO_CONTROL_RATE / (2.0 * Math::PI)
LFO_RED_NOISE_FC_AT_80 = -Math.log(1.0 - LFO_RED_NOISE_A_AT_80) * LFO_CONTROL_RATE / (2.0 * Math::PI)
LFO_RED_NOISE_FC_RATIO = (LFO_RED_NOISE_FC_AT_80 / LFO_RED_NOISE_FC_MIN) ** (1.0 / 79.0)
LFO_RED_NOISE_COEF_MAX = (1 << 31) - 1
LFO_RED_NOISE_IN_RMS   = Math.sqrt(((1 << 15) ** 2 - 1) / 12.0)
LFO_RED_NOISE_OUT_RMS  = (LFO_RED_NOISE_IN_RMS / Math.sqrt(8.0)) * 64.0

def lfo_red_noise_coef(i)
  return 0 if i == 0

  fc = LFO_RED_NOISE_FC_MIN * (LFO_RED_NOISE_FC_RATIO ** (i - 1))
  a = 1.0 - Math.exp(-2.0 * Math::PI * fc / LFO_CONTROL_RATE)

  [(a * (1 << 31)).round, LFO_RED_NOISE_COEF_MAX].min
end

def lfo_red_noise_gain(i)
  coef = lfo_red_noise_coef(i)
  return 0 if coef == 0

  a = coef / (2.0 ** 31)
  ((LFO_RED_NOISE_OUT_RMS / Math.sqrt(a / (2.0 - a))) / LFO_RED_NOISE_IN_RMS * 4.0).round
end

# LFO Fade Time controller value -> the step of the fade per control period
def lfo_fade_coef(controller_value)
  return 1 if controller_value == 0

  (10.0 * (10.0 ** ((controller_value - 128.0) / 64.0)) * (SAMPLING_RATE / 4.0) / 128.0).floor
end

# Chorus Rate controller value -> the phase step per control period
def chorus_rate(controller_value)
  ((2.0 ** ((controller_value - 64) / 12.0)) *
   (A4_FREQ * (2.0 ** ((-49 - 69) / 12.0))) * (1 << 24) / (SAMPLING_RATE / 4.0)).floor
end

write_header("pra32-u2-lfo-table.h") do |file|
  write_array(file, "uint32_t g_lfo_rate_table[]", CONTROLLER_VALUES.map { |i| lfo_rate(i) }, per_line: 16, format: "%6d")
  write_array(file, "int32_t g_lfo_red_noise_coef_table[]", CONTROLLER_VALUES.map { |i| lfo_red_noise_coef(i) }, per_line: 8, format: "0x%08X")
  write_array(file, "uint16_t g_lfo_red_noise_gain_table[]", CONTROLLER_VALUES.map { |i| lfo_red_noise_gain(i) }, per_line: 16, format: "%6d")
  write_array(file, "uint16_t g_lfo_fade_coef_table[]", CONTROLLER_VALUES.map { |i| lfo_fade_coef(i) }, per_line: 16, format: "%6d")
  write_array(file, "uint32_t g_chorus_rate_table[]", CONTROLLER_VALUES.map { |i| chorus_rate(i) }, per_line: 16, format: "%6d")
end
