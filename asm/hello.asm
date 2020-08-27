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
	string "Hello, World!"

stack:
    data[0x20] 0
