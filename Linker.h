/*
 * Link addresses and pointers/labels for CS4200 at UCCS, Fall 2021
 * Written by Matthew Hileman <mhileman@uccs.edu>
 * Date Last Updated: 25 September 2021
 *
 *  PURPOSE: Link the labels from RISCV_32I_Assembler
 */

#ifndef LINKER_H_
#define LINKER_H_

#include <stdint.h>
#include <stdio.h>
#include "parser.h"
#include "RISCV_32I_Assembler.h"

#define DATA_ADDRESS 0x10000000
#define TEXT_ADDRESS 0x00400000



void link_program(struct sAssembledProgram program, uint8_t *data_segment, uint8_t *text_segment);






//accepts text & data segments
// assembled program








#endif
