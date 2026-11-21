# Generates the images of the display of the panel (the OLED of 128 x 64 dots) in "images", for README-PRA32-U2-P.md,
# with the font of the panel ("../Digital-Synth-PRA32-U2/pra32-u2-control-panel-font-table.h"): 21 x 8 characters
# of 6 x 8 dots, each dot 2 x 2 pixels (256 x 128; no other library than Zlib of Ruby)
#
#   ruby generate-panel-display-images.rb

require 'zlib'

FONT_TABLE_FILE = File.join(__dir__, '../Digital-Synth-PRA32-U2/pra32-u2-control-panel-font-table.h')
IMAGES_DIR      = File.join(__dir__, 'images')

SCALE  = 2
MARGIN = 0                   # (in the dots; none, so that the image is 256 x 128, and shown at a multiple of 128)
WIDTH  = 128
HEIGHT = 64
DOT_ON  = [0xE8, 0xF4, 0xFF] # (a white, a little blue, as an OLED)
DOT_OFF = [0x00, 0x00, 0x00]

# The screens (8 lines of 21 characters: columns 0-9 on the left, and 11-20 on the right), as the panel shows them
SCREENS = {
  'panel-display-key' => [  # PRA32-U2/P: Key Mode, with the page at the power-on (A-11 Filter a)
    '#0                   ',
    'Ch 1       Panel     ',
    'Page A-11  Play Pitch',
    'Filter a   C= 64[ C4]',
    '                     ',
    'Filter     Filter    ',
    'Cutoff     Resonance ',
    'A=112[8k4] B= 64[2.8]',
  ],
  'panel-display-layering' => [  # PRA32-U2/M/P: the Layering (Ly2) in K+S Mode, the Play Key and the Step Sequencer playing
    '#4            S8  *53',
    'Ch 5 [Ly2] Panel     ',
    'Page A-11  Play Pitch',
    'Filter a   C= 64[ C4]',
    '                     ',
    'Filter     Filter    ',
    'Cutoff     Resonance ',
    'A=112[8k4] B= 64[2.8]',
  ],
  'panel-display-general-purpose' => [  # General Purpose Controller 1 and 2 (Page B-24), for Assign #16 and #17 (74 and 71)
    '#0                   ',
    'Ch 1       Panel     ',
    'Page B-24  Play Pitch',
    'Control e  C= 64[ C4]',
    '                     ',
    'General    General   ',
    'Purpose 1  Purpose 2 ',
    'A=112[ 74] B= 64[ 71]',
  ],
  'panel-display-seq' => [  # Seq Mode (Parameter C is Seq Pitch Ofst), the Step Sequencer playing
    '#0                  3',
    'Ch 1       Seq       ',
    'Page A-11  Pitch Ofst',
    'Filter a   C= 64[ +0]',
    '                     ',
    'Filter     Filter    ',
    'Cutoff     Resonance ',
    'A=112[8k4] B= 64[2.8]',
  ],
}

def read_font
  bytes = File.read(FONT_TABLE_FILE).scan(/0b([01]{8})/).map { |b| b[0].to_i(2) }
  raise 'the font table is not of 128 x 6 bytes' if bytes.size != 128 * 6
  bytes.each_slice(6).to_a
end

def render(font, lines)
  raise 'a screen is not of 8 lines' if lines.size != 8
  dots = Array.new(HEIGHT) { Array.new(WIDTH, false) }
  lines.each_with_index do |line, y|
    raise "not of 21 characters: #{line.inspect}" if line.size != 21
    line.each_char.with_index do |c, x|
      font[c.ord].each_with_index do |column, i|
        8.times { |bit| dots[(y * 8) + bit][(x * 6) + i] = (column[bit] == 1) }  # (bit 0 at the top)
      end
    end
  end
  dots
end

def png_chunk(type, data)
  [data.bytesize].pack('N') + type + data + [Zlib.crc32(type + data)].pack('N')
end

def write_png(path, dots)
  w = (WIDTH  + (MARGIN * 2)) * SCALE
  h = (HEIGHT + (MARGIN * 2)) * SCALE
  raw = ''.b
  h.times do |py|
    raw << "\x00".b  # (no filter)
    y = (py / SCALE) - MARGIN
    w.times do |px|
      x = (px / SCALE) - MARGIN
      on = (y >= 0) && (y < HEIGHT) && (x >= 0) && (x < WIDTH) && dots[y][x]
      raw << (on ? DOT_ON : DOT_OFF).pack('C3')
    end
  end
  png  = "\x89PNG\r\n\x1A\n".b
  png << png_chunk('IHDR', [w, h, 8, 2, 0, 0, 0].pack('NNCCCCC'))
  png << png_chunk('IDAT', Zlib::Deflate.deflate(raw, Zlib::BEST_COMPRESSION))
  png << png_chunk('IEND', '')
  File.binwrite(path, png)
end

font = read_font
SCREENS.each do |name, lines|
  path = File.join(IMAGES_DIR, "#{name}.png")
  write_png(path, render(font, lines))
  puts "Wrote #{path}"
end
