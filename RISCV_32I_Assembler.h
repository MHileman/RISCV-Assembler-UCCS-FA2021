/*
 * Assemble Instructions and Data for CS4200 at UCCS, Fall 2021
 * Written by Matthew Hileman <mhileman@uccs.edu>
 * Date Last Updated: 25 September 2021
 *
 *  PURPOSE: Assemble 32-bit instruction set for RISCV_32I Architecture (.text)
             Assemble data for RISV_32I Architecture (.data)
 */

#ifndef RISCV_32I_ASSEMBLER_H_
#define RISCV_32I_ASSEMBLER_H_

#include <stdint.h>
#include <stdio.h>
#include "parser.h"


enum eLinkerCode {
  LINKER_NONE = 0,
  LINKER_JAL = 1,
  LINKER_BRANCH = 2,
  LINKER_LA_AUIPC = 3,
  LINKER_LA_ADDI = 4,
  LINKER_ALIGN = 5,
  LINKER_SPACE = 6,
};


struct sAssembledData{
  uint8_t *data;
  size_t data_len;
  char *label;
  uint32_t arg_n;
  enum eLinkerCode linker_code;
  struct sAssembledData *next;
};

struct sAssembledInstruction {
  uint32_t binary;            // Assembled binary instruction
  char *target_label;         // Target label for b/j instructions (null others)
  char *label;                // Label of present instruction (null if has none)
  enum eLinkerCode linker_code;
  struct sAssembledInstruction *next;
};

struct sAssembledProgram {
  struct sAssembledData *data;
  struct sAssembledInstruction *text;
};

struct sAssembledProgram assemble_program(struct line *line);
void free_instructions(struct sAssembledInstruction *instructions);
void free_data(struct sAssembledData *data);

void _bind_imm_j_type(uint32_t *instr, uint32_t immediate);
void _bind_imm_b_type(uint32_t *instr, uint32_t immediate);
void _bind_imm_i_type(uint32_t *instr, uint32_t immediate);
void _bind_imm_u_type(uint32_t *instr, uint32_t immediate);

#endif /* RISCV_32I_ASSEMBLER_H_ */
