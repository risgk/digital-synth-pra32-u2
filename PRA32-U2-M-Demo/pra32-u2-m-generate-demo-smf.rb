require_relative '../Digital-Synth-PRA32-U2/pra32-u2-constants'

# "Ode to Joy" (Beethoven, Symphony No. 9; public domain), arranged for PRA32-U2/M in 4 parts,
# written to "pra32-u2-m-demo-ode-to-joy.mid" (SMF format 1, a track for each part):
# - Channel 1 (Main Synth): Melody, Channel 2 (Sub Synth 1): Bass,
#   Channel 3 (Sub Synth 2): Arpeggio, Channel 4 (Sub Synth 3): Rhythm
# - D major, 120 BPM, 4/4: an intro of 2 bars, the theme (8 bars), the middle and the theme (8 bars),
#   them again with the arpeggio and the rhythm busier (8 bars), and the ending
# - The melody is played by #20 Saw Lead up to the theme, by #21 Sync Lead in the middle and the theme,
#   and by #28 Fifth Lead in them again (Program Changes of the Main Synth), with the Modulation Wheel
#   up to 64 from the middle of each long note

SMF_FILE = 'pra32-u2-m-demo-ode-to-joy.mid'

PPQ     = 480
BPM     = 120
LEAD_IN = PPQ / 2  # The setup, then the music after an eighth note
BAR     = PPQ * 4
Q       = PPQ      # Quarter note
E       = PPQ / 2  # Eighth note
S       = PPQ / 4  # Sixteenth note

PC = :program_change

# The setup of each part: a preset (Program Change), then CCs
# The presets, with Amp Gain lowered for the balance of the parts (the same as the user programs of "pra32-u2-m-demo-ode-to-joy.json")
SETUP = [
  [[PC, 20],                     # #20 Saw Lead (Legato: Mono, so that the Sub Synths play),
   [AMP_GAIN, 79]],              # whose FX (Chorus, and Delay of an eighth note at 120 BPM) are for all the synths
  [[PC, 22],                     # #22 Synth Bass
   [FX_ROUTING, 102],            # Comp (no Chorus or Delay on the bass)
   [AMP_GAIN, 82]],
  [[PC, 18],                     # #18 FM Piano
   [AMP_GAIN, 73]],
  # The rhythm: #23 Initial, made a drum of a sine with a pitch drop and noise, whose sound changes with
  # the note: the higher, the brighter (Filter Key Track) and the shorter (EG Att/Dec Key Track)
  [[PC, 23],                     # #23 Initial
   [FX_ROUTING, 102],            # Comp
   [OSC_1_WAVE, 77],             # Sine
   [OSC_2_WAVE, 127],            # White Noise
   [MIXER_OSC_MIX, 80],          # More of the noise
   [FILTER_CUTOFF, 40],
   [FILTER_RESO, 0],
   [FILTER_KEY_TRK, 127],        # +1.000
   [FILTER_MODE, 0],             # Low Pass
   [FILTER_EG_AMT, 100],
   [EG_AMP_MOD, 127],            # On: the EG shapes the level too
   [REL_EQ_DECAY, 127],          # On
   [EG_ATTACK, 0],
   [EG_DECAY, 54],
   [EG_SUSTAIN, 0],
   [EG_OSC_DST, 0],              # Pitch
   [EG_OSC_AMT, 107],            # +12 semitone
   [A_D_KEY_TRK, 31],            # -6 / octave
   [EG_VEL_SENS, 80],            # The hats are played softer
   [AMP_GAIN, 127]],
]

# The melody from the bars: #21 Sync Lead from the middle, and #28 Fifth Lead from them again
# (whose FX are the same as those of #20 Saw Lead)
LEAD_CHANGES = {
  10 => [[PC, 21], [AMP_GAIN, 63]],  # #21 Sync Lead
  18 => [[PC, 28], [AMP_GAIN, 64]],  # #28 Fifth Lead
}

# The Modulation Wheel at the most in a long note (a half note or longer)
MODULATION_MAX = 64

