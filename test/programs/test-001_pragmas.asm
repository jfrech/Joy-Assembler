pragma_memory-size := 0xff
pragma_rng-seed := 0x5eed

jmp @main

puts:
    lsa -8

    puts$loop:
        sta @puts$string-ptr

        swp
        lia
        jz @puts$return
        ptc

        lda @puts$string-ptr
        inc 4

        jmp @puts$loop

    puts$return:
        mov '\n'
        ptc
        ret

    puts$string-ptr:
        data [1]0x00000000

main:
    mov 1
    rnd
    mov 1
    rnd
    jz @tails
    jnz @heads

    tails:
        mov @string-tails
        psh
        cal @puts
        pop
        jmp @hlt

    heads:
        mov @string-heads
        psh
        cal @puts
        pop
        jmp @hlt

    hlt:
        hlt

string-heads:
    data "heads\0"
string-tails:
    data "tails\0"

stack:
    data [2]0
