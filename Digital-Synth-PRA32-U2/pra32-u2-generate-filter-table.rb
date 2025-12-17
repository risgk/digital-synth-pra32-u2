require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-filter-table.h", "w")

$file.printf("#pragma once\n\n")

OCTAVES = 10

def generate_filter_lpf_table(res_id, name, q)
  $file.printf("const int32_t g_filter_lpf_table_%s[] = {\n  ", name)
  (0..((DATA_BYTE_MAX * 2) << FILTER_TABLE_CUTOFF_EXT_BITS) + 1).each do |i|
    f_idx = [[-2 << FILTER_TABLE_CUTOFF_EXT_BITS, i - ((1 * 2) << FILTER_TABLE_CUTOFF_EXT_BITS)].max,
              252 << FILTER_TABLE_CUTOFF_EXT_BITS].min
    f_idx = 120 if f_idx < 120
    f_0 = (2.0 ** ((f_idx / 2.0 / (1 << FILTER_TABLE_CUTOFF_EXT_BITS)) / (120.0 / OCTAVES))) * ((A4_FREQ * 2.0) * 16.0) * 2.0 / (2.0 ** (OCTAVES.to_f + 1.0))
    f_0_over_f_s = f_0 / SAMPLING_RATE

    w_0 = 2.0 * Math::PI * f_0_over_f_s
    alpha = Math.sin(w_0) / (2.0 * q)

    lpf_b_2 = (1.0 - Math.cos(w_0)) / 2.0
    hpf_b_2 = (1.0 + Math.cos(w_0)) / 2.0
    a_0 = 1.0 + alpha
    a_1 = (-2.0) * Math.cos(w_0)
    a_2 = 1.0 - alpha

    input_gain = 1.0
    input_gain = Math.sqrt(8.0) / q if q > Math.sqrt(8.0)

    b_2_over_a_0_gain = (input_gain * (lpf_b_2 / a_0) * (1 << FILTER_TABLE_FRACTION_BITS)).floor.to_i
    a_1_over_a_0 = ((a_1 / a_0) * (1 << FILTER_TABLE_FRACTION_BITS)).floor.to_i
    a_2_over_a_0 = ((a_2 / a_0) * (1 << FILTER_TABLE_FRACTION_BITS)).floor.to_i

    printf("i: %d, f_idx: %d, f_0_over_f_s: %f, f_0: %f, res_id: %d, q: %f, g: %f, q_mul_g: %f\n", i, f_idx, f_0_over_f_s, f_0, res_id, q, input_gain, q * input_gain)

    $file.printf("%+11d, %+11d, %+11d,", b_2_over_a_0_gain, a_1_over_a_0, a_2_over_a_0)
    if i == (((DATA_BYTE_MAX * 2) << FILTER_TABLE_CUTOFF_EXT_BITS) + 1)
      $file.printf("\n")
    elsif i % 2 == (2 - 1)
      $file.printf("\n  ")
    else
      $file.printf("  ")
    end
  end
  $file.printf("};\n\n")
end

MAX_RES_ID = (14 * (1 << FILTER_TABLE_RESO_EXT_BITS))

(0..MAX_RES_ID).each do |res_id|
  generate_filter_lpf_table(res_id, res_id.to_s, Math.sqrt(2.0) ** ((res_id - (2.0 * (1 << FILTER_TABLE_RESO_EXT_BITS))) / (2.0 * (1 << FILTER_TABLE_RESO_EXT_BITS))))
end

$file.printf("const int32_t* g_filter_tables[] = {\n  ")
(0..16 * (1 << FILTER_TABLE_RESO_EXT_BITS)).each do |res_index|
  res_id = [[res_index - (2 * (1 << FILTER_TABLE_RESO_EXT_BITS)), 0].max, MAX_RES_ID].min
  $file.printf("g_filter_lpf_table_%-2d,", res_id)
  if res_index == DATA_BYTE_MAX
    $file.printf("\n")
  elsif res_index % 4 == 3
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.close
