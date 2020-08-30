; multiply function
multiply:
    jmp @multiply-data-skip
    multiply-x:
        data 0
    multiply-y:
        data 0
    multiply-z:
        data 0
    multiply-data-skip:

    ; load local stack variables into global memory
    lsa -8
    sta @multiply-y
    lsa -12
    sta @multiply-x

    ; initialize the result to zero
    mov 0x00000000
    sta @multiply-z

    ; perform the multiplication
    multiply-loop:
        lda @multiply-y

        ; only add when a bit was found
        je @skip
            lda @multiply-z
            swp
            lda @multiply-x
            add
            sta @multiply-z
        skip:

        ; shift appropriately
        lda @multiply-x
        shl
        sta @multiply-x

        lda @multiply-y
        shr
        sta @multiply-y

        ; loop
        jnz @multiply-loop

    ; return the result via the stack
    lda @multiply-z
    ssa -8

    ; second value down the stack is unused in return

    ; return
    ret
