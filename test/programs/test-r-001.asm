pragma_rng-seed := 0x5eed

jmp @proto-main

primes:
    data 2, 3, 5, 7, 11, 13, 17
proto-main:
    jmp @main
random-data:
    data [0x10] rperm
random-data-end:

main:
    cal @sum-random-data
    cal @pc
    jmp @success

success:
    mov 's'
    ptc
    mov '\n'
    ptc
    hlt

pc:
    lpc
    dec 5
    swp
    mov @pc
    sub
    jnz @failure


    mov @pc$flag
    swp
    mov 0
    sia

    mov @pc2
    spc
    pc2-ret:
    lda @pc$flag
    jz @failure
    ret

    pc$flag:
        data [1]

    pc2:
        mov 1
        sta @pc$flag
        jmp @pc2-ret

random-data-p:
    data [1]

failure:
    mov 0xffffffff
    rnd

    mov 'f'
    ptc
    mov '\n'
    ptc
    hlt

sum-random-data:
    mov @random-data
    sta @random-data-p

    mov 0
    sta @sum-random-data$sum

    sum-random-data$loop:
        mov @random-data-end
        ldb @random-data-p
        sub
        jnp @sum-random-data$loop-end

        lda @random-data-p
        swp
        lia

        swp
        lda @sum-random-data$sum
        add
        sta @sum-random-data$sum

        lda @random-data-p
        inc 4
        sta @random-data-p
        jmp @sum-random-data$loop
    sum-random-data$loop-end:

    mov 120
    ldb @sum-random-data$sum
    sub
    jnz @failure

    ret

    sum-random-data$sum:
        data [1]

stack:
    data [0xff]
