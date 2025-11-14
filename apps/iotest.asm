.NVM0
; Write/Read test on port 0x80 (safe for writes)

; 1. Read original value
push 0x80
syscall inb

; 2. Write test pattern
push 0x80    ; port
push 0x55    ; value (01010101)
syscall outb

; 3. Read back
push 0x80
syscall inb

; Stack: [original, new_value]

syscall exit
hlt