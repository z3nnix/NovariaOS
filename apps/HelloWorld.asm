.NVM0
; Write "Hello, World!" at the end of VGA buffer

; VGA buffer: 0xB8000 - 0xB8F9E (80x25 * 2 = 4000 bytes)
; Last line starts at: 0xB8F00 (24 * 80 * 2 = 3840 = 0xF00)

push 0xB8F00      ; Start of last line
push 0x0748       ; 'H'
store_abs

push 0xB8F02      ; 'e'  
push 0x0765
store_abs

push 0xB8F04      ; 'l'
push 0x076C
store_abs

push 0xB8F06      ; 'l'
push 0x076C
store_abs

push 0xB8F08      ; 'o'
push 0x076F
store_abs

push 0xB8F0A      ; ','
push 0x072C
store_abs

push 0xB8F0C      ; ' '
push 0x0720
store_abs

push 0xB8F0E      ; 'W'
push 0x0757
store_abs

push 0xB8F10      ; 'o'
push 0x076F
store_abs

push 0xB8F12      ; 'r'
push 0x0772
store_abs

push 0xB8F14      ; 'l'
push 0x076C
store_abs

push 0xB8F16      ; 'd'
push 0x0764
store_abs

push 0xB8F18      ; '!'
push 0x0721
store_abs

; Exit
push 0
syscall exit

hlt