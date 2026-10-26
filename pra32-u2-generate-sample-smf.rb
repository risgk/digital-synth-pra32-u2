require_relative 'Digital-Synth-PRA32-U2/pra32-u2-constants'

# The sample Standard MIDI File "pra32-u2-sample.mid", for checking a change of the sound on PC with
# "PRA32-U2-M-Demo/pra32-u2-m-make-wav-file-from-smf.cc" (PRA32-U2/M, whose Main Synth on MIDI Channel 1 plays
# as PRA32-U2 does): a tick is 12 samples (250 us: 480 ticks a quarter note of 120 ms), the time of a byte at
# 38400 bps, and each message takes the ticks of its bytes, as the sample MIDI stream did (until v3.17.4)

PRA32_U2_MIDI_CH  = 0  # 0-based
NOTE_ON_VELOCITY  = 100
NOTE_OFF_VELOCITY = 64

PPQ   = 480
TEMPO = 120_000  # us a quarter note

EVENTS = []  # [tick, bytes]
$tick = 0

def message(bytes)
  $tick += bytes.size
  EVENTS.push([$tick, bytes])
end

def wait(length)
  $tick += length
end

def control_change(control_number, value)
  message([CONTROL_CHANGE | PRA32_U2_MIDI_CH, control_number, value])
  wait(10)
end

def program_change(program_number)
  message([PROGRAM_CHANGE | PRA32_U2_MIDI_CH, program_number])
  wait(800)
end

def note_on(note_number, velocity = NOTE_ON_VELOCITY)
  message([NOTE_ON | PRA32_U2_MIDI_CH, note_number, velocity])
end

def note_off(note_number)
  message([NOTE_OFF | PRA32_U2_MIDI_CH, note_number, NOTE_OFF_VELOCITY])
end

def sound_off
  control_change(ALL_SOUND_OFF, 0)
  wait(800)
end

# The 4 chords (or their roots), relative to the octave oct
CHORDS = [[12, 16, 19, 23], [16, 19, 23, 26], [14, 17, 21, 24], [17, 21, 24, 28]]

# Each root alone
def play_mono(oct)
  CHORDS.each do |chord|
    note = chord.first + (oct * 12)
    note_on(note)
    wait(3200)
    note_off(note)
    wait(800)
  end
end

# Each chord, its notes on one by one and off in reverse
def play_arpeggiated(oct)
  CHORDS.each do |chord|
    notes = chord.map { |x| x + (oct * 12) }
    notes.each_with_index do |note, i|
      note_on(note)
      wait(i < notes.size - 1 ? 800 : 3200)
    end
    notes.reverse_each do |note|
      note_off(note)
      wait(800)
    end
  end
end

sound_off

program_change(4)
play_mono(3)
sound_off

program_change(3)
play_arpeggiated(3)
sound_off

def vlq(value)
  bytes = [value & 0x7F]
  while (value >>= 7) > 0
    bytes.unshift((value & 0x7F) | 0x80)
  end
  bytes
end

track = [0x00, 0xFF, 0x51, 0x03, (TEMPO >> 16) & 0xFF, (TEMPO >> 8) & 0xFF, TEMPO & 0xFF]
last = 0
EVENTS.each do |tick, bytes|
  track += vlq(tick - last) + bytes
  last = tick
end
track += vlq($tick - last) + [0xFF, 0x2F, 0x00]

smf = 'MThd'.bytes + [6].pack('N').bytes + [0, 0, 0, 1, (PPQ >> 8), (PPQ & 0xFF)] +
      'MTrk'.bytes + [track.size].pack('N').bytes + track
File.binwrite(File.join(__dir__, 'pra32-u2-sample.mid'), smf.pack('C*'))
