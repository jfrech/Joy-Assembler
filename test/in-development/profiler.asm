pragma_embed-profiler-output := true

mov 0
mov 1
profiler start, one instruction
nop
profiler stop, one instruction

profiler start, loop test
mov 4
loop:
    dec
    jnz @loop
profiler stop, loop test

hlt
