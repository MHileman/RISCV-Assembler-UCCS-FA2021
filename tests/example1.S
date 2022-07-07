.data
myvar:   .word 5, 10, 	15

.text
_start:
	la t0, myvar
	lw t1, 0(t0)
	lw t2, 4(t0)
	add x2, t1, t2
	lw t3, 8(t1)
	bne x2, t3, _start
	nop
	ret
