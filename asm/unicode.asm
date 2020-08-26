jmp @main

c0:
    uint[1] 0
c1:
    uint[1] 0
c2:
    uint[1] 0

main:

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