KICK  = 36
SNARE = 55
HAT   = 108

# The melody: [note, length] in each bar
D5, E5, FS5, G5, A5 = 74, 76, 78, 79, 81
A4 = 69
QD = Q + E  # Dotted quarter note

PHRASE = [
  [[FS5, Q], [FS5, Q], [G5, Q], [A5, Q]],
  [[A5, Q], [G5, Q], [FS5, Q], [E5, Q]],
  [[D5, Q], [D5, Q], [E5, Q], [FS5, Q]],
]
THEME_1 = PHRASE + [[[FS5, QD], [E5, E], [E5, 2 * Q]]]
THEME_2 = PHRASE + [[[E5, QD], [D5, E], [D5, 2 * Q]]]
MIDDLE  = [
  [[E5, Q], [E5, Q], [FS5, Q], [D5, Q]],
  [[E5, Q], [FS5, E], [G5, E], [FS5, Q], [D5, Q]],
  [[E5, Q], [FS5, E], [G5, E], [FS5, Q], [E5, Q]],
  [[D5, Q], [E5, Q], [A4, 2 * Q]],
]

# The chords, of the halves of each bar: [name, root of the bass]
CH_D = [:D, 50]
CH_A = [:A, 45]
CH_G = [:G, 43]
TRIADS = { D: [62, 66, 69], A: [61, 64, 69], G: [62, 67, 71] }

CHORDS_THEME_1 = [[CH_D, CH_D], [CH_A, CH_A], [CH_D, CH_D], [CH_D, CH_A]]
CHORDS_THEME_2 = [[CH_D, CH_D], [CH_A, CH_A], [CH_D, CH_D], [CH_A, CH_D]]
CHORDS_MIDDLE  = [[CH_A, CH_D], [CH_A, CH_D], [CH_A, CH_D], [CH_G, CH_A]]

# [melody, chords, busier] of each bar
BARS = [
  *[[nil, CHORDS_THEME_1[0], false], [nil, [CH_D, CH_A], false]],
  *(THEME_1 + THEME_2).zip(CHORDS_THEME_1 + CHORDS_THEME_2).map { |m, c| [m, c, false] },
  *(MIDDLE + THEME_2).zip(CHORDS_MIDDLE + CHORDS_THEME_2).map { |m, c| [m, c, false] },
  *(MIDDLE + THEME_2).zip(CHORDS_MIDDLE + CHORDS_THEME_2).map { |m, c| [m, c, true] },
]

TRACKS = Array.new(4) { [] }

# [tick, order, bytes]: at the same tick, the setup first, and a Note Off before a Note On
def event(channel, tick, bytes)
  order = case bytes[0] & 0xF0
          when NOTE_OFF then 1
          when NOTE_ON  then 2
          else               0
          end
  TRACKS[channel].push([tick, order, bytes])
end

def note(channel, tick, note_number, length, velocity, gate)
  event(channel, tick, [NOTE_ON | channel, note_number, velocity])
  event(channel, tick + [(length * gate).to_i, 1].max, [NOTE_OFF | channel, note_number, 64])
end

# The Modulation Wheel in a long note: up from a third of the note, over a third of it, and back to 0 at its end
def modulation(tick, length, max)
  8.times { |k| event(0, tick + (length / 3) + (length / 3) * k / 8, [CONTROL_CHANGE | 0, MODULATION, max * (k + 1) / 8]) }
  event(0, tick + length, [CONTROL_CHANGE | 0, MODULATION, 0])
end

def setup(channel, tick, items)
  items.each do |number, value|
    if number == PC
      event(channel, tick, [PROGRAM_CHANGE | channel, value])
    else
      event(channel, tick, [CONTROL_CHANGE | channel, number, value])
    end
  end
end

SETUP.each_with_index { |items, channel| setup(channel, 0, items) }

