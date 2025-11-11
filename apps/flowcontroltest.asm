; =============================================
; FLOW CONTROL TEST
; =============================================

.NVM0

; Test 1: JMP (unconditional jump)
; Expect: skip push 99, no extra values
jmp jmp_done
push byte 99         ; skipped
jmp_done:

; Test 2: JZ (jump if zero)
; Push 0 → condition true → jump → skip push 99
push byte 0
jz jz_done
push byte 99         ; skipped
jz_done:
push byte 1           ; marker 1

; Test 3: JNZ (jump if not zero)
; Push 5 → nonzero → jump → skip push 99
push byte 5
jnz jnz_done
push byte 99          ; skipped
jnz_done:
push byte 2           ; marker 2

; Test 4: Simple decrement loop
; Count from 2 down to 0
push byte 2           ; counter = 2
loop_start:
dup                   ; duplicate counter
push byte 0
eq                    ; check if counter == 0
jz loop_end           ; jump if zero
push byte 1
sub                   ; counter -= 1
jmp loop_start
loop_end:
pop                   ; remove counter
push byte 3           ; marker 3

; Test 5: Conditional GT
; 10 > 5 → true → jump → skip push 0
push byte 10
push byte 5
gt
jnz gt_true
push byte 0           ; skipped
gt_true:
push byte 4           ; marker 4

; Final marker for successful exit
push byte 255
syscall exit
hlt

; EXPECTED FINAL STACK: [1, 2, 3, 4, 255]
; =============================================