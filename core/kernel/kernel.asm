bits 32

section .multiboot
align 4
    dd 0x1BADB002               ; magic
    dd 0x00000003               ; flags (page align + memory info)
    dd -(0x1BADB002 + 0x00000003) ; checksum

section .text
global start
global port_byte_in
global port_byte_out
extern kmain

start:
    cli                  ; Disable interrupts
    mov esp, stack_space ; Set the stack pointer
    push ebx             ; Push multiboot info structure address
    call kmain           ; Call the main function
    hlt                  ; Halt the processor

; Function to read a byte from a port
port_byte_in:
    mov dx, [esp + 4]    ; Get the port from the arguments
    in al, dx            ; Read the value from the port
    mov [esp + 4], al    ; Store the value in the return location
    ret

; Function to write a byte to a port
port_byte_out:
    mov dx, [esp + 4]    ; Get the port from the arguments
    mov al, [esp + 8]    ; Get the value from the arguments
    out dx, al           ; Write the value to the port
    ret

section .bss
align 4
stack_bottom:
    resb 16384           ; 16 KB for stack
stack_space: