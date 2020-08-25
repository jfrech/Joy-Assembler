; Jonathan Frech, 23rd of August 2020
; Joy Assembly code which calculates successive powers of five

; memory layout
addr_powers-of-five-x := prg+0
addr_powers-of-five-n := prg+4

; sequence length
n := 10

jmp @main

; calculate successive powers of five
powers-of-five:
    ; sequence initialization
    mov 1
    sta addr_powers-of-five-x

    powers-of-five-loop:
        ; end condition
        lda addr_powers-of-five-n
        jz @powers-of-five-end
        dec
        sta addr_powers-of-five-n

        ; output and multiply by five
        lda addr_powers-of-five-x
        ptu
        shl 2
        ldb addr_powers-of-five-x
        add
        sta addr_powers-of-five-x

        jmp @powers-of-five-loop

    powers-of-five-end:
        ; sequence calculated

jmp @end
main:
    ; initialize sequence length
    mov n
    sta addr_powers-of-five-n

    jmp @powers-of-five

; end of program
end:
    hlt
