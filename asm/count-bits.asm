; Jonathan Frech, 23rd of August 2020
; Joy Assembly code which calculates the number of bits in a given value

addr_temporary-value-a := 0
addr_temporary-value-b := 4

jmp @main

count-bits:
    sta addr_temporary-value-a
    mov 0
    sta addr_temporary-value-b

    count-bits-loop:
        mov 1
        swp
        lda addr_temporary-value-a
        and
        swp

        lda addr_temporary-value-a
        shr
        sta addr_temporary-value-a

        swp
        jz @not-set
            lda addr_temporary-value-b
            inc
            sta addr_temporary-value-b
        not-set:

        lda addr_temporary-value-a
        jnz @count-bits-loop

    lda addr_temporary-value-b

    jmp @back

jmp @end
main:
    mov 0xab
    jmp @count-bits
    back:
    put
end:
