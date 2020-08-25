; Jonathan Frech, August 2020
; a Joy Assembler program to calculate a number's factorial using recursion

; N.b.: To call a function with `n` parameter values, the stack has to be
; expanded by `4*(n+1)` bytes, the last byte being used to store the return
; address.

; global variables in memeory
global-x := prg+0
global-y := prg+4
global-z := prg+8
global-w := prg+12
global-v := prg+16

; stack
pstack := prg+20
mov prg+24
sta pstack

; initialize the stack with a visual marker
ldb pstack
mov 0xffffffff
sia

; jump to main program
jmp @main


factorial:
    ; load argument
    ldb pstack
    lia -4
    sta global-w

    ; base case is 1
    mov 1
    sta global-v

    lda global-w
    jz @base-case
        ; put argument minus one onto stack
        lda global-w
        dec
        psh pstack

        ; recursively call factorial
        ; value of register `A` does not matter; will be overriden by `cal`
        psh pstack
        ldb pstack
        cal @factorial

        ; ignore return address
        pop pstack

        ; return value
        pop pstack
        sta global-v

        ; get this function's invocation argument
        ldb pstack
        lia -4
        sta global-w

        ; prepare stack for multiplication call
        lda global-v
        psh pstack
        lda global-w
        psh pstack

        ; perform multiplication
        psh pstack
        ldb pstack
        cal @multiply

        ; get result and shrink stack
        pop pstack
        pop pstack
        sta global-v
        ; multiply requires two arguments yet only returns one
        pop pstack
    base-case:

    ; return argument
    lda global-v
    ldb pstack
    sia -4

    ; return
    ldb pstack
    ret

; multiply function
multiply:
    ; load local stack variables into global memory
    ldb pstack
    lia -8
    sta global-y
    lia -4
    sta global-x

    ; initialize the result to zero
    mov 0x00000000
    sta global-z

    ; perform the multiplication
    multiply-loop:
        ; put the first bit of the value at address global-y into register A
        mov 0x00000001
        swp
        lda global-y
        and

        ; only add when a bit was found
        jz @skip
            lda global-z
            swp
            lda global-x
            add
            sta global-z
        skip:

        ; shift appropriately
        lda global-x
        shl
        sta global-x

        lda global-y
        shr
        sta global-y

        ; loop
        jnz @multiply-loop

    ; return the result via the stack
    ldb pstack
    lda global-z
    sia -4

    ; second value down the stack is unused in return

    ; return
    ldb pstack
    ret

; main program
main:
    mov 6
    psh pstack

    psh pstack
    ldb pstack
    cal @factorial

    pop pstack
    pop pstack
    ptu

    hlt
