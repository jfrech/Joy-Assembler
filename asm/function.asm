; Jonathan Frech, August 2020
; a Joy Assembler function call example

; global variables in memeory
global-x := prg+0
global-y := prg+4
global-z := prg+8
global-w := prg+12

; stack
pstack := prg+16
*stack := prg+20
mov *stack
sta pstack

; initialize the stack with a visual marker
ldb pstack
mov 0xffffffff
sia
lda pstack
inc 4
sta pstack


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

    ; second value down the stack is unused

    ; exit cleanly
    mov 0
    swp
    mov 0

    ldb pstack
    ret
