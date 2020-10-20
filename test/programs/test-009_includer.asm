jmp @main

include "test-011_include-me.asm"

main:
    mov 5
    psh
    cal @f
    pop
    swp
    mov 8
    sub

    jnz @err
    mov 's'
    ptc
    mov '\n'
    ptc
    hlt

err:
    mov 'E'
    ptc
    mov '\n'
    ptc
    hlt

stack:
    data [0xff]
