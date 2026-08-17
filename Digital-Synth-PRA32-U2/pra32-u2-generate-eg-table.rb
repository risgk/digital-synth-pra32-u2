require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-eg-table.h", "wb")

$file.printf("#pragma once\n\n")

$file.printf("int32_t g_eg_attack_coef_table[] = {\n  ")
(0..128 * (2 ** EG_TABLE_EXT_BITS) + 16 * (2 ** EG_TABLE_EXT_BITS)).each do |i|
  time = i
  attack_time_sec = 0.001 * (10.0 ** (time / (31.75 * (2 ** EG_TABLE_EXT_BITS))))
  eg_coef = (0.5 ** (1.0 / ((SAMPLING_RATE / 4.0) * attack_time_sec))) * 0x40000000
  eg_coef = eg_coef.round

  $file.printf("%10d,", eg_coef)
  if i == 128 * (2 ** EG_TABLE_EXT_BITS) + 16 * (2 ** EG_TABLE_EXT_BITS)
    $file.printf("\n")
  elsif i % (8 * (2 ** EG_TABLE_EXT_BITS)) == ((8 * (2 ** EG_TABLE_EXT_BITS)) - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.printf("int32_t g_eg_decay_release_coef_table[] = {\n  ")
(0..128 * (2 ** EG_TABLE_EXT_BITS) + 16 * (2 ** EG_TABLE_EXT_BITS)).each do |i|
  time = i
  eg_coef = (0.5 ** (1.0 / ((0.2 / 10.0) * (SAMPLING_RATE / 4) *
             (10.0 ** ((time - (64.0 * (2 ** EG_TABLE_EXT_BITS))) /
              (32.0 * (2 ** EG_TABLE_EXT_BITS)))))) * 0x40000000).round

  $file.printf("%10d,", eg_coef)
  if i == 128 * (2 ** EG_TABLE_EXT_BITS) + 16 * (2 ** EG_TABLE_EXT_BITS)
    $file.printf("\n")
  elsif i % (8 * (2 ** EG_TABLE_EXT_BITS)) == ((8 * (2 ** EG_TABLE_EXT_BITS)) - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.close
