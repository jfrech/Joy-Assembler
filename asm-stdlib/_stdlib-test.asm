jmp @main

include "arithmetic.asm"
include "io.asm"

main:
    mov @a-string
    psh
    cal @puts
    pop

    mov 123
    psh
    mov 456
    psh
    cal @multiply
    pop
    ptu
    pop

    hlt

_:
    data [0xf]0xffffffff

a-string:
    data "testing `puts` \u21AC ...\0"

stack:
    data [0xf]
