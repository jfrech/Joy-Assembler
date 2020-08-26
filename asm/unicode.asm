jmp @main

c0:
    uint[1] 0
c1:
    uint[1] 0
c2:
    uint[1] 0

nop 0xaabbccdd
msgptr:
    uint[1] 0
msg:
    string "Hällö wörld. ♖"

print-string:
    mov @msg
    sta @msgptr

    print-string-loop:
        lda @msgptr
        swp
        lia
        jz @print-string-end
        ptc
        swp
        inc 4
        sta @msgptr

        jmp @print-string-loop
    print-string-end:

    mov 0x0a
    ptc
    hlt


main:
    jmp @print-string
    hlt

    gtc
    sta @c0

    gtc
    sta @c1

    gtc
    sta @c2

    mov 0x4f
    ptc
    mov 0x4b
    ptc
    mov 0x0a
    ptc

    lda @c2
    ptc
    lda @c1
    ptc
    lda @c0
    ptc

    hlt




hlt
mov 0x04c1
swp
gtc

swp
ptc
swp
ptc

hlt
