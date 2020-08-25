jmp @main

var:
    uint[4] 0x3e

main:
    lia @var
    ptc
    mov 0xff
    sia prg+0
    hlt
