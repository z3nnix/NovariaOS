; Example app
; Compiled with NVMa(https://github.com/z3nnix/NVMa)

.NVM0

; Calculate 5 + 3
push 5
push 3
add

; Result (8) now on stack

syscall exit ; result 8 will be use like exitcode
hlt ; if syscall doesn't work - halt the procces