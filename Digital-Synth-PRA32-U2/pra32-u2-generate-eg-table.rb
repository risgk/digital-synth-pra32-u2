require_relative 'pra32-u2-generate-common'

# Coefficients of the EG
#
# Only one entry per controller value is generated; the EG interpolates
# between the entries at run time
#
# The tables end with a guard entry that repeats the entry for the controller
# value 127, so that the interpolation needs no run-time check at the end of
# the table, and anything above the controller value 127 is treated as 127

def write_eg_table(file, name)
  coefs = controller_values_with_guard(EG_TABLE_LENGTH).map do |controller_value|
    half_life_time_sec = yield(controller_value)
    ((0.5 ** (1.0 / ((SAMPLING_RATE / 4.0) * half_life_time_sec))) * 0x40000000).round
  end
  # Not const, so that the small tables are placed in the SRAM, not in the flash
  write_array(file, "int32_t #{name}[EG_TABLE_LENGTH]", coefs, per_line: 8, format: "%10d")
end

write_header("pra32-u2-eg-table.h") do |file|
  # EG Attack controller value -> attack time (1.00 ms .. 9.31 s)
  write_eg_table(file, "g_eg_attack_coef_table") do |controller_value|
    0.001 * (10.0 ** (controller_value / 32.0))
  end

  # EG Decay/Release controller value -> time to 1/1024, approx. -60 dB (3.00 ms .. 27.9 s)
  write_eg_table(file, "g_eg_decay_release_coef_table") do |controller_value|
    decay_release_time_sec = 0.003 * (10.0 ** (controller_value / 32.0))
    decay_release_time_sec / 10.0
  end
end
