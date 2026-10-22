require_relative 'pra32-u2-constants'

$file = File.open("pra32-u2-output-compressor-table.h", "wb")

$file.printf("#pragma once\n\n")

# Gain curve of the Output Compressor (PRA32_U2_OutputCompressor)
#
# The table is indexed by the envelope (1.0 = 1 << 23) in a pseudo-logarithmic
# scale: OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS bits below the leading 1 of the
# envelope, for each octave from 2^15 (-48 dB) to 2^25 (+12 dB); the compressor
# interpolates between the entries at run time. The table ends with a guard
# entry for 2^25, so that the interpolation needs no run-time check at the end
#
# Each entry is the whole gain in Q16: the compression with a soft knee, the
# ceiling (infinity:1 above the input 1.0, as the Output Limiter), and the
# output gain (x 0.75, -2.5 dB), so that no extra multiplication is needed
OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS = 4
OUTPUT_COMPRESSOR_TABLE_OCTAVES      = 10
OUTPUT_COMPRESSOR_TABLE_MIN_BITS     = 15
OUTPUT_COMPRESSOR_TABLE_LENGTH       = (OUTPUT_COMPRESSOR_TABLE_OCTAVES << OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS) + 1

# Mild settings, to keep the Presets sounding about the same: the peaks above
# -9 dB (the bottom of the knee) are compressed by up to 2:1, and the output
# never goes above the ceiling
THRESHOLD_DB   = -6.0
RATIO          = 2.0
KNEE_DB        = 6.0
CEILING_DB     = 0.0
MAKEUP_DB      = 0.0
OUTPUT_GAIN    = 0.75

ONE = (1 << 23).to_f

def envelope(index)
  octave  = index >> OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS
  segment = index & ((1 << OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS) - 1)
  (2.0 ** (OUTPUT_COMPRESSOR_TABLE_MIN_BITS + octave)) * (1.0 + segment.to_f / (1 << OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS))
end

# Output level (dB) of the compression, from the input level (dB)
def compressed_db(input_db)
  over_db = input_db - THRESHOLD_DB
  if 2.0 * over_db < -KNEE_DB
    input_db
  elsif 2.0 * over_db.abs <= KNEE_DB
    input_db + (1.0 / RATIO - 1.0) * ((over_db + KNEE_DB / 2.0) ** 2) / (2.0 * KNEE_DB)
  else
    THRESHOLD_DB + over_db / RATIO
  end
end

def gain(index)
  input_db  = 20.0 * Math.log10(envelope(index) / ONE)
  output_db = [compressed_db(input_db) + MAKEUP_DB, CEILING_DB].min
  (10.0 ** ((output_db - input_db) / 20.0)) * OUTPUT_GAIN
end

$file.printf("const uint8_t  OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS = %d;\n", OUTPUT_COMPRESSOR_TABLE_SEGMENT_BITS)
$file.printf("const uint8_t  OUTPUT_COMPRESSOR_TABLE_MIN_BITS     = %d;\n", OUTPUT_COMPRESSOR_TABLE_MIN_BITS)
$file.printf("const uint8_t  OUTPUT_COMPRESSOR_TABLE_OCTAVES      = %d;\n", OUTPUT_COMPRESSOR_TABLE_OCTAVES)
$file.printf("const uint16_t OUTPUT_COMPRESSOR_TABLE_LENGTH       = %d;\n\n", OUTPUT_COMPRESSOR_TABLE_LENGTH)

$file.printf("// The output gain in Q16, also applied to the Dry input (not compressed)\n")
$file.printf("const int32_t  OUTPUT_COMPRESSOR_OUTPUT_GAIN        = %d;\n\n", (OUTPUT_GAIN * (1 << 16)).round)

$file.printf("// Gain in Q16 (threshold %.1f dB, ratio %.1f:1, knee %.1f dB, ceiling %.1f dB, makeup %.1f dB, x %.2f)\n",
             THRESHOLD_DB, RATIO, KNEE_DB, CEILING_DB, MAKEUP_DB, OUTPUT_GAIN)
$file.printf("// Not const, so that the table is in the RAM (no XIP cache misses)\n")
$file.printf("int32_t g_output_compressor_gain_table[OUTPUT_COMPRESSOR_TABLE_LENGTH] = {\n  ")
(0..(OUTPUT_COMPRESSOR_TABLE_LENGTH - 1)).each do |index|
  g = gain(index)
  printf("index: %3d, envelope: %+7.2f dB, gain: %+7.2f dB\n", index, 20.0 * Math.log10(envelope(index) / ONE), 20.0 * Math.log10(g))
  $file.printf("%+7d,", (g * (1 << 16)).round)
  if index == OUTPUT_COMPRESSOR_TABLE_LENGTH - 1
    $file.printf("\n")
  elsif index % 16 == 15
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n")

$file.close
