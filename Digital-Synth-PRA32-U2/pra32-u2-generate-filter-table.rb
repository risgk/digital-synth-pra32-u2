require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-filter-table.h", "wb")

$file.printf("#pragma once\n\n")

# Coefficients of the ZDF/TPT State Variable Filter
#
# Only one entry per controller value is generated; the filter interpolates
# between the entries at run time
#
# The tables end with a guard entry that repeats the entry for the controller
# value 127, so that the interpolation needs no run-time check at the end of
# the table, and anything above the controller value 127 is treated as 127

# Filter Cutoff controller value -> cutoff frequency (12.98 Hz .. 19912 Hz)
def cutoff_freq(controller_value)
  (A4_FREQ / 32.0) * (2.0 ** ((controller_value - 1.0) / 12.0))
end

# g = tan(pi * f_0 / f_s), the TPT integrator gain (prewarped cutoff)
def cutoff_g(controller_value)
  f_0 = [cutoff_freq(controller_value), SAMPLING_RATE * 0.49].min
  Math.tan(Math::PI * f_0 / SAMPLING_RATE)
end

# Filter Resonance controller value -> Q (0.707 .. 5.66 at 96, doubling every
# 32 controller values, then rising faster up to 256 at 122); above 96 a
# quadratic term is added to log2(Q), so that the slope grows smoothly from
# 1/32 octave per controller value without a sudden step; above 122 the filter
# self-oscillates, and k = 1 / Q is 0
RESONANCE_CURVE_START = 96.0
RESONANCE_CURVE_C = (8.0 - (122.0 - 16.0) / 32.0) / ((122.0 - RESONANCE_CURVE_START) ** 2)

def resonance_k(controller_value)
  if controller_value <= RESONANCE_CURVE_START
    1.0 / (2.0 ** ((controller_value - 16.0) / 32.0))
  elsif controller_value <= 122
    1.0 / (2.0 ** ((controller_value - 16.0) / 32.0 + RESONANCE_CURVE_C * ((controller_value - RESONANCE_CURVE_START) ** 2)))
  else
    0.0
  end
end

# The self-oscillation takes k below 0 by t * kappa * (1 + g^2)^2 / g, where
# t = (controller value - 122) / 5, clamped to 0 .. 1; the level is about
# 11.4 * sqrt(kappa), and (1 + g^2)^2 / g holds it across the cutoff. kappa is
# scaled by 48000 / f_s, like the soft clipping of the band pass state
#
# A loud input weakens or stops the self-oscillation. The input also passes
# through the soft clipping of the band pass state, whose gain 1 - c^2 / 48
# damps the oscillation as well: with the oscillation A cos(w t) and an input
# component B cos(v t) in the state, the cubic gives the oscillation the
# damping (3 / 4) * A^3 + (3 / 2) * A * B^2, so the level falls to about
# sqrt(A_0^2 - 2 * B^2), A_0 being the level without input, and the
# oscillation stops once B reaches about 0.7 * A_0 (asynchronous quenching,
# as in analog filters). B is the input near the cutoff, which the band pass
# state passes: the Square (with harmonics twice those of the Saw) and the
# Multi Saw (with many partials near the cutoff) stop it first, depending on
# the note and the cutoff, while the Sub Osc, far below the cutoff, hardly
# does. An input harmonic near the oscillation pulls it to the pitch of that
# harmonic instead. As kappa alone sets both the level and the A_0 that the
# input has to reach, a quieter self-oscillation is also stopped more easily
#
# 0.5 balances how much the self-oscillation stands out over the Osc while
# playing against how readily the input stops it (0.4 before v3.7.1, when the
# Osc output was 1 / 1.25 of the current one: the ratio of the two is kept);
# of the Presets, Sync Lead, WT Pad, Fifth Lead, and PWM Lead stop it while
# playing. Below the knee of soft_clip_output() (0.75), so that it leaves the
# Filter as a clean sine
SELF_OSC_LEVEL = 0.5
SELF_OSC_KAPPA = ((SELF_OSC_LEVEL / 11.4) ** 2) * (48000.0 / SAMPLING_RATE)

