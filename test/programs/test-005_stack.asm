mov 0xfe
psh
mov 0xef
psh
pop
swp
pop
and
hlt

stack:
    data [0xff]0
