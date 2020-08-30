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
