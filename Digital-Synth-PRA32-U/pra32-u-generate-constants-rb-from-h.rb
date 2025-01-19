File.open("../Digital-Synth-PRA32-U2/pra32-u2-constants.h", "r") do |input|
  File.open("./pra32-u2-constants.rb", "w") do |output|
    input.each_line do |line|
      output.puts line.chomp[16..-2]
    end
  end
end
