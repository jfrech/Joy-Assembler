; Jonathan Frech, 26th of August 2020
; a Joy Assembler example program demonstrating byte code self-modification

jmp @main

benevolent:
    mov 0x266b
    ptc
    jmp @end

main:
    jmp @dubious
    dubious-back:
        mov 0
        nop 0xdddddddd
        jnz @malicious
        jmp @benevolent

dubious:
    ;jmp @dubious-back
    mov 12
    sya 35
    jmp @dubious-back

malicious:
    mov 0x2620
    ptc

end:
    mov 0x0a
    ptc
    hlt
