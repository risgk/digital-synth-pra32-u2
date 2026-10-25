require_relative 'pra32-u2-generate-common'

# refs http://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
# Cooley–Tukey FFT algorithm - Wikipedia, the free encyclopedia

def fft(a)
  ditfft2(a, a.size, 1)
end

def ditfft2(x, n, s)
  return [x[0]] if n == 1

  result = ditfft2(x, n / 2, 2 * s) + ditfft2(x.drop(s), n / 2, 2 * s)
  (0...(n / 2)).each do |k|
    t = result[k]
    result[k]         = t + (Math::E ** Complex(0, -2 * Math::PI * k / n)) * result[k + n / 2]
    result[k + n / 2] = t - (Math::E ** Complex(0, -2 * Math::PI * k / n)) * result[k + n / 2]
  end
  result
end

def ifft(ffta, amp)
  n = ffta.size
  fft(ffta.map(&:conj)).map(&:conj).map { |i| i * amp / n }.map(&:real)
end

# Only the harmonic k (and its mirror) of ffta
def bpf_fft(ffta, k)
  n = ffta.size
  a = ffta.clone
  (0..(n / 2)).each do |i|
    next if i == k

    a[i] = 0.0
    a[n - i] = 0.0
  end
  a
end

def freq_from_note_number(note_number, print = false)
  cent = (note_number * 100.0) - 6900.0
  hz = A4_FREQ * (2.0 ** (cent / 1200.0))
  freq = (hz * (1 << OSC_PHASE_RESOLUTION_BITS) / SAMPLING_RATE).floor.to_i
  freq += 1 if freq.even?
  if print
    printf("[0]%3d, %+f, %d\n", note_number, 1.0 - freq.to_f * SAMPLING_RATE / (hz * (1 << OSC_PHASE_RESOLUTION_BITS)), freq)
  end
  freq
end

# The tune rate of the largest detune, to keep the harmonics of a detuned note below FREQUENCY_MAX
# (the hardcoded constants preserve the original harmonics restriction)
MAX_TUNE_RATE = (0...(1 << 8)).map do |i|
  ((2.0 ** ((i - (1 << (8 - 1))) / (12.0 * (1 << 8)))) * (1 << 15) / 1.0).round - (1 << 15) / 1.0
end.max

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
  Math.log2(samples).ceil.clamp(OSC_WAVE_TABLE_SAMPLES_BITS_MIN,
                                soft ? OSC_WAVE_TABLE_SAMPLES_BITS_MAX_SOFT : OSC_WAVE_TABLE_SAMPLES_BITS_MAX)
end

# The table of the wave name with the harmonics 1 to last, each given by the block, for the
# position x (0.0 to 1.0), the harmonic k, the index n, and the samples; the entry after the last
# sample repeats the first, for the interpolation
def write_osc_wave_table(file, name, last, amp)
  bits = table_samples_bits(name, last)
  samples = 1 << bits
  levels = (0..samples).map do |n|
    nn = (n == samples) ? 0 : n
    # Added up one by one (not by sum, which compensates the rounding), as the table always was
    level = (1..last).inject(0) { |total, k| total + yield(nn.to_f / samples, k, nn, samples) }
    ((level * amp) * OSC_WAVE_TABLE_AMP).round.to_i
  end
  # The first entry is the number of index bits, which the run time reads
  # back through the pointer the table array holds (which points at [1])
  write_array(file, "int16_t g_osc_#{name}_wave_table_h#{last}[]", levels, per_line: 16, format: "%+6d", first: bits)
end

OSC_HARMONICS_RESTRICTION_TABLE = (NOTE_NUMBER_MIN..NOTE_NUMBER_MAX).map do |note_number|
  correction = (MAX_TUNE_RATE.to_f + (1 << 15)) / (1 << 15)
  freq = (freq_from_note_number(((note_number + (3 - 1)) / 3) * 3) * correction).floor
  freq + 1
end

OSC_DETUNE_CORRECRION = 1060  # Approx. 101 cents

