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

# Filter Resonance controller value -> Q (0.707 .. 8.0 at 112, then doubling
# every 2 controller values up to 256 at 122); above 122 the filter
# self-oscillates, and k = 1 / Q is 0
def resonance_k(controller_value)
  if controller_value <= 112
    1.0 / (2.0 ** ((controller_value - 16.0) / 32.0))
  elsif controller_value <= 122
    1.0 / (8.0 * (2.0 ** ((controller_value - 112.0) / 2.0)))
  else
    0.0
  end
end

# The self-oscillation takes k below 0 by t * kappa * (1 + g^2)^2 / g, where
# t = (controller value - 122) / 5, clamped to 0 .. 1; the level is about
# 11.4 * sqrt(kappa), and (1 + g^2)^2 / g holds it across the cutoff. kappa is
# scaled by 48000 / f_s, like the soft clipping of the band pass state
SELF_OSC_LEVEL = 0.2
SELF_OSC_KAPPA = ((SELF_OSC_LEVEL / 11.4) ** 2) * (48000.0 / SAMPLING_RATE)

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

# k = 1 / Q, the damping of the ZDF/TPT State Variable Filter
generate_table("g_filter_k_table", "k = 1 / Q", FILTER_TABLE_FRACTION_BITS) do |controller_value|
  k = resonance_k(controller_value)
  printf("reso  : %3d, q  : %9.3f, k: %9.6f\n", controller_value, (k > 0.0) ? 1.0 / k : Float::INFINITY, k)
  k
end

$file.close
