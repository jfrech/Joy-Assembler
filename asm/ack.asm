; memory layout

mov @stack
lsc
mov 0xffeefeff
psh

jmp @main

global-m:
    data[1] 0
global-n:
    data[1] 0
global-a:
    data[1] 0

; main program
main:
    ; m
    mov 3
    psh
    ; n
    mov 5
    psh

    cal @ack

    ; ack(m, n)
    pop
    ptu
    ; ignore (ack takes two argument, yet only returns one)
    pop

    hlt


; ack(0, n) = n+1
; ack(m, 0) = ack(m-1, 1)
; ack(m, n) = ack(m-1, ack(m, n-1))
ack:
    lsa -12
    sta @global-m
    lsa -8
    sta @global-n

    lda @global-m
    jz @base-left

    lda @global-n
    jz @base-right

    jmp @general

    ; ack(m, n) = ack(m-1, ack(m, n-1))
    general:
        ; ack(m, n-1)
        lda @global-m
        psh
        lda @global-n
        dec
        psh

        cal @ack

        pop
        sta @global-a
        pop

        ; ack(m-1, ack(m, n-1))
        lda @global-a
        sta @global-n
        lsa -12
        dec
        sta @global-m

        lda @global-m
        psh
        lda @global-n
        psh

        cal @ack

        pop
        sta @global-a
        pop

        jmp @return

    ; ack(0, n) = n+1
    base-left:
        lda @global-n
        inc
        sta @global-a

        jmp @return

    ; ack(m, 0) = ack(m-1, 1)
    base-right:
        lda @global-m
        dec
        psh
        mov 1
        psh

        cal @ack

        pop
        sta @global-a
        pop

        jmp @return

    ; put `global-a` onto the stack (where one argument once resided)
    return:
        lda @global-a
        ssa -8
        ret

stack:
    data[0xfff] 0
