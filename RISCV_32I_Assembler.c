/*
 * Assemble Instructions and Data for CS4200 at UCCS, Fall 2021
 * Written by Matthew Hileman <mhileman@uccs.edu>
 * Date Last Updated: 25 September 2021
 *
 *  PURPOSE: Assemble 32-bit instruction set for RISCV_32I Architecture (.text)
             Assemble data for RISV_32I Architecture (.data)
 */

#include "RISCV_32I_Assembler.h"


#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
----------------------------------
----------- PROTOTYPES -----------
----------------------------------
  Binds are nearly constant methods for dealing with different formats.
  Everything is private to follow conventions, starting with _ means private.
*/

struct sArgArray {
  char** args;
  size_t len;
};

enum eAssemblerState {
  S_UNCLASSIFIED = 0,
  S_DATA = 1,
  S_TEXT = 2,
};

static struct sAssembledInstruction *_instruction_to_binary(struct sArgArray struct_args);
static struct sAssembledInstruction *_psuedo_to_binary(struct sArgArray struct_args);
static struct sAssembledData *_data_to_binary(struct sArgArray struct_args, int data_type);

static int _check_psuedo(char *opName);

static void _assemble_r_type(uint32_t *binary,
  struct sArgArray struct_args, uint32_t opcode,
  uint32_t funct3, uint32_t funct7);
static void _assemble_i_type(uint32_t *binary,
  struct sArgArray struct_args, uint32_t opcode, uint32_t funct3);

static void _bind_opcode(uint32_t *instr, uint32_t opcode);
static void _bind_rd(uint32_t *instr, uint32_t rd);
static void _bind_funct3(uint32_t *instr, uint32_t funct3);
static void _bind_rs1(uint32_t *instr, uint32_t rs1);
static void _bind_rs2(uint32_t *instr, uint32_t rs2);
static void _bind_funct7(uint32_t *instr, uint32_t funct7);

static void _bind_imm_s_type(uint32_t *instr, uint32_t immediate);
static void _bind_shamt_srai(uint32_t *instr, uint32_t shamt);
static void _bind_shamt_i_type(uint32_t *instr, uint32_t shamt);


static uint32_t _sign_reduce(uint32_t value, int width);

static int _get_imm(char *imm_str);
static int _get_imm_and_ptr(char *imm_str, char **ptr);
static int _get_reg(char *reg_name);
static char *_get_arg(struct token_node *parts, int index);
static struct sArgArray _token_list_to_array(struct token_node *argument);

/*
----------------------------------
------------ DEFINES -------------
----------------------------------
Includes:
  -Registers and their abi names.
  -Psuedo instructions.
*/
#define NUM_PSUEDO_INSTS (8)
char *psuedo_instructions[NUM_PSUEDO_INSTS] = {
  "j",
  "la",
  "li",
  "mv",
  "neg",
  "nop",
  "not",
  "ret"
};

#define NUM_REGS (32)
char *abi_registers[NUM_REGS] = {
  "zero",
  "ra",
  "sp",
  "gp",
  "tp",
  "t0",
  "t1",
  "t2",
  "s0",
  "s1",
  "a0",
  "a1",
  "a2",
  "a3",
  "a4",
  "a5",
  "a6",
  "a7",
  "s2",
  "s3",
  "s4",
  "s5",
  "s6",
  "s7",
  "s8",
  "s9",
  "s10",
  "s11",
  "t3",
  "t4",
  "t5",
  "t6",
};

char *registers[NUM_REGS] = {
  "x0",
  "x1",
  "x2",
  "x3",
  "x4",
  "x5",
  "x6",
  "x7",
  "x8",
  "x9",
  "x10",
  "x11",
  "x12",
  "x13",
  "x14",
  "x15",
  "x16",
  "x17",
  "x18",
  "x19",
  "x20",
  "x21",
  "x22",
  "x23",
  "x24",
  "x25",
  "x26",
  "x27",
  "x28",
  "x29",
  "x30",
  "x31",
};





/*
----------------------------------
---------- MAIN METHODS ----------
----------------------------------
  The main framework, which directs the logic of the assembler.
*/


struct sAssembledProgram assemble_program(struct line *line){

