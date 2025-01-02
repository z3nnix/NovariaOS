bits 32

section .multiboot
align 4
    dd 0x1BADB002               ; magic
    dd 0x00000003               ; flags (page align + memory info)
    dd -(0x1BADB002 + 0x00000003) ; checksum

section .text
global start
global port_byte_in
global outb
extern kmain

start:
    cli                  ; Disable interrupts
    mov esp, stack_space ; Set the stack pointer
    push ebx             ; Push multiboot info structure address
    call kmain           ; Call the main function
    hlt                  ; Halt the processor

port_byte_in:
    mov dx, [esp + 4]
    in al, dx
    ret

outb:
    mov dx, [esp + 4]   ; Get the port from the arguments
    mov al, [esp + 8]   ; Get the value from the arguments
    out dx, al          ; Write the value to the port
    ret

section .bss
align 4
stack_bottom:
    resb 16384 ; 16 KB for stack
stack_space:
