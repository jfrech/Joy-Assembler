; Joy Assembler IO test

addr_x := prg+0

;jmp @io-c

gtu
swp
gtu
sta addr_x
gts
lda addr_x
add
ptu


gtb
ptb

hlt

io-c:
    gtc
    swp
    gtc
    swp
    ptc

hlt
