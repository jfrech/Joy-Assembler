; Jonathan Frech, August 2020
; a Joy Assembler function call example

; global variables in memeory
global-x := prg+0
global-y := prg+4
global-z := prg+8
global-w := prg+12
global-v := prg+16

; stack
pstack := prg+20
*stack := prg+24
mov *stack
sta pstack

; initialize the stack with a visual marker
ldb pstack
mov 0xffffffff
sia
lda pstack
inc 4
sta pstack

jmp @factorial-test

; load two arguments, each four bytes onto the stack
lda pstack
inc 8
sta pstack
swp
; load first function argument
mov 0x00000004
sia -8
; load second function argument
mov 0x00000005
sia -4

; call multiply, passing arguments via the stack
ldb pstack
cal @multiply
mov 0x72
ptc
ldb pstack
lia -4
ptu

; shrink the stack
lda pstack
dec 8
sta pstack

; halt the machine
hlt

factorial-test:
    lda pstack
    inc 8
    sta pstack

    ldb pstack
    mov 6
    sia -4

    ldb pstack
    cal @factorial

    mov 0x0A
    ptc
    ptc
    mov 0x2d
    ptc
    mov 0x3e
    ptc
    mov 0x20
    ptc

    ldb pstack
    lia -4
    ptu

    lda pstack
    dec 8
    sta pstack

    hlt


factorial:
    mov 0x46
    ptc

    ; load argument
    ldb pstack
    lia -4
    sta global-w
    ;ptu

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

        ldb pstack
        lia -4
        sta global-v

        lda pstack
        dec 8
        sta pstack

        ldb pstack
        lia -4
        sta global-w


        lda pstack
        inc 12
        sta pstack

        ldb pstack
        lda global-w
        sia -4
        lda global-v
        sia -8

        ldb pstack
        cal @multiply
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
    ; visually indicate that the multiply function was entered
    mov 0x6D
    ptc

    ; load local stack variables into global memory locations
    ldb pstack
    lia -8
    sta global-y
    lia -4
    sta global-x

    ; visually output the variables
    lda global-x
    ptu
    mov 0x2A
    ptc
    lda global-y
    ptu

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

    mov 0x3D
    ptc
    lda global-z
    ptu
    mov 0x2E
    ptc

    ; second value down the stack is unused

    ; exit cleanly
    mov 0
    swp
    mov 0

    ldb pstack
    ret
