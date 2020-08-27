mov @stack
lsc

mov @hello
cal @print

hlt

; function print
; register a = pointer to 0-terminated string
print:
	swp
	lia
	jz @printend
	ptc
	swp
	inc 4
	jmp @print
printend:
	ret

hello:
	data "Hellö, World! (Unicode: \u1e9E \u2713)\n\0"

stack:
    data[0x20] 0
