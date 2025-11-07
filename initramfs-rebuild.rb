#!/usr/bin/env ruby
require 'fileutils'

def create_initramfs(app_dir, output_file)
  bin_files = Dir.glob(File.join(app_dir, "*.bin")).sort
  puts "Found #{bin_files.size} .bin files in #{app_dir}"
  
  File.open(output_file, 'wb') do |out|
    bin_files.each do |bin_file|
      data = File.binread(bin_file)
      size = data.bytesize
      
      puts "Adding #{File.basename(bin_file)} (#{size} bytes)"
      
      out.write([size].pack('N'))
      
      out.write(data)
    end
  end
  
  puts "Created initramfs: #{output_file} (#{File.size(output_file)} bytes)"
end

if ARGV[0] != nil
  app_dir = ARGV[0]
else
  app_dir = "apps/"
end

if ARGV[1] != nil
  output_dir = ARGV[1]
else
  output_file = "build/boot/initramfs"
end

unless Dir.exist?(app_dir)
  puts "Error: Directory #{app_dir} does not exist"
  exit 1
end

FileUtils.mkdir_p(File.dirname(output_file))

create_initramfs(app_dir, output_file)