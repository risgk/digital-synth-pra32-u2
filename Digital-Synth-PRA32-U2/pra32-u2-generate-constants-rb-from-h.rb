# pra32-u2-constants.rb from pra32-u2-constants.h: each line without its first 16 characters
# ("const uint8_t   " and the like) and its last (";"), so that the scripts share the constants
lines = File.readlines("../Digital-Synth-PRA32-U2/pra32-u2-constants.h", chomp: true)
File.binwrite("./pra32-u2-constants.rb", lines.map { |line| "#{line[16..-2]}\n" }.join)
