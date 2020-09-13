; Jonathan Frech, August 2020
; Joy Assembly code to multiply two integers

jmp @main

; memory layout
global-x:
    data[1] 0
global-y:
    data[1] 0
global-z:
    data[1] 0

; input values to be multiplied
x := 342
y := 83

; multiply the value at address global-x with those at
; address global-y and store the result in addr_mutliply-z
multiply:
    ; initialize the result to zero
    mov 0x00000000
    sta @global-z

    ; perform the multiplication
    multiply-loop:
        ; put the first bit of the value at address global-y into
        ; register A
        mov 0x00000001
        swp
        lda @global-y
        and

        ; only add when a bit was found
        jz @skip
            lda @global-z
            swp
            lda @global-x
            add
            sta @global-z
        skip:

        ; shift appropriately
        lda @global-x
        shl
        sta @global-x

        lda @global-y
        shr
        sta @global-y

        ; loop
        jnz @multiply-loop

    ; load the result into register A
    lda @global-z
    ptu

jmp @end
main:
    mov x
    sta @global-x
    ptu

    mov y
    sta @global-y
    ptu

    jmp @multiply

; end of program
end:
    hlt
