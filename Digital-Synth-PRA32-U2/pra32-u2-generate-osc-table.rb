require_relative 'pra32-u2-constants'

# refs http://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
# Cooley–Tukey FFT algorithm - Wikipedia, the free encyclopedia

def fft(a)
  ditfft2(a, a.size, 1)
end

def ditfft2(x, n, s)
  result = []
  if n == 1 then
    result[0] = x[0]
  else
    result += ditfft2(x,         n / 2, 2 * s)
    result += ditfft2(x.drop(s), n / 2, 2 * s)
    for k in 0..(n / 2 - 1) do
      t = result[k]
      result[k]         = t + (Math::E ** Complex(0, -2 * Math::PI * k / n)) * result[k + n / 2]
      result[k + n / 2] = t - (Math::E ** Complex(0, -2 * Math::PI * k / n)) * result[k + n / 2]
    end
  end
  result
end

def ifft(ffta, amp)
  n = ffta.size
  fft(ffta.map {|i| i.conj }).map {|i| i.conj }.map {|i| i * amp / n }.map {|i| i.real }
end

def lpf_fft(ffta, k)
  n = ffta.size
  a = ffta.clone
  (k + 1 .. (n / 2)).each do |i|
    a[i] = 0.0
    a[n - i] = 0.0
  end
  return a
end

def bpf_fft(ffta, k)
  n = ffta.size
  a = ffta.clone
  (0 .. (n / 2)).each do |i|
    if i != k
      a[i] = 0.0
      a[n - i] = 0.0
    end
  end
  return a
end

$file = File.open("pra32-u2-osc-table.h", "wb")

$file.printf("#pragma once\n\n")

def freq_from_note_number(note_number, pr = false)
  cent = (note_number * 100.0) - 6900.0
  hz = A4_FREQ * (2.0 ** (cent / 1200.0))
  freq = (hz * (1 << OSC_PHASE_RESOLUTION_BITS) / SAMPLING_RATE).floor.to_i
  freq = freq + 1 if freq.even?
  if pr
    printf("[0]%3d, %+f, %d\n",note_number, 1.0 - freq.to_f * SAMPLING_RATE / (hz * (1 << OSC_PHASE_RESOLUTION_BITS)), freq)
  end
  return freq
end

