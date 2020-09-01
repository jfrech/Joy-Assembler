nop

jmp @main

garbage:
    data 0xabbccddd

main:
    mov @primes
    psh
    cal @f
    cal @g
    pop

f:
    ssa -8
    swp
    lia +8
    swp
    mov -5
    add
    jz @works
    hlt
    works:
    inc
    jnz @works2
    hlt
    works2:
    jmp @hlt
    ret

g:
    ret

hlt:
    nop 0x11111111
    hlt

primes:
    data 2, 3, 5, 7, 11 ; ...

stack:
    data [0xff]