# The soft clipping of the band pass state gives the self-oscillation a 3rd
# harmonic, which folds back to f_s - 3 * f_0 once 3 * f_0 is above f_s / 2
# (e.g. an inharmonic 11.8 kHz tone for f_0 = 19.9 kHz at 48 kHz). The
# self-oscillation fades out from f_s / 6 to f_s / 4.8, linearly in the cutoff
# controller value, where the folded tone is still above 0.375 * f_s (18 kHz
# at 48 kHz); above f_s / 4.8, the filter acts as at the Resonance 122
#
# An alternative, not adopted for its cost, removes the 3rd harmonic at its
# source: in the self-oscillation range, the soft clipping of the band pass
# state c is weighted by the low pass state o (both clamped to -4 .. +4),
#   gain = 1 - e / 48,  e = c^2 * (1 - t / 4) + o^2 * (3 * t / 4)
# (e = c^2 at t = 0). Self-oscillating, the two states run at nearly equal
# amplitude a quarter cycle apart, so e holds nearly still over a cycle and
# the oscillation stays nearly a pure sine.
#   - Effect (measured at 48 kHz with the level 0.5): the folded tone falls by
#     30-50 dB up to f_s / 3 (e.g. -43 dBFS to -85 dBFS at 11.8 kHz for the
#     Cutoff 118), but less so near f_s / 2, where the states drift from the
#     quadrature (-59 dBFS for the Cutoff 127). The fade-out would then only
#     be needed from f_s / 3 to f_s / 2.5, keeping the self-oscillation (and
#     its intermodulation with the input) up to 16 kHz
#   - Cost: about 10 more instructions per sample and voice on the Cortex-M33
#     (2 SMULL and 1 SSAT), about 0.7% of a core with 2 voices, against about
#     0.3% for this fade-out
#   - It also removes the 3rd harmonic of the self-oscillation at a low
#     cutoff, which changes the sound of the existing programs
SELF_OSC_FADE_START_FREQ = SAMPLING_RATE / 6.0
SELF_OSC_FADE_END_FREQ   = SAMPLING_RATE / 4.8

def self_osc_weight(controller_value)
  f_0 = [cutoff_freq(controller_value), SAMPLING_RATE * 0.49].min
  w = Math.log2(SELF_OSC_FADE_END_FREQ / f_0) / Math.log2(SELF_OSC_FADE_END_FREQ / SELF_OSC_FADE_START_FREQ)
  w.clamp(0.0, 1.0)
end

def generate_table(name, comment, fraction_bits)
  $file.printf("// %s\n", comment)
  # Not const, so that the small tables are placed in the SRAM, not in the flash
  $file.printf("int32_t %s[FILTER_TABLE_LENGTH] = {\n  ", name)
  (0..(FILTER_TABLE_LENGTH - 1)).each do |index|
    controller_value = [index, FILTER_TABLE_LENGTH - 2].min  # The guard entry repeats the last one
    value = yield(controller_value)
    raise "#{name}[#{index}] out of range" if value.abs >= (2 ** (31 - fraction_bits))
    $file.printf("%+11d,", (value * (1 << fraction_bits)).round)
    if index == (FILTER_TABLE_LENGTH - 1)
      $file.printf("\n")
    elsif index % 8 == (8 - 1)
      $file.printf("\n  ")
    else
      $file.printf(" ")
    end
  end
  $file.printf("};\n\n")
end

generate_table("g_filter_g_table", "g = tan(pi * f_0 / f_s)", FILTER_G_FRACTION_BITS) do |controller_value|
  g = cutoff_g(controller_value)
  printf("cutoff: %3d, f_0: %9.3f, g: %9.6f\n", controller_value, [cutoff_freq(controller_value), SAMPLING_RATE * 0.49].min, g)
  g
end

# The filter multiplies this by t * 5, so that t needs no division
generate_table("g_filter_self_osc_table", "kappa * (1 + g^2)^2 / g / 5", FILTER_TABLE_FRACTION_BITS) do |controller_value|
  g = cutoff_g(controller_value)
  h = SELF_OSC_KAPPA * ((1.0 + g * g) ** 2) / g / 5.0
  printf("cutoff: %3d, kappa * (1 + g^2)^2 / g / 5: %9.6f\n", controller_value, h)
  h
end

# The filter scales the Resonance above 122 by this
generate_table("g_filter_self_osc_weight_table", "Weight of the self-oscillation (1 .. 0)", FILTER_TABLE_FRACTION_BITS) do |controller_value|
  w = self_osc_weight(controller_value)
  printf("cutoff: %3d, self-oscillation weight: %9.6f\n", controller_value, w)
  w
end

# k = 1 / Q, the damping of the ZDF/TPT State Variable Filter
generate_table("g_filter_k_table", "k = 1 / Q", FILTER_TABLE_FRACTION_BITS) do |controller_value|
  k = resonance_k(controller_value)
  printf("reso  : %3d, q  : %9.3f, k: %9.6f\n", controller_value, (k > 0.0) ? 1.0 / k : Float::INFINITY, k)
  k
end

$file.close
