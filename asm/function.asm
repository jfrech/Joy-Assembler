; Jonathan Frech, August 2020
; a Joy Assembler function call example

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
lda pstack
inc 4
sta pstack

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
        ; increment stack
        lda pstack
        inc 8
        sta pstack

        ; put argument minus one onto stack
        lda global-w
        dec
        ldb pstack
        sia -4

        ; recursively call factorial
        ldb pstack
        cal @factorial

        ; get recursive factorial result and shrink stack
        ldb pstack
        lia -4
        sta global-v
        lda pstack
        dec 8
        sta pstack

        ; get this function's invocation argument
        ldb pstack
        lia -4
        sta global-w

        ; prepare stack for multiplication call
        lda pstack
        inc 12
        sta pstack
        ldb pstack
        lda global-w
        sia -4
        lda global-v
        sia -8

        ; perform multiplication
        ldb pstack
        cal @multiply

        ; get result and shrink stack
        ldb pstack
        lia -4
        sta global-v
        lda pstack
        dec 12
        sta pstack
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
    lda pstack
    inc 8
    sta pstack

    ldb pstack
    mov 6
    sia -4

    ldb pstack
    cal @factorial

    ldb pstack
    lia -4
    ptu

    lda pstack
    dec 8
    sta pstack

    hlt
