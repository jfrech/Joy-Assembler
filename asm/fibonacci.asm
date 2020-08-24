; Jonathan Frech, 23rd of August 2020
; Joy Assembly code which calculates the Fibonacci sequence

; memory layout
addr_fibonacci-x := 0
addr_fibonacci-y := 4
addr_fibonacci-n := 8

; sequence length
n := 32

jmp @main

; calculate the Fibonacci sequence
fibonacci:
    ; sequence initialization
    mov 0
    sta addr_fibonacci-x
    mov 1
    sta addr_fibonacci-y

    fibonacci-loop:
        ; end condition
        lda addr_fibonacci-n
        jz @fibonacci-end
        dec
        sta addr_fibonacci-n

        ; output x
        lda addr_fibonacci-x
        put

        ; add y to x, call it y and move y to x
        ldb addr_fibonacci-y
        add
        sta addr_fibonacci-y
        stb addr_fibonacci-x

        jmp @fibonacci-loop

    fibonacci-end:
        ; sequence calculated

jmp @end
main:
    ; initialize sequence length
    mov n
    sta addr_fibonacci-n

    jmp @fibonacci

end:
    ; end of program
