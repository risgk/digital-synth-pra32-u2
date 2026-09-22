require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-eg-table.h", "wb")

$file.printf("#pragma once\n\n")

# Coefficients of the EG
#
# Only one entry per controller value is generated; the EG interpolates
# between the entries at run time
#
# The tables end with a guard entry that repeats the entry for the controller
# value 127, so that the interpolation needs no run-time check at the end of
# the table, and anything above the controller value 127 is treated as 127

def generate_table(name)
  # Not const, so that the small tables are placed in the SRAM, not in the flash
  $file.printf("int32_t %s[EG_TABLE_LENGTH] = {\n  ", name)
  (0..(EG_TABLE_LENGTH - 1)).each do |index|
    controller_value = [index, EG_TABLE_LENGTH - 2].min  # The guard entry repeats the last one
    half_life_time_sec = yield(controller_value)
    eg_coef = (0.5 ** (1.0 / ((SAMPLING_RATE / 4.0) * half_life_time_sec))) * 0x40000000
    $file.printf("%10d,", eg_coef.round)
    if index == (EG_TABLE_LENGTH - 1)
      $file.printf("\n")
    elsif index % 8 == (8 - 1)
      $file.printf("\n  ")
    else
      $file.printf(" ")
    end
  end
  $file.printf("};\n\n")
end

# EG Attack controller value -> attack time (1.00 ms .. 9.31 s)
generate_table("g_eg_attack_coef_table") do |controller_value|
  attack_time_sec = 0.001 * (10.0 ** (controller_value / 32.0))
  attack_time_sec
end

# EG Decay/Release controller value -> time to 1/1024, approx. -60 dB (3.00 ms .. 27.9 s)
generate_table("g_eg_decay_release_coef_table") do |controller_value|
  decay_release_time_sec = 0.003 * (10.0 ** (controller_value / 32.0))
  decay_release_time_sec / 10.0
end

$file.close