BARS.each_with_index do |(melody, chords, busier), b|
  t0 = LEAD_IN + (b * BAR)

  # Melody
  setup(0, t0, LEAD_CHANGES[b]) if LEAD_CHANGES[b]
  if melody
    t = t0
    melody.each do |note_number, length|
      note(0, t, note_number, length, (t == t0) ? 100 : 88, 0.95)
      modulation(t, length, MODULATION_MAX) if length >= 2 * Q
      t += length
    end
  end

  chords.each_with_index do |(name, root), h|
    th = t0 + (h * 2 * Q)

    # Bass: eighth notes, the root and an octave up by turns
    4.times do |k|
      note(1, th + (k * E), root - 12 + ((k.odd?) ? 12 : 0), E, (k == 0) ? 104 : 84, 0.7)
    end

    # Arpeggio: sixteenth notes, up and down the triad (over 2 octaves when busier)
    triad = TRIADS[name]
    notes = busier ? (triad + triad.map { |n| n + 12 } + [triad[0] + 24]) : (triad + [triad[0] + 12])
    path = notes + notes[1..-2].reverse
    8.times do |k|
      note(2, th + (k * S), path[k % path.size], S, (k % 4 == 0) ? 96 : 80, 0.8)
    end
  end

  # Rhythm: the kick on the beats 1 and 3, the snare on 2 and 4, and the hats on the off-beats
  # (on the sixteenth notes in between when busier)
  16.times do |k|
    t = t0 + (k * S)
    if [0, 8].include?(k) || (busier && k == 10)
      note(3, t, KICK, S, 120, 0.5)
    elsif [4, 12].include?(k)
      note(3, t, SNARE, S, 110, 0.5)
    elsif busier ? k.odd? : (k % 4 == 2)
      note(3, t, HAT, S, (k % 4 == 0) ? 64 : 52, 0.3)
    end
  end
end

# The ending: the tonic, held
t0 = LEAD_IN + (BARS.size * BAR)
note(0, t0, D5, 3 * Q, 100, 1.0)
modulation(t0, 3 * Q, MODULATION_MAX)
note(1, t0, 38, 2 * Q, 110, 0.9)
[62, 66, 69, 74, 78, 81].each_with_index { |n, k| note(2, t0 + (k * S), n, S, 70, 0.8) }
note(3, t0, KICK, S, 124, 0.5)

END_TICK = t0 + BAR + (2 * Q)  # With the tail

def vlq(value)
  bytes = [value & 0x7F]
  while (value >>= 7) > 0
    bytes.unshift((value & 0x7F) | 0x80)
  end
  bytes
end

def meta(type, data)
  [0x00, 0xFF, type, *vlq(data.size), *data]
end

def chunk(type, data)
  type.bytes + [data.size].pack('N').bytes + data
end

def track(name, events)
  data = meta(0x03, name.bytes)
  last = 0
  events.sort_by.with_index { |(tick, order, _), i| [tick, order, i] }.each do |tick, _, bytes|
    data += vlq(tick - last) + bytes
    last = tick
  end
  data + vlq(END_TICK - last) + [0xFF, 0x2F, 0x00]
end

tempo = (60_000_000 / BPM)
conductor = meta(0x03, 'Ode to Joy'.bytes) +
            meta(0x51, [(tempo >> 16) & 0xFF, (tempo >> 8) & 0xFF, tempo & 0xFF]) +
            meta(0x58, [4, 2, 24, 8]) +
            vlq(END_TICK) + [0xFF, 0x2F, 0x00]

NAMES = ['Melody (Main Synth)', 'Bass (Sub Synth 1)', 'Arpeggio (Sub Synth 2)', 'Rhythm (Sub Synth 3)']

smf = chunk('MThd', [0, 1, 0, 1 + TRACKS.size, (PPQ >> 8), (PPQ & 0xFF)])
smf += chunk('MTrk', conductor)
TRACKS.each_with_index { |events, channel| smf += chunk('MTrk', track(NAMES[channel], events)) }

File.binwrite(File.join(__dir__, SMF_FILE), smf.pack('C*'))