  // Assign state to unclassified by defult
  enum eAssemblerState state = S_UNCLASSIFIED;
  struct sAssembledInstruction *head_instruction = NULL;
  struct sAssembledInstruction *curr_instruction = NULL;
  struct sAssembledInstruction *prev_instruction = NULL;
  struct sAssembledData *head_data = NULL;
  struct sAssembledData *curr_data = NULL;
  struct sAssembledData *prev_data = NULL;

  for(; line != NULL; line = line->next){

    // STATE MACHINE
    if (line->type == DATA){
      state = S_DATA;
      continue;
    }
    if (line->type == TEXT){
      state = S_TEXT;
      continue;
    }


    // Skip lines that don't fall under data or text
    if (state == S_UNCLASSIFIED)
      continue;

    //get argument array from token list
    struct sArgArray struct_args = _token_list_to_array(line->token_listhead);

    if (state == S_DATA){

      // TODO-Error/warning, not data type
      if (line->type > WORD){
        continue;
      }

      // _data_to_binary returns a sAssembledData struct that is put into curr_data
      curr_data = _data_to_binary(struct_args, line->type);

      // START CASES
      if (curr_data == NULL){
        continue;
      }
      //set label if has a label.
      if (line->label != NULL){
        curr_data->label = line->label;
      }

      //  LINKED LIST CONSTRUCTION
      if (head_data == NULL)
        head_data = curr_data;
      if (prev_data != NULL)

        prev_data->next = curr_data;

      prev_data = curr_data;


    } else if (state == S_TEXT){

      // TODO-Error/warning, not instruction
      if (line->type < INST){
        continue;
      }

      // CHECKS IF PSUEDO OR REGULAR OPERATION
      char *opName = struct_args.args[0];
      if(_check_psuedo(opName)){
        curr_instruction = _psuedo_to_binary(struct_args);
      } else {
        curr_instruction = _instruction_to_binary(struct_args);
      }

      // START CASES
      if (curr_instruction == NULL){
        continue;
      }
      if (line->label != NULL){
        curr_instruction->label = line->label;
      }

      //  LINKED LIST CONSTRUCTION
      if(head_instruction == NULL){
        head_instruction = curr_instruction;
      }
      if(prev_instruction != NULL){
        prev_instruction->next = curr_instruction;
      }

      //  Move to the last element of the linked list (this is for psuedo)
      //  Multiple instructions could be returned in a list
      for (; curr_instruction->next != NULL; curr_instruction = curr_instruction->next);
      prev_instruction = curr_instruction;


    }// S_TEXT

    free(struct_args.args);
  }// for

  return (struct sAssembledProgram){.data = head_data, .text = head_instruction};

}

/*[[ DATA TO BINARY]]
  converts anything in the data section to its binary representation.
  is like _instruction_to_binary, except for the data segment instead of text.
  */
static struct sAssembledData *_data_to_binary(struct sArgArray struct_args, int data_type){

  struct sAssembledData *assembled_data =
    calloc(1, sizeof(struct sAssembledData));
  assert(assembled_data != NULL);

  switch(data_type){
    case ALIGN:{
      assembled_data->arg_n = _get_imm(struct_args.args[1]);
      assembled_data->linker_code = LINKER_ALIGN;
      break;
    }

    case ASCIIZ:{

      char *str = (char*)malloc(strlen(struct_args.args[1]) + 1);
      strcpy(str, struct_args.args[1]);
      str = strtok(str, "\"");
      assembled_data->data = (uint8_t*)str;
      assembled_data->data_len = strlen(str) + 1;

      break;
    }

    case SPACE:{
      assembled_data->arg_n = _get_imm(struct_args.args[1]);
      assembled_data->linker_code = LINKER_SPACE;
      break;

    }
    case WORD:{
      assembled_data->data = (uint8_t*)malloc((struct_args.len - 1 ) * 4);
      assembled_data->data_len = (struct_args.len - 1) * 4;
      for (int i = 0; i < (struct_args.len - 1) * 4; i += 4){
        uint32_t value = (uint32_t)_get_imm(struct_args.args[i / 4 + 1]);
        assembled_data->data[i] = (uint8_t)(value >> 0);
        assembled_data->data[i + 1] = (uint8_t)(value >> 8);
        assembled_data->data[i + 2] = (uint8_t)(value >> 16);
        assembled_data->data[i + 3] = (uint8_t)(value >> 24);
      }
      break;
    }

  }//switch
  return assembled_data;
}

