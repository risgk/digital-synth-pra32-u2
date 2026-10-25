require_relative 'Digital-Synth-PRA32-U2/pra32-u2-constants'

# The sample MIDI stream, for pra32-u2-make-sample-wav-file.cc, which renders the time of a byte at
# 38400 bps (12 samples) after each byte: so a wait is Active Sensings, a byte each

PRA32_U2_MIDI_CH  = 0  # 0-based
NOTE_ON_VELOCITY  = 100
NOTE_OFF_VELOCITY = 64

STREAM = []

def wait(length)
  STREAM.concat([ACTIVE_SENSING] * length)
end

def control_change(control_number, value)
  STREAM.push(CONTROL_CHANGE | PRA32_U2_MIDI_CH, control_number, value)
  wait(10)
end

def program_change(program_number)
  STREAM.push(PROGRAM_CHANGE | PRA32_U2_MIDI_CH, program_number)
  wait(800)
end

def note_on(note_number, velocity = NOTE_ON_VELOCITY)
  STREAM.push(NOTE_ON | PRA32_U2_MIDI_CH, note_number, velocity)
end

def note_off(note_number)
  STREAM.push(NOTE_OFF | PRA32_U2_MIDI_CH, note_number, NOTE_OFF_VELOCITY)
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

File.binwrite("pra32-u2-sample-midi-stream.bin", STREAM.pack("C*"))
