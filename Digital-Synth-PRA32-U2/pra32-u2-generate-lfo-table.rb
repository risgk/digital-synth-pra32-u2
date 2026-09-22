require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-lfo-table.h", "wb")

$file.printf("#pragma once\n\n")

$file.printf("uint32_t g_lfo_rate_table[] = {\n  ")
(0..127).each do |i|
  lfo_rate = ((2.0 ** ((i - 64) / 12.0)) *
              (A4_FREQ * (2.0 ** ((-19 - 69) / 12.0))) * (1 << 24) / (SAMPLING_RATE / 4.0)).floor
  lfo_rate = 0 if i == 0

  $file.printf("%6d,", lfo_rate)
  if i == DATA_BYTE_MAX
    $file.printf("\n")
  elsif i % 16 == (16 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

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

$file.printf("int32_t g_lfo_red_noise_coef_table[] = {\n  ")
(0..127).each do |i|
  $file.printf("0x%08X,", lfo_red_noise_coef(i))
  if i == DATA_BYTE_MAX
    $file.printf("\n")
  elsif i % 8 == (8 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.printf("uint16_t g_lfo_red_noise_gain_table[] = {\n  ")
(0..127).each do |i|
  $file.printf("%6d,", lfo_red_noise_gain(i))
  if i == DATA_BYTE_MAX
    $file.printf("\n")
  elsif i % 16 == (16 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.printf("uint16_t g_lfo_fade_coef_table[] = {\n  ")
(0..127).each do |i|
  fade_coef = (10.0 * (10.0 ** ((i - 128.0) / 64.0)) * (SAMPLING_RATE / 4.0) / 128.0).floor
  fade_coef = 1 if i == 0

  $file.printf("%6d,", fade_coef)
  if i == DATA_BYTE_MAX
    $file.printf("\n")
  elsif i % 16 == (16 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.printf("uint32_t g_chorus_rate_table[] = {\n  ")
(0..127).each do |i|
  lfo_rate = ((2.0 ** ((i - 64) / 12.0)) *
              (A4_FREQ * (2.0 ** ((-49 - 69) / 12.0))) * (1 << 24) / (SAMPLING_RATE / 4.0)).floor

  $file.printf("%6d,", lfo_rate)
  if i == DATA_BYTE_MAX
    $file.printf("\n")
  elsif i % 16 == (16 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.close
