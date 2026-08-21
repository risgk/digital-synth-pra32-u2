require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-eg-table.h", "wb")

$file.printf("#pragma once\n\n")

$file.printf("int32_t g_eg_attack_coef_table[] = {\n  ")
(0..129).each do |i|
  time = i > 128 ? 128 : i
  attack_time_sec = 0.001 * (10.0 ** (time / 32.0))
  eg_coef = (0.5 ** (1.0 / ((SAMPLING_RATE / 4.0) * attack_time_sec))) * 0x40000000
  eg_coef = eg_coef.round

  $file.printf("%10d,", eg_coef)
  if i == 129
    $file.printf("\n")
  elsif i % 8 == 7
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.printf("int32_t g_eg_decay_release_coef_table[] = {\n  ")
(0..129).each do |i|
  time = i > 128 ? 128 : i
  decay_release_time_sec = 0.003 * (10.0 ** (time / 32.0))
  half_life_time_sec = decay_release_time_sec / 10.0
  eg_coef = (0.5 ** (1.0 / ((SAMPLING_RATE / 4.0) * half_life_time_sec))) * 0x40000000
  eg_coef = eg_coef.round

  $file.printf("%10d,", eg_coef)
  if i == 129
    $file.printf("\n")
  elsif i % 8 == 7
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.close
