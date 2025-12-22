; SPDX-License-Identifier: LGPL-3.0-or-later

section .text
bits 64

global _start
extern kmain

section .bss
align 16
stack_bottom:
    resb 16384  ; 16 KB stack
stack_top:

section .text
_start:
    mov rsp, stack_top

    cld
    push rdi

    call kmain
    
    cli
.hang:
    hlt
    jmp .hang


global inb
inb:
    mov dx, di
    in al, dx
    ret

global outb
outb:
    mov dx, di
    mov al, sil
    out dx, al
    ret