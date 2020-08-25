; memory layout
global-m := prg+0
global-n := prg+4
global-a := prg+8

pstack := prg+20
mov prg+24
sta pstack

jmp @main


; main program
main:
    ; m
    mov 3
    psh pstack
    ; n
    mov 2
    psh pstack

    psh pstack
    ldb pstack
    cal @ack
    pop pstack

    ; ack(m, n)
    pop pstack
    ptu

    ; ignore (ack takes two argument, yet only returns one)
    pop pstack

    hlt


; ack(0, n) = n+1
; ack(m, 0) = ack(m-1, 1)
; ack(m, n) = ack(m-1, ack(m, n-1))
ack:
    ldb pstack
    lia -8
    sta global-m
    ldb pstack
    lia -4
    sta global-n

    lda global-m
    jz @base-left

    lda global-n
    jz @base-right

    jmp @general

    ; ack(m, n) = ack(m-1, ack(m, n-1))
    general:
        ; ack(m, n-1)
        lda global-m
        psh pstack
        lda global-n
        dec
        psh pstack

        psh pstack
        ldb pstack
        cal @ack
        pop pstack

        pop pstack
        sta global-a
        pop pstack

        ; ack(m-1, ack(m, n-1))
        lda global-a
        sta global-n
        ldb pstack
        lia -8
        dec
        sta global-m

        lda global-m
        psh pstack
        lda global-n
        psh pstack

        psh pstack
        ldb pstack
        cal @ack
        pop pstack

        pop pstack
        sta global-a
        pop pstack

        jmp @return

    ; ack(0, n) = n+1
    base-left:
        lda global-n
        inc
        sta global-a

        jmp @return

    ; ack(m, 0) = ack(m-1, 1)
    base-right:
        lda global-m
        dec
        psh pstack
        mov 1
        psh pstack

        psh pstack
        ldb pstack
        cal @ack
        pop pstack

        pop pstack
        sta global-a
        pop pstack

    ; put `global-a` onto the stack (where one argument once resided)
    return:
        lda global-a
        ldb pstack
        sia -4
        ret
