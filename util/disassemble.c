/*
 * Copyright (C) 2021 Regents of University of Colorado
 * Written by Gedare Bloom <gbloom@uccs.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define DEBUG

#define DATA_BEGIN (0x10000000)
#define TEXT_BEGIN (0x00400000)


/* Decode feature is not fully implemented */
char *decode(uint32_t word) {
  uint8_t opcode, funct3, funct7;

  opcode = word&0x7f;
  funct3 = (word>>12)&0x7;
  funct7 = (word>>25)&0x7f;

  switch (opcode) {
    case 0x3:
      if (funct3 == 0x2) return "lw";
      break;

    case 0x13: /* immediates */
      switch (funct3) {
        case 0x0:
          return "addi";
        case 0x1:
          if (funct7 == 0) return "slli";
          break;
        case 0x2:
          return "slti";
        case 0x4:
          return "xori";
        case 0x5:
          if (funct7 == 0) return "srli";
          break;
        case 0x6:
          return "ori";
        case 0x7:
          return "andi";
      }
      break;

    case 0x17:
      return "auipc";
      break;

    case 0x23:
      if (funct3 == 0x2) return "sw";
      break;

    case 0x33:
      switch (funct3) {
        case 0x0:
          if (funct7 == 0) return "add";
          else if (funct7 == 0x20) return "sub";
          break;
        case 0x1:
          if (funct7 == 0) return "sll";
          break;
        case 0x2:
          if (funct7 == 0) return "slt";
          break;
        case 0x4:
          if (funct7 == 0) return "xor";
          break;
        case 0x5:
          if (funct7 == 0) return "srl";
          else if (funct7 == 0x20) return "sra";
          break;
        case 0x6:
          if (funct7 == 0) return "or";
          break;
        case 0x7:
          if (funct7 == 0) return "and";
          break;
      }
      break;

    case 0x37:
      return "lui";
      break;

    case 0x63:
      switch (funct3) {
        case 0x0:
          return "beq";
        case 0x1:
          return "bne";
      }
      break;

    case 0x67:
      if (funct3 == 0) return "jalr";
      break;
      
    case 0x6F:
      return "jal";
      break;
  }

  return "IllegalInst";
}


static void read_and_print(char *infile, size_t data_words, size_t text_words)
{
  size_t count;
  FILE *in;
  uint32_t *data = malloc(sizeof(uint32_t)*data_words);
  uint32_t *text = malloc(sizeof(uint32_t)*text_words);

  assert(data != NULL);
  assert(text != NULL);

	in = fopen(infile, "r");
  assert(in);

  count = fread(data, sizeof(uint32_t), data_words, in);
  assert(count == data_words);
  count = fread(text, sizeof(uint32_t), text_words, in);
  assert(count == text_words);

  printf("\n%s:\tfile format cs4200-riscv32\n\n", infile);

  printf(".data\n");
  for (count = 0; count < data_words; count++) {
    printf("%.8X:\t%.2x %.2x %.2x %.2x\t%s\n",
        (uint32_t)(DATA_BEGIN + count*4),
        data[count] >> 24 & 0xff,
        data[count] >> 16 & 0xff,
        data[count] >> 8 & 0xff,
        data[count] >> 0 & 0xff,
        decode(data[count]));
  }
  printf("\n");

  printf(".text\n");
  for (count = 0; count < text_words; count++) {
    printf("%.8X:\t%.2x %.2x %.2x %.2x\t%s\n",
        (uint32_t)(TEXT_BEGIN + count*4),
        text[count] >> 24 & 0xff,
        text[count] >> 16 & 0xff,
        text[count] >> 8 & 0xff,
        text[count] >> 0 & 0xff,
        decode(text[count]));
  }
  printf("\n");

  fclose(in);
}

void usage(char *name)
{
	printf("Usage: %s [input program]\n\
where:\n\
\t[input program] is a file containing the program in the expected format.\n",
	 	name);
	exit(1);
}

int main( int argc, char *argv[] )
{
	if ( argc < 2 ) usage(argv[0]);
  read_and_print(argv[1], 1024, 1024);
	return 0;
}