$file.printf("uint32_t g_osc_freq_table[] = {\n  ")
# Loop extended to NOTE_NUMBER_MAX + 1 (0..128) for safe linear interpolation (idx + 1)
(NOTE_NUMBER_MIN..NOTE_NUMBER_MAX + 1).each do |note_number|
  # Clamp the index 128 to NOTE_NUMBER_MAX to duplicate the last frequency entry as a guard
  target_note = note_number > NOTE_NUMBER_MAX ? NOTE_NUMBER_MAX : note_number
  freq = freq_from_note_number(target_note, (note_number <= NOTE_NUMBER_MAX))

  $file.printf("0x%08X,", freq)
  if note_number == NOTE_NUMBER_MAX + 1
    $file.printf("\n")
  elsif note_number % 12 == (12 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

# Re-calculate max_tune_rate with hardcoded constants to preserve original harmonics restriction
max_tune_rate = -Float::INFINITY
(0..(1 << 8) - 1).each do |i|
  tune_rate = ((2.0 ** ((i - (1 << (8 - 1))) / (12.0 * (1 << 8)))) *
               (1 << 15) / 1.0).round -
              (1 << 15) / 1.0
  max_tune_rate = tune_rate if max_tune_rate < tune_rate
end

# The linear interpolation of the table leaves an image of the harmonic h at
# (length - h) times the fundamental, at (h / length) ** 2 of its amplitude.
# Summed over the harmonics that the mipmap holds, that comes to
#
#   Saw, Square     amplitude 1 / h    ->  images  ~  H ** 3 / length ** 4
#   Triangle        amplitude 1 / h**2 ->  images  ~  H      / length ** 4
#
# which matches the measured floors within 0.5 dB. Keeping the floor even
# across the mipmaps therefore wants the length to follow H ** 0.75 (H ** 0.25
# for the Triangle), not H itself, and every doubling of a length buys 12 dB.
# Sizing each table this way spends the memory where it actually buys quality:
# a mipmap that holds 1 harmonic does not need the same length as one that
# holds 255
SOFT_TABLE_WAVES = ["triangle"]
NOMINAL_TABLE_WAVES = ["sine"]

def table_samples_bits(name, last)
  # The LFO indexes the Sine table with OSC_WAVE_TABLE_SAMPLES_BITS directly
  return OSC_WAVE_TABLE_SAMPLES_BITS if NOMINAL_TABLE_WAVES.include?(name)

  soft = SOFT_TABLE_WAVES.include?(name)
  samples = soft ? OSC_WAVE_TABLE_SAMPLES_SCALE_SOFT * (last ** 0.25)
                 : OSC_WAVE_TABLE_SAMPLES_SCALE      * (last ** 0.75)
  bits = Math.log2(samples).ceil
  bits = [bits, OSC_WAVE_TABLE_SAMPLES_BITS_MIN].max
  bits = [bits, soft ? OSC_WAVE_TABLE_SAMPLES_BITS_MAX_SOFT : OSC_WAVE_TABLE_SAMPLES_BITS_MAX].min
  bits
end

def generate_osc_wave_table(name, last, amp)
  bits = table_samples_bits(name, last)
  samples = 1 << bits
  # The first entry is the number of index bits, which the run time reads
  # back through the pointer the table array holds (which points at [1])
  $file.printf("int16_t g_osc_#{name}_wave_table_h%d[] = {\n  %+6d,\n  ", last, bits)
  (0..samples).each do |n|
    level = 0
    nn = n
    nn = 0 if n == samples
    max = last
    (1..max).each do |k|
      level += yield(nn.to_f / samples, k, nn, samples)
    end
    level *= amp
    level = (level * OSC_WAVE_TABLE_AMP).round.to_i
    $file.printf("%+6d,", level)
    if n == samples
      $file.printf("\n")
    elsif n % 16 == 15
      $file.printf("\n  ")
    else
      $file.printf(" ")
    end
  end
  $file.printf("};\n\n")
end

$osc_harmonics_restriction_table = []

(NOTE_NUMBER_MIN..NOTE_NUMBER_MAX).each do |note_number|
  correction = (max_tune_rate.to_f + (1 << 15)) / (1 << 15)
  freq = freq_from_note_number(((note_number + (3 - 1)) / 3) * 3) * correction
  freq = freq.floor
  bit = 1
  freq += bit
  $osc_harmonics_restriction_table << freq
end

OSC_DETUNE_CORRECRION = 1060  # Approx. 101 cents

def last_harmonic(freq)
  correction = [freq * (OSC_DETUNE_CORRECRION - 1000) / 1000, OSC_DETUNE_FREQ_MAX].max
  last = (freq != 0) ? ((FREQUENCY_MAX * (1 << OSC_PHASE_RESOLUTION_BITS)) /
                        ((freq + correction) * SAMPLING_RATE)) : 0
  last = [last, OSC_WAVE_TABLE_LAST_HARMONIC].min
  last
end

def generate_osc_wave_table_arrays
  $osc_harmonics_restriction_table.
    map { |freq| last_harmonic(freq) }.uniq.sort.reverse.each do |i|
    yield(i)
  end
end

generate_osc_wave_table_arrays do |last|
  generate_osc_wave_table("saw", last, 1.0) do |x, k|
    (2.0 / Math::PI) * Math.sin((2.0 * Math::PI) * x * k) / k
  end
end

# The Saw2 wave has no closed-form harmonic series, so it is decomposed by an
# FFT. Tables of different lengths need their own decomposition
$osc_saw2_decompositions = {}

def saw2_decomposition(samples)
  $osc_saw2_decompositions[samples] ||= begin
    table = (0...samples).map do |n|
      x = n.to_f / samples
      1.0 - (x - (x ** 3.0) / 3.0) * 3.0
    end
    ffta = fft(table)
    (0..(samples / 2)).map { |k| ifft(bpf_fft(ffta, k), 1.0) }
  end
end

generate_osc_wave_table_arrays do |last|
  generate_osc_wave_table("saw2", last, 1.0) do |x, k, n, samples|
    saw2_decomposition(samples)[k][n]
  end
end

generate_osc_wave_table_arrays do |last|
  generate_osc_wave_table("triangle", last, 1.0) do |x, k|
    if k % 4 == 1
      +(8.0 / (Math::PI * Math::PI)) * Math.sin((2.0 * Math::PI) * x * k) / (k * k)
    elsif k % 4 == 3
      -(8.0 / (Math::PI * Math::PI)) * Math.sin((2.0 * Math::PI) * x * k) / (k * k)
    else
      0.0
    end
  end
end

generate_osc_wave_table_arrays do |last|
  generate_osc_wave_table("square", last, 1.0) do |x, k|
    if k % 2 == 1
      (4.0 / Math::PI) * Math.sin((2.0 * Math::PI) * x * k) / k
    else
      0.0
    end
  end
end

generate_osc_wave_table("sine", 1, 1.0) do |x, k|
  Math.sin((2.0 * Math::PI) * x * k)
end

def generate_osc_wave_tables_array(name, last = OSC_WAVE_TABLE_LAST_HARMONIC)
  $file.printf("int16_t* g_osc_#{name}_wave_tables[] = {\n  ")
  $osc_harmonics_restriction_table.each_with_index do |freq, idx|
    $file.printf("g_osc_#{name}_wave_table_h%-3d + 1,", [last_harmonic(freq), last].min)
    if idx == DATA_BYTE_MAX
      $file.printf("\n")
    elsif idx % 3 == (3 - 1)
      $file.printf("\n  ")
    else
      $file.printf(" ")
    end
  end
  $file.printf("};\n\n")
end

generate_osc_wave_tables_array("saw")
generate_osc_wave_tables_array("saw2")
generate_osc_wave_tables_array("triangle")
generate_osc_wave_tables_array("square")
generate_osc_wave_tables_array("sine", 1)

$file.printf("int32_t g_portamento_coef_table[] = {\n  ")
(0..127).each do |i|
  time = i
  portamento_coef = (0.5 ** (1.0 / ((0.1 / 10.0) * (SAMPLING_RATE / 4) * (10.0 ** ((time - 64.0) / 32.0)))) * 0x40000000).round
  portamento_coef = 0 if time == 0
  
  $file.printf("%10d,", portamento_coef)
  if i == 127
    $file.printf("\n")
  elsif i % 8 == (8 - 1)
    $file.printf("\n  ")
  else
    $file.printf(" ")
  end
end
$file.printf("};\n\n")

$file.close
