/*
 * Link addresses and pointers/labels for CS4200 at UCCS, Fall 2021
 * Written by Matthew Hileman <mhileman@uccs.edu>
 * Date Last Updated: 25 September 2021
 *
 *  PURPOSE: Link the labels from RISCV_32I_Assembler
 */

 #include "Linker.h"


 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>


 struct sLinkedLabel {
   char *label;
   uint32_t address;
   struct sLinkedLabel *next;
 };



 /*
 ----------------------------------
 ---------- LINK PROGRAM ----------
 ----------------------------------
 */
void link_program(struct sAssembledProgram program, uint8_t *data_segment, uint8_t *text_segment){


  uint32_t address = DATA_ADDRESS;
  struct sLinkedLabel *head_label = NULL;
  struct sLinkedLabel *tail_label = NULL;

  /*------------ Data-Loop -------------*/
  for (struct sAssembledData *data_node = program.data; data_node != NULL; data_node = data_node->next){

    // increase address by data length
    // (only for word and ascii at the moment)
    address += data_node->data_len;

    // increase address for align and space
    uint32_t n = data_node->arg_n;
    if (data_node->linker_code == LINKER_ALIGN){
      uint32_t rem = address % (1<<n);
      if (rem > 0)
        address += (1<<n) - rem;
    }
    if (data_node->linker_code == LINKER_SPACE){
      address += n;
    }


    // load data into data segment if data exists
    for (int i = 0; i < data_node->data_len; i++){
      data_segment[address - DATA_ADDRESS + i] = data_node->data[i];
    }


    // if program has a label, add to linkedlabel struct
    if (data_node->label != NULL){
      struct sLinkedLabel *linked_label = calloc(1, sizeof(struct sLinkedLabel));
      assert(linked_label != NULL);

      linked_label->label = data_node->label;
      linked_label->address = address;

      if (head_label == NULL)
        head_label = linked_label;
      if (tail_label != NULL)
        tail_label->next = linked_label;
      tail_label = linked_label;
    }

  } // data loop

  address = TEXT_ADDRESS;
  /*------------ Text-Loop -------------*/
  for (struct sAssembledInstruction *text_node = program.text; text_node != NULL; text_node = text_node->next){

    // Always a constant.
    address += 4;

    // if program has a label, add to linkedlabel struct
    if (text_node->label != NULL){
      struct sLinkedLabel *linked_label = calloc(1, sizeof(struct sLinkedLabel));
      assert(linked_label != NULL);

      linked_label->label = text_node->label;
      linked_label->address = address;

      if (head_label == NULL)
        head_label = linked_label;
      if (tail_label != NULL)
        tail_label->next = linked_label;
      tail_label = linked_label;
    }

  } // text loop

  address = TEXT_ADDRESS;
  /*------------ Last-Loop -------------*/
  for (struct sAssembledInstruction *text_node = program.text; text_node != NULL; text_node = text_node->next){
    // Always a constant.

    // link labels to targets (for j and b instructions)
    if (text_node->target_label != NULL){
      uint32_t target_address;

      for (struct sLinkedLabel *label_node = head_label; label_node != NULL; label_node = label_node->next){
        if (!strcmp(label_node->label, text_node->target_label)){
          target_address = label_node->address;
          break;
        }
      } // 2nd loop


      // relative to PC
      uint32_t relative_addr = target_address - address;
      switch (text_node->linker_code){
        case LINKER_JAL:{
          _bind_imm_j_type(&text_node->binary, relative_addr);
          break;
        }

        case LINKER_BRANCH:{
          _bind_imm_b_type(&text_node->binary, relative_addr);
          break;
        }

        case LINKER_LA_AUIPC:{
          _bind_imm_u_type(&text_node->binary, relative_addr >> 12);
          break;
        }

        case LINKER_LA_ADDI:{
          _bind_imm_i_type(&text_node->binary, relative_addr);
          break;
        }

        default:
          break;
      }
    } //if

    // load data into data segment if data exists


    text_segment[address - TEXT_ADDRESS] = (uint8_t)(text_node->binary);
    text_segment[address - TEXT_ADDRESS + 1] = (uint8_t)(text_node->binary >> 8);
    text_segment[address - TEXT_ADDRESS + 2] = (uint8_t)(text_node->binary >> 16);
    text_segment[address - TEXT_ADDRESS + 3] = (uint8_t)(text_node->binary >> 24);


    // Always a constant.
    address += 4;

  } // last loop

  struct sLinkedLabel *next = head_label;
  while (next != NULL){
    next = head_label->next;
    free(head_label);
    head_label = next;
  }

} // link_program




/*
//-------Break word into 4 bytes-------
// little indian following risc-v arch
assembled_instruction->bytes[0] = (uint8_t)(assembled_instruction->binary >> 0);
assembled_instruction->bytes[1] = (uint8_t)(assembled_instruction->binary >> 8);
assembled_instruction->bytes[2] = (uint8_t)(assembled_instruction->binary >> 16);
assembled_instruction->bytes[3] = (uint8_t)(assembled_instruction->binary >> 24);
*/
