source_path = ARGV[0]
output_path = ARGV[1]

raise "err: enter source path and output path" unless source_path && output_path

lines = File.readlines(source_path)

bytes = lines.flat_map do |line|
  line.strip.split(/\s+/).map { |hb| hb.to_i(16) }
end

binary_data = bytes.pack('C*')

File.open(output_path, "wb") { |f| f.write(binary_data) }

puts "Success: #{output_path}"