/*[[ INSTRUCTION TO BINARY]]
  first finds out what instruction was used, then proceeds to
  assemble using method sub-method calls to type based ordering.
  */
static struct sAssembledInstruction *_instruction_to_binary(struct sArgArray struct_args){

  struct sAssembledInstruction *assembled_instruction =
    calloc(1, sizeof(struct sAssembledInstruction));
  assert(assembled_instruction != NULL);

  // Get operation name (such as add) from the parsed in line (parser.c)
  char *opName = struct_args.args[0];
  // Default link is NULL
  assembled_instruction->target_label = NULL;

  /*------------ R-Type -------------*/
  // numbers are: opcode, funct3, funct7
  if( strcmp(opName, "add") == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x0, 0x00);
  } else if( strcmp(opName, "sub" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x0, 0x20);
  } else if( strcmp(opName, "xor" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x4, 0x00);
  } else if( strcmp(opName, "or" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x6, 0x00);
  } else if( strcmp(opName, "and" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x7, 0x00);
  } else if( strcmp(opName, "sll" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x1, 0x00);
  } else if( strcmp(opName, "srl" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x5, 0x00);
  } else if( strcmp(opName, "sra" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x5, 0x20);
  } else if( strcmp(opName, "slt" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x2, 0x00);
  } /*else if( strcmp(opName, "sltu" ) == 0 ){
    _assemble_r_type(&assembled_instruction->binary, struct_args, 0x33, 0x3, 0x00);
  }
  */

    /*------------ I-Type -------------*/
    // numbers are: opcode, funct3
    else if( strcmp(opName, "addi" ) == 0 ){
    _assemble_i_type(&assembled_instruction->binary, struct_args, 0x13, 0x0);
  } else if( strcmp(opName, "xori" ) == 0 ){
    _assemble_i_type(&assembled_instruction->binary, struct_args, 0x13, 0x4);
  } else if( strcmp(opName, "ori" ) == 0 ){
    _assemble_i_type(&assembled_instruction->binary, struct_args, 0x13, 0x6);
  } else if( strcmp(opName, "andi" ) == 0 ){
    _assemble_i_type(&assembled_instruction->binary, struct_args, 0x13, 0x7);
  }
  else if( strcmp(opName, "slti" ) == 0 ){
    _assemble_i_type(&assembled_instruction->binary, struct_args, 0x13, 0x2);
  }

  /*--------- I-Type Specials ----------*/
  // These require some work that cannot be called using the regular methods
  else if( strcmp(opName, "slli" ) == 0 ){
    char *rd = struct_args.args[1];    //  1st argument
    char *rs1 = struct_args.args[2];   //  2nd argument
    char *imm = struct_args.args[3];   //  immediate

    _bind_opcode(&assembled_instruction->binary, 0x13);             // 0010011
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_funct3(&assembled_instruction->binary, 0x1);
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    // passing to its own special immediate binder
    _bind_shamt_i_type(&assembled_instruction->binary, (uint32_t)_get_imm(imm));
  }
  else if( strcmp(opName, "srli" ) == 0 ){
    char *rd = struct_args.args[1];    //  1st argument
    char *rs1 = struct_args.args[2];   //  2nd argument
    char *imm = struct_args.args[3];   //  immediate

    _bind_opcode(&assembled_instruction->binary, 0x13);             // 0010011
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_funct3(&assembled_instruction->binary, 0x5);
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    // passing to its own special immediate binder
    _bind_shamt_i_type(&assembled_instruction->binary, (uint32_t)_get_imm(imm));
  }
  else if( strcmp(opName, "srai" ) == 0 ){
    char *rd = struct_args.args[1];    //  ret argument
    char *rs1 = struct_args.args[2];   //  2nd argument
    char *imm = struct_args.args[3];   //  immediate

    _bind_opcode(&assembled_instruction->binary, 0x13);             // 0010011
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_funct3(&assembled_instruction->binary, 0x5);
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    // passing to its own special immediate binder
    _bind_shamt_srai(&assembled_instruction->binary, (uint32_t)_get_imm(imm));
  }

  /*------------ I-Type Loads -------------*/
  // Loads (such as load word) require extra work to parse.

  else if( strcmp(opName, "lw" ) == 0 ){
    char *rd = struct_args.args[1];
    char *rs1;
    int imm = _get_imm_and_ptr(struct_args.args[2], &rs1);

    _bind_opcode(&assembled_instruction->binary, 0x3);             // 010011
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_funct3(&assembled_instruction->binary, 0x2);
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    _bind_imm_i_type(&assembled_instruction->binary, (uint32_t)imm);
  }


  /*------------ S-Type Jump -------------*/
  else if( strcmp(opName, "sw" ) == 0 ){
    char *rs1 = struct_args.args[1];
    char *rs2;
    int imm = _get_imm_and_ptr(struct_args.args[2], &rs2);

    _bind_opcode(&assembled_instruction->binary, 0x23);             // 010011
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    _bind_funct3(&assembled_instruction->binary, 0x2);
    _bind_rs2(&assembled_instruction->binary, _get_reg(rs2));
    _bind_imm_s_type(&assembled_instruction->binary, (uint32_t)imm);
  }

  /*------------ B-Type Jump -------------*/
  else if( strcmp(opName, "beq" ) == 0 ){
    char *rs1 = struct_args.args[1];    //  1st argument
    char *rs2 = struct_args.args[2];   //  2nd argument
    char *target = struct_args.args[3];   //  target

    _bind_opcode(&assembled_instruction->binary, 0x63);
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    _bind_rs2(&assembled_instruction->binary, _get_reg(rs2));
    _bind_funct3(&assembled_instruction->binary, 0x0);
    assembled_instruction->target_label = target;
    assembled_instruction->linker_code = LINKER_BRANCH;

  }
  else if( strcmp(opName, "bne" ) == 0 ){
    char *rs1 = struct_args.args[1];      //  1st argument
    char *rs2 = struct_args.args[2];      //  2nd argument
    char *target = struct_args.args[3];   //  target

    if (target == NULL){
      // error function
    }

    _bind_opcode(&assembled_instruction->binary, 0x63);
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    _bind_rs2(&assembled_instruction->binary, _get_reg(rs2));
    _bind_funct3(&assembled_instruction->binary, 0x1);
    assembled_instruction->target_label = target;
    assembled_instruction->linker_code = LINKER_BRANCH;
  }

  /*------------ I-Type Jump -------------*/
  // jalr is the only I-type jump.
  else if( strcmp(opName, "jalr" ) == 0 ){
    char *rd = struct_args.args[1];
    char *rs1;
    int imm = _get_imm_and_ptr(struct_args.args[2], &rs1);

    _bind_opcode(&assembled_instruction->binary, 0x67);             // 010011
    _bind_rs1(&assembled_instruction->binary, _get_reg(rs1));
    _bind_funct3(&assembled_instruction->binary, 0x0);
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_imm_i_type(&assembled_instruction->binary, (uint32_t)imm);
  }

  // J-Type
  else if( strcmp(opName, "jal" ) == 0 ){

    char *arg1 = struct_args.args[1];      //  ret argument
    char *arg2 = struct_args.args[2];  //  target


    _bind_opcode(&assembled_instruction->binary, 0x6F);
    // If jal has no target
    if (arg2 == NULL){
      _bind_rd(&assembled_instruction->binary, _get_reg("ra"));
      assembled_instruction->target_label = arg1;
    } else {
      _bind_rd(&assembled_instruction->binary, _get_reg(arg1));
      assembled_instruction->target_label = arg2;
    }
    assembled_instruction->linker_code = LINKER_JAL;
  }

  // U-Type
  else if( strcmp(opName, "lui" ) == 0 ){

    char *rd = struct_args.args[1];      //  rd
    char *imm = struct_args.args[2];     //  imm

    _bind_opcode(&assembled_instruction->binary, 0x37);             // 010011
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_imm_u_type(&assembled_instruction->binary, (uint32_t)_get_imm(imm));

  } else if( strcmp(opName, "auipc" ) == 0 ){
    char *rd = struct_args.args[1];      //  rd
    char *imm = struct_args.args[2];     //  imm

    _bind_opcode(&assembled_instruction->binary, 0x17);             // 010011
    _bind_rd(&assembled_instruction->binary, _get_reg(rd));
    _bind_imm_u_type(&assembled_instruction->binary, (uint32_t)_get_imm(imm));

  } else {
    // TODO: unrecognized instruction
  }

  return assembled_instruction;
} // _instruction_to_binary() end


/*[[ PSUEDO INSTRUCTION TO BINARY ]]
  first finds out what psuedo instruction was used, then proceeds to
  assemble using each part and calls to _instruction_to_binary().
*/
static struct sAssembledInstruction *_psuedo_to_binary(struct sArgArray struct_args){

  struct sAssembledInstruction *assembled_instruction =
    calloc(1, sizeof(struct sAssembledInstruction));
  assert(assembled_instruction != NULL);
  // Default link is NULL
  assembled_instruction->target_label = NULL;

  // Get operation name (such as add) from the parsed in line
  char *psuedoName = struct_args.args[0];

  /*------------ PSUEDO-Type -------------*/
  if( strcmp(psuedoName, "j") == 0 ){
    char *array[] = {"jal", "x0", struct_args.args[1]};
    struct sArgArray args = {array, 3};
    assembled_instruction = _instruction_to_binary(args);

  } else if( strcmp(psuedoName, "la") == 0 ){
    char *array[] = {"auipc", struct_args.args[1], "0"};
    struct sArgArray args = {array, 3};
    assembled_instruction = _instruction_to_binary(args);
    assembled_instruction->linker_code = LINKER_LA_AUIPC;
    assembled_instruction->target_label = struct_args.args[2];

    char *array2[] = {"addi", struct_args.args[1], struct_args.args[1], "0"};
    struct sArgArray args2 = {array2, 4};
    assembled_instruction->next = _instruction_to_binary(args2);
    assembled_instruction->next->linker_code = LINKER_LA_AUIPC;
    assembled_instruction->next->target_label = struct_args.args[2];

  } else if( strcmp(psuedoName, "li") == 0 ){

    char *array[] = {"addi", struct_args.args[1], "x0", struct_args.args[2]};
    struct sArgArray args = {array, 4};
    assembled_instruction = _instruction_to_binary(args);

    //If over 12 bits, only 1 instruction
    uint32_t imm = (uint32_t)_get_imm(struct_args.args[2]) >> 12;
    if(imm){

      char *array2[] = {"lui", struct_args.args[1], struct_args.args[2]};
      struct sArgArray args2 = {array2, 3};
      assembled_instruction->next = _instruction_to_binary(args2);
    }


  } else if( strcmp(psuedoName, "mv") == 0 ){
    char *array[] = {"addi", struct_args.args[1], struct_args.args[2], "0"};
    struct sArgArray args = {array, 4};
    assembled_instruction = _instruction_to_binary(args);

  } else if( strcmp(psuedoName, "neg") == 0 ){
    char *array[] = {"sub", struct_args.args[1], "x0", struct_args.args[2]};
    struct sArgArray args = {array, 4};
    assembled_instruction = _instruction_to_binary(args);

  } else if( strcmp(psuedoName, "nop") == 0 ){
    char *array[] = {"addi", "x0", "x0", "0"};
    struct sArgArray args = {array, 4};
    assembled_instruction = _instruction_to_binary(args);

  } else if( strcmp(psuedoName, "ret") == 0 ){
    char *array[] = {"jalr", "x0", "x1", "0"};
    struct sArgArray args = {array, 4};
    assembled_instruction = _instruction_to_binary(args);
  }

  return assembled_instruction;
}// _psuedo_to_binary() end

/*
----------------------------------
------- SUPPORTING METHODS -------
----------------------------------
  Repetative calls and code goes here
*/


/*------------ R-Type ASSEMBLE -------------*/
static void _assemble_r_type(uint32_t *binary,
  struct sArgArray struct_args, uint32_t opcode,
  uint32_t funct3, uint32_t funct7){

  char *rd = struct_args.args[1];    //  1st argument
  char *rs1 = struct_args.args[2];   //  2nd argument
  char *rs2 = struct_args.args[3];   //  3rd argument
  char *opName = struct_args.args[0];

  _bind_opcode(binary, opcode);   // 011001
  _bind_rd(binary, _get_reg(rd));
  _bind_funct3(binary, funct3);
  _bind_rs1(binary, _get_reg(rs1));
  _bind_rs2(binary, _get_reg(rs2));
  _bind_funct7(binary, funct7);
}


/*------------ I-Type ASSEMBLE -------------*/
static void _assemble_i_type(uint32_t *binary,
  struct sArgArray struct_args, uint32_t opcode, uint32_t funct3){

  char *rd = struct_args.args[1];    //  1st argument
  char *rs1 = struct_args.args[2];   //  2nd argument
  char *imm = struct_args.args[3];   //  immediate

  _bind_opcode(binary, opcode);             // 010011
  _bind_rd(binary, _get_reg(rd));
  _bind_funct3(binary, funct3);
  _bind_rs1(binary, _get_reg(rs1));
  _bind_imm_i_type(binary, (uint32_t)_get_imm(imm));
}


