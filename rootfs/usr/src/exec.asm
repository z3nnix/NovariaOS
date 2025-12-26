.NVM0

push 0
push '/'
push 'b'
push 'i'
push 'n'
push '/'
push 't'
push 'e'
push 's'
push 't'
push '.'
push 'b'
push 'i'
push 'n'
syscall open
store 0

; Вторая строка "ABC"
push 0    ; terminator
push 'A'
push 'B'
push 'C'

; Первая строка "test"
push 0    ; terminator
push 'T'
push 'e'
push 's'
push 't'

; Количество аргументов и дескриптор файла
push 2    ; argc (2 аргумента)
load 0 ; fd

syscall exec

; Если exec не удался, продолжаем выполнение
push 0
syscall exit