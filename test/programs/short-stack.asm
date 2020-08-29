mov 0xf
psh
mov 0xff
psh
mov 0xa
swp
pop
add
psh
psh

; psh

pop
pop
pop

; pop

hlt

stack:
    data [3]