// Free the callocs of sAssembledInstruction
void free_instructions(struct sAssembledInstruction *instruction){
  struct sAssembledInstruction *next;
  while (instruction != NULL){
    next = instruction->next;
    free(instruction);
    instruction = next;
  }
}

void free_data(struct sAssembledData *data){
  struct sAssembledData *next;
  while (data != NULL){
    next = data->next;
    free(data);
    data = next;
  }
}

/*------------ Convert Line Struct to Array -------------*/
static struct sArgArray _token_list_to_array(struct token_node *argument){

  // count number of arguments in struct
  int num_args = 0;
  struct token_node *next = argument;
  while (next != NULL){
    num_args++;
    next = next->next;
  }

  // Create an array of strings from the tokenized list.
  char ** arg_array = (char**)malloc(num_args * sizeof(char*));
  for (int i = 0; i < num_args; i++){
    arg_array[i] = argument->token;
    argument = argument->next;
  }

  // return a special instruction type sArgArray
  return (struct sArgArray){.args = arg_array, .len = num_args};
}



/*------------ Check for Psuedo Instr -------------*/
// returns -1 if not a psuedo instruction
// othereise, returns index from psuedo list
static int _check_psuedo(char *opName){
  for (int i = 0; i < NUM_PSUEDO_INSTS; i++){
    if(!strcmp(opName, psuedo_instructions[i])){
      return 1;
    }
  }
  return 0;
}



/*------------ Tools and Standards -------------*/
// Converts the string immidiate to numerical format (int), and pointer
static int _get_imm_and_ptr(char *imm_str, char **ptr){
  if (strncmp("0x", imm_str, 2) == 0)
    return strtol(&imm_str[2], ptr, 16);
  //binary
  if (strncmp("0b", imm_str, 2) == 0)
    return strtol(&imm_str[2], ptr, 2);
  //decimal by default
  return strtol(imm_str, ptr, 10);
}

// Converts the string immidiate to numerical format (int)
static int _get_imm(char *imm_str){
  char *ptr;
  //hex
  if (strncmp("0x", imm_str, 2) == 0)
    return strtol(&imm_str[2], &ptr, 16);
  //binary
  if (strncmp("0b", imm_str, 2) == 0)
    return strtol(&imm_str[2], &ptr, 2);
  //decimal by default
  return strtol(imm_str, &ptr, 10);
}

// gets argument in the parsed line
// index is argument location
static char *_get_arg(struct token_node *parts, int index){
  for (int i = 0; i < index; i++){
    if (parts->next == NULL)
      return NULL;
    parts = parts->next;
  }
  return parts->token;
}

// get numerial representation of a register
// !IMOPRTANT NOTE: I must be decremented here as strstr() needs it:
//   -> strstr() could get x1 instead of x11 first, and thus ruin the register lookup.
static int _get_reg(char *reg_name){
  for (int i = NUM_REGS-1; i >= 0; i--){
    // if ( !strcmp(reg_name, registers[i]) || !strcmp(reg_name, abi_registers[i]) ){
    if ( strstr(reg_name, registers[i]) != NULL || strstr(reg_name, abi_registers[i]) != NULL ){
      return i;
    }
  }
  return -1;
}



