#!/bin/sh
# Assembler compilation
nasm -f elf32 core/kernel/kernel.asm -o kasm.o

# Compile the C file with stack protection disabled and the flag for unexecutable stack.

gcc -I./. -fno-stack-protector -m32 -c core/kernel/kernel.c -o kc.o
gcc -I./. -fno-stack-protector -m32 -c core/kernel/syscall.c -o syscall.o
gcc -I./. -fno-stack-protector -m32 -c core/kernel/acpi.c -o acpi.o

# Compose with flag for non-executable stack
ld -m elf_i386 -T link.ld -o kernel kasm.o syscall.o acpi.o kc.o -z noexecstack

# Move the compiled kernel to the correct directory
mv kernel build/boot/kernel.bin

# Get the current date in YYYY-MM-DD format
DATE=$(date +%Y-%m-%d)

# Create an ISO image with the date in the filename
grub-mkrescue -o "build_${DATE}.iso" build

# Delete temporary files
rm kasm.o kc.o syscall.o acpi.o

# Run QEMU with the created ISO image
qemu-system-i386 -m 40M -cdrom "build_${DATE}.iso"