def last_harmonic(freq)
  correction = [freq * (OSC_DETUNE_CORRECRION - 1000) / 1000, OSC_DETUNE_FREQ_MAX].max
  last = (freq != 0) ? ((FREQUENCY_MAX * (1 << OSC_PHASE_RESOLUTION_BITS)) /
                        ((freq + correction) * SAMPLING_RATE)) : 0
  [last, OSC_WAVE_TABLE_LAST_HARMONIC].min
end

# The last harmonics of the tables of a wave, from the most
LAST_HARMONICS = OSC_HARMONICS_RESTRICTION_TABLE.map { |freq| last_harmonic(freq) }.uniq.sort.reverse

# The Saw2 wave has no closed-form harmonic series, so it is decomposed by an
# FFT. Tables of different lengths need their own decomposition
SAW2_DECOMPOSITIONS = {}

def saw2_decomposition(samples)
  SAW2_DECOMPOSITIONS[samples] ||= begin
    table = (0...samples).map do |n|
      x = n.to_f / samples
      1.0 - (x - (x ** 3.0) / 3.0) * 3.0
    end
    ffta = fft(table)
    (0..(samples / 2)).map { |k| ifft(bpf_fft(ffta, k), 1.0) }
  end
end

# The array of the tables of the wave name for the note numbers, each pointing past the entry of
# the index bits
def write_osc_wave_tables_array(file, name, last = OSC_WAVE_TABLE_LAST_HARMONIC)
  lasts = OSC_HARMONICS_RESTRICTION_TABLE.map { |freq| [last_harmonic(freq), last].min }
  write_array(file, "int16_t* g_osc_#{name}_wave_tables[]", lasts, per_line: 3, format: "g_osc_#{name}_wave_table_h%-3d + 1")
end

def portamento_coef(time)
  return 0 if time == 0

  (0.5 ** (1.0 / ((0.1 / 10.0) * (SAMPLING_RATE / 4) * (10.0 ** ((time - 64.0) / 32.0)))) * 0x40000000).round
end

write_header("pra32-u2-osc-table.h") do |file|
  # Extended to NOTE_NUMBER_MAX + 1 (0..128) for safe linear interpolation (idx + 1): the index 128
  # duplicates the last frequency entry as a guard
  freqs = (NOTE_NUMBER_MIN..(NOTE_NUMBER_MAX + 1)).map do |note_number|
    freq_from_note_number([note_number, NOTE_NUMBER_MAX].min, note_number <= NOTE_NUMBER_MAX)
  end
  write_array(file, "uint32_t g_osc_freq_table[]", freqs, per_line: 12, format: "0x%08X")

  LAST_HARMONICS.each do |last|
    write_osc_wave_table(file, "saw", last, 1.0) do |x, k|
      (2.0 / Math::PI) * Math.sin((2.0 * Math::PI) * x * k) / k
    end
  end

  LAST_HARMONICS.each do |last|
    write_osc_wave_table(file, "saw2", last, 1.0) do |x, k, n, samples|
      saw2_decomposition(samples)[k][n]
    end
  end

  LAST_HARMONICS.each do |last|
    write_osc_wave_table(file, "triangle", last, 1.0) do |x, k|
      case k % 4
      when 1 then +(8.0 / (Math::PI * Math::PI)) * Math.sin((2.0 * Math::PI) * x * k) / (k * k)
      when 3 then -(8.0 / (Math::PI * Math::PI)) * Math.sin((2.0 * Math::PI) * x * k) / (k * k)
      else        0.0
      end
    end
  end

  LAST_HARMONICS.each do |last|
    write_osc_wave_table(file, "square", last, 1.0) do |x, k|
      k.odd? ? (4.0 / Math::PI) * Math.sin((2.0 * Math::PI) * x * k) / k : 0.0
    end
  end

  write_osc_wave_table(file, "sine", 1, 1.0) do |x, k|
    Math.sin((2.0 * Math::PI) * x * k)
  end

  write_osc_wave_tables_array(file, "saw")
  write_osc_wave_tables_array(file, "saw2")
  write_osc_wave_tables_array(file, "triangle")
  write_osc_wave_tables_array(file, "square")
  write_osc_wave_tables_array(file, "sine", 1)

  write_array(file, "int32_t g_portamento_coef_table[]", (0..127).map { |i| portamento_coef(i) }, per_line: 8, format: "%10d")
end