/*
----------------------------------
---------- BIND METHODS ----------
----------------------------------
  "Constant" methods that deal with shifts and locations.
*/

/*------------ Standard -------------*/
// OPCODE (0:6)
static void _bind_opcode(uint32_t *instr, uint32_t opcode){
  *instr |= (uint32_t)(opcode & 0x7F);
}
// RD (7:11)
static void _bind_rd(uint32_t *instr, uint32_t rd){
  *instr |= (uint32_t)(rd & 0x1F) << 7;
}
// funct3 (12:14)
static void _bind_funct3(uint32_t *instr, uint32_t funct3){
  *instr |= (uint32_t)(funct3 & 0x7) << 12;
}
// rs1 (15:19)
static void _bind_rs1(uint32_t *instr, uint32_t rs1){
  *instr |= (uint32_t)(rs1 & 0x1F) << 15;
}
// rs2 (20:24)
static void _bind_rs2(uint32_t *instr, uint32_t rs2){
  *instr |= (uint32_t)(rs2 & 0x1F) << 20;
}
// funct7 (25:31)
static void _bind_funct7(uint32_t *instr, uint32_t funct7){
  *instr |= (uint32_t)(funct7 & 0x7F) << 25;
}


/*------------ Immediates -------------*/
// immediate-I-type
void _bind_imm_i_type(uint32_t *instr, uint32_t immediate){
  immediate = _sign_reduce(immediate, 12);
  *instr |= (uint32_t)(immediate & 0xFFF) << 20;
}
// Immediate special for slli, srli
static void _bind_shamt_i_type(uint32_t *instr, uint32_t shamt){
  *instr |= (uint32_t)(shamt & 0x1F) << 20;
}
// Immediate special for srai
static void _bind_shamt_srai(uint32_t *instr, uint32_t shamt){
  shamt &= 0x1F;
  shamt |= 0x20;          //  [5:11] = 0x20, reveresed so is 0x40
  *instr |= shamt << 20;
}
// immediate-S-type (4:0 & 11:5)
static void _bind_imm_s_type(uint32_t *instr, uint32_t immediate){
  immediate = _sign_reduce(immediate, 12);
  uint32_t im11_5, im4_0;
  im4_0 = immediate & 0x1F;
  im11_5 = (immediate >> 5) & 0x7F;
  *instr |= (im4_0 << 7) | (im11_5 << 25);
}
// immediate-B-type
void _bind_imm_b_type(uint32_t *instr, uint32_t immediate){
  immediate = _sign_reduce(immediate, 12);
  uint32_t im4_1, im11, im12, im10_5;
  im4_1 = (immediate >> 1) & 0xF;
  im10_5 = (immediate >> 5) & 0x3F;
  im11 = (immediate >> 11) & 1;
  im12 = (immediate >> 12) & 1;
  *instr |= (im11 << 7) | (im4_1 << 8) | (im10_5 << 25) | (im12 << 31);
}
// immediate-U-type
void _bind_imm_u_type(uint32_t *instr, uint32_t immediate){
  // last 12 are = 0
  *instr |= immediate & 0xFFFFF000;
}
// immediate-J-type (20|10:1|11|19:12)
void _bind_imm_j_type(uint32_t *instr, uint32_t immediate){
  immediate = _sign_reduce(immediate, 21);
  uint32_t im20, im10_1, im11, im19_12;
  im10_1 = (immediate >> 1) & 0x3FF;
  im19_12 = (immediate >> 12) & 0xFF;
  im11 = (immediate >> 11) & 1;
  im20 = (immediate >> 20) & 1;
  *instr |= (im19_12 << 12) | (im11 << 20) | (im10_1 << 21) | (im20 << 31);
}


/*------------ Tools and Other -------------*/
// reduce the sign of a 32 signed into given width signed
static uint32_t _sign_reduce(uint32_t value, int width){
  uint32_t sign = value >> 30;
  uint32_t mask = (1 << (width - 1)) - 1;
  return ( value & mask | sign << (width - 1) );
}
