.data
empty:  .space 32
myvar:   .word 5, -10, 	15
.align 4 #align to 16 bytes
astring: .asciiz "Hello, world!"

.text
_start:
	add	t0, t1, t2
	sub	t4, x11, x12
	xor t2, x11, t3
	or t1, t3, x0
	and t1, t2, t3
	sll t1, t2, t3
	srl t1, t2, t3
	sra t1, t2, t3
	slt t1, t2, t3
	addi t1, t2, 50
	xori t1, t2, 54
	ori t1, t2, 1
	andi t1, t2, 6
	slti t1, t2, 1
	slli t1, t2, 2
	srli t1, t2, 2
	srai t1, t2, 2

	lw t4, 8(t1)
	sw t5, 8(a1)
	beq	x2, t3, _next

	_next:
	bne x2, x2, _nextus

	_nextus:
	jalr x0, 0(x1)
	jal dest

	dest:
	lui x2, 12
	auipc a0, 24

	j destin

	destin:
	la t0, myvar
	li t1, 64
	mv a0, t2
	neg t1, t2
	nop
	ret
