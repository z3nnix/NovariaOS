; =============================================
; COMPARISON OPERATIONS TEST
; =============================================

.NVM0

; Test 1: CMP (0 < 1 = 1)
push byte 0
push byte 1
cmp             ; [1] - result: 0 < 1 = 1
pop             ; clear stack

; Test 2: EQ (5 == 5 = 1)
push byte 5
push byte 5
eq              ; [1] - result: 5 == 5 = true
pop

; Test 3: NEQ (5 != 3 = 1)
push byte 5
push byte 3
neq             ; [1] - result: 5 != 3 = true
pop

; Test 4: GT (10 > 5 = 1)
push byte 10
push byte 5
gt              ; [1] - result: 10 > 5 = true
pop

; Test 5: LT (3 < 7 = 1)
push byte 3
push byte 7
lt              ; [1] - result: 3 < 7 = true
pop

; Test 6: SWAP + CMP
push byte 10    ; first 10
push byte 5     ; then 5
swap            ; now [5, 10] instead of [10, 5]
cmp             ; [1] - result: 5 < 10 = 1
pop

; Test 7: EQ after SWAP
push byte 8
push byte 8
swap            ; [8, 8] - swap doesn't change identical values
eq              ; [1] - result: 8 == 8 = true
pop

; Test 8: NEQ after SWAP
push byte 6
push byte 9
swap            ; [9, 6]
neq             ; [1] - result: 9 != 6 = true
pop

; Test 9: GT after SWAP
push byte 15
push byte 20
swap            ; [20, 15]
gt              ; [1] - result: 20 > 15 = true
pop

; Test 10: LT after SWAP
push byte 12
push byte 8
swap            ; [8, 12]
lt              ; [1] - result: 8 < 12 = true
pop

; All tests passed successfully
push byte 0
syscall exit
hlt