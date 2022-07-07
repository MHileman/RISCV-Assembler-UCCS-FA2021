
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "writer.h"
#include "RISCV_32I_Assembler.h"
#include "Linker.h"

static void usage(char *name)
{
  printf("Usage: %s [input source]\n\
where:\n\
\t[input source] is a file containing assembly source code.\n\
", name);
  exit(1);
}


int main( int argc, char *argv[] )
{
  //First item in the linked list of "Line" structures
  struct line* llh;
  uint32_t *text_segment, *data_segment;
  size_t prog_sz;

  // exit if arguments not enough
  if ( argc < 2 ) usage(argv[0]);

  // line header
  llh = get_lines(argv[1]);
  if (!llh) {
    fprintf(stderr, "Error getting the lines of file: %s\n", argv[1]);
    exit(1);
  }

  //print_lines(llh);

  // allocate 32 bits (4 bytes) - 4 KiB of code/data
  // text = instructions, data = data used by instructions
  data_segment = malloc(sizeof(uint32_t)*DATA_SEGMENT_WORDS);
  text_segment = malloc(sizeof(uint32_t)*TEXT_SEGMENT_WORDS);

  // operating system failed to give memeory
  if (data_segment == NULL || text_segment == NULL) {
    fprintf(stderr, "Uh oh, looks like we ran out of memory!\n");
    exit(1);
  }

  //error
  struct sAssembledProgram program = assemble_program(llh);
  link_program(program, (uint8_t*)data_segment, (uint8_t*)text_segment);

  printf("%s\n", "DATA:");
  for (struct sAssembledData *current = program.data; current != NULL; current = current->next){
    printf("%s\t", current->label);
    for (int i = 0; i < current->data_len; i++){
      printf("%02x\t", current->data[i]);
    }
    printf("\n");
  }

printf("%s\n", "TEXT:");
  for (struct sAssembledInstruction *current = program.text; current != NULL; current = current->next){
    printf("%s\t%08x\n", current->label, current->binary);
  }

  prog_sz = write_program("a.mxe", text_segment, data_segment);
  assert(prog_sz == DATA_SEGMENT_WORDS+TEXT_SEGMENT_WORDS);

  free_instructions(program.text);
  free_data(program.data);
  free_lines(llh);

  return 0;
}
