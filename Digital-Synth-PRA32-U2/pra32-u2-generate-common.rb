# Common to the "pra32-u2-generate-*-table.rb" scripts, which write the tables as C++ headers

require_relative 'pra32-u2-constants'

# Writes the header at path: "#pragma once" and a blank line, then what the block writes to it
def write_header(path)
  File.open(path, "wb") do |file|
    file.print("#pragma once\n\n")
    yield file
  end
end

# Writes a C array of the values, per_line values a line, each printed by format, as
#
#   declaration = {
#     first,              <- the first value on a line of its own, if first is given
#     value, value, ...,
#     value, value,
#   };
#
# and then trailer (a blank line by default)
def write_array(file, declaration, values, per_line:, format:, first: nil, trailer: "\n\n")
  file.print("#{declaration} = {\n")
  file.print("  #{format % first},\n") if first
  values.each_slice(per_line) do |line|
    file.print("  ", line.map { |value| "#{format % value}," }.join(" "), "\n")
  end
  file.print("};", trailer)
end

# The controller values 0-127, and one more, the guard entry at the end of a table, which repeats
# the entry for 127, so that the interpolation needs no run-time check at the end of the table,
# and anything above 127 is treated as 127
def controller_values_with_guard(length)
  (0...length).map { |index| [index, length - 2].min }
end
