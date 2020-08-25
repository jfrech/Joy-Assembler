; Jonathan Frech, August 2020
; a Joy Assembler program to calculate a number's factorial using recursion

; N.b.: The memory location identified by the register `SC` never holds valuable
; information, i.e. is written to. When entering a function, `lsa -4` loads the
; return address, not the first argument. The first argument is loaded into
; register `A` via `lsa -8`, the second via `lsa -12` and so forth.

mov @stack
lsc

; initialize the stack with a visual marker
mov 0xffffffff
psh

jmp @main

; global variables in memeory
global-x:
    uint[1] 0
global-y:
    uint[1] 0
global-z:
    uint[1] 0
global-w:
    uint[1] 0
global-v:
    uint[1] 0


factorial:
    ; load argument
    lsa -8
    sta @global-w

    ; base case is 1
    mov 1
    sta @global-v

    lda @global-w
    jz @base-case
        ; put argument minus one onto stack
        lda @global-w
        dec
        psh

        ; recursively call factorial
        ; value of register `A` does not matter; will be overriden by `cal`
        cal @factorial

        ; return value
        pop
        sta @global-v

        ; get this function's invocation argument
        lsa -8
        sta @global-w

        ; prepare stack for multiplication call
        lda @global-v
        psh
        lda @global-w
        psh

        ; perform multiplication
        cal @multiply

        ; get result and shrink stack
        pop
        sta @global-v
        ; multiply requires two arguments yet only returns one
        pop
    base-case:

    ; return argument
    lda @global-v
    ssa -8

    ; return
    ret

; multiply function
multiply:
    nop 0xeeeeeeee
    ; load local stack variables into global memory
    lsa -8
    sta @global-y
    lsa -12
    sta @global-x

    ; initialize the result to zero
    mov 0x00000000
    sta @global-z

    ; perform the multiplication
    multiply-loop:
        ; put the first bit of the value at address @global-y into register A
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

    ; return the result via the stack
    lda @global-z
    ssa -8

    ; second value down the stack is unused in return

    ; return
    ret

; main program
main:
    mov 6
    psh

    cal @factorial

    pop
    ptu

    hlt

stack:
    uint[0x20] 0
