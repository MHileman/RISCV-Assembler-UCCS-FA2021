# simple makefile

all: mas

mas: parser.c parser.h writer.c writer.h RISCV_32I_Assembler.h RISCV_32I_Assembler.c main.c Linker.h Linker.c
	gcc -O2 parser.c writer.c RISCV_32I_Assembler.c Linker.c main.c -o mas

clean:
	rm mas
