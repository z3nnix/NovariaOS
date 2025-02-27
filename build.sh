#!/bin/sh

# Function to compile C files
compile_c_file() {
    local file="$1"
    local output="$2"
    gcc -I./. -fno-stack-protector -m32 -c "$file" -o "$output" || {
        echo "Error compiling $file"
        exit 1
    }
}

# Assembler compilation
nasm -f elf32 core/arch/boot.asm -o kasm.o || {
    echo "Error assembling kernel.asm"
    exit 1
}

# Compile C files
compile_c_file core/arch/acpi.c acpi.o
compile_c_file core/arch/pause.c pause.o

compile_c_file core/kernel/kernel.c kc.o
compile_c_file core/kernel/kstd.c kstd.o
compile_c_file core/kernel/mem.c mem.o

compile_c_file core/drivers/ps2.c ps2.o
compile_c_file core/drivers/vga.c vga.o
compile_c_file core/drivers/serial.c serial.o

# Link object files with flags for non-executable stack
ld -m elf_i386 -T link.ld -o kernel *.o -z noexecstack || {
    echo "Error linking object files"
    exit 1
}

# Move the compiled kernel to the correct directory
mv kernel build/boot/kernel.bin || {
    echo "Error moving kernel to build/boot"
    exit 1
}

# Get the current date in YYYY-MM-DD format
DATE=$(date +%Y-%m-%d)

# Create an ISO image with the date in the filename
grub-mkrescue -o "build_${DATE}.iso" build || {
    echo "Error creating ISO image"
    exit 1
}

# Delete temporary object files
rm *.o

# Run QEMU with the created ISO image
qemu-system-i386 -m 18M -serial file:qemu.log -cdrom "build_${DATE}.iso" || {
    echo "Error running QEMU"
    exit 1
}

echo "Build and run completed successfully!"