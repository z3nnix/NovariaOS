; =============================================
; LOAD&STORE TEST
; =============================================

.NVM0

; Init a few variables
push byte 10
store 0              ; store to variable #0

push byte 20         ; value 20  
store 1              ; store to variable #1

push byte 5          ; value 5
store 2              ; store to variable #2

; Now working with variables
load 0               ; load variable #0 (10)
load 1               ; load variable #1 (20)
add                  ; 10 + 20 = 30

load 2               ; load variable #2 (5)
sub                  ; 30 - 5 = 25

; Save result to a new variable
store 3              ; save result to variable #3

; Check what was saved
load 3               ; load variable #3 (should be 25)
dup                  ; duplicate for output

; Output the result
syscall print_num    ; system call to print number
pop                  ; remove extra copy

; Terminate program
push byte 0
syscall exit
hlt