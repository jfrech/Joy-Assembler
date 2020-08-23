; Jonathan Frech, 22nd, 23rd of August 2020
; Joy Assembly code to multiply two integers

; memory layout
addr_multiply-x := 0
addr_multiply-y := 4
addr_multiply-z := 8

; input values to be multiplied
x := 342
y := 83

jmp @main

; multply the value at address addr_multiply-x with those at
; address addr_multiply-y and store the result in addr_mutliply-z
multiply:
    ; initialize the result to zero
    imm 0x00000000
    sta addr_multiply-z

    ; perform the multiplication
    multiply-loop:
        ; put the first bit of the value at address addr_multiply-y into
        ; register A
        imm 0x00000001
        swp
        lda addr_multiply-y
        and

        ; only add when a bit was found
        jz @skip
            lda addr_multiply-z
            swp
            lda addr_multiply-x
            add
            sta addr_multiply-z
        skip:

        ; shift appropriately
        lda addr_multiply-x
        shl
        sta addr_multiply-x

        lda addr_multiply-y
        shr
        sta addr_multiply-y

        ; loop
        jnz @multiply-loop

    ; load the result into register A
    lda addr_multiply-z
    put

jmp @end
main:
    imm x
    sta addr_multiply-x
    put

    imm y
    sta addr_multiply-y
    put

    jmp @multiply

end:
    ; end of program
