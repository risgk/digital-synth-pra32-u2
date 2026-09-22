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

# Filter Resonance controller value -> Q (0.707 .. 11.07)
def resonance_q(controller_value)
  2.0 ** ((controller_value - 16.0) / 32.0)
end

def generate_table(name, comment, fraction_bits)
  $file.printf("// %s\n", comment)
  # Not const, so that the small tables are placed in the SRAM, not in the flash
  $file.printf("int32_t %s[FILTER_TABLE_LENGTH] = {\n  ", name)
  (0..(FILTER_TABLE_LENGTH - 1)).each do |index|
    controller_value = [index, FILTER_TABLE_LENGTH - 2].min  # The guard entry repeats the last one
    value = yield(controller_value)
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

# g = tan(pi * f_0 / f_s), the TPT integrator gain (prewarped cutoff)
generate_table("g_filter_g_table", "g = tan(pi * f_0 / f_s)", FILTER_G_FRACTION_BITS) do |controller_value|
  f_0 = [cutoff_freq(controller_value), SAMPLING_RATE * 0.49].min
  g = Math.tan(Math::PI * f_0 / SAMPLING_RATE)
  printf("cutoff: %3d, f_0: %9.3f, g: %9.6f\n", controller_value, f_0, g)
  g
end

# k = 1 / Q, the damping of the ZDF/TPT State Variable Filter
generate_table("g_filter_k_table", "k = 1 / Q", FILTER_TABLE_FRACTION_BITS) do |controller_value|
  q = resonance_q(controller_value)
  k = 1.0 / q
  printf("reso  : %3d, q  : %9.3f, k: %9.6f\n", controller_value, q, k)
  k
end

$file.close
