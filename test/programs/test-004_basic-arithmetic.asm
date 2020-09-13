jmp @main

main:
    mov -3
    swp
    mov -4
    add
    swp
    mov 18
    sub
    shl
    jnz @skip
        nop
    skip:
    shr
    hlt
