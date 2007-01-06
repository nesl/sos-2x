/**
 * \file avrfreeops.c
 * \brief Find the unused opcodes in AVR ISA
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <inttypes.h>
#include <avrinstr.h>
#include <avrbinparser.h>

int avrFindFreeOpcodes();


int main(int argc, char* argv[])
{
  return avrFindFreeOpcodes();
}

int avrFindFreeOpcodes()
{
  uint16_t instrWord;
  int8_t retval;
  int numFreeOpcodes;
  
  instrWord = 0x0000;
  numFreeOpcodes = 0;

  while (instrWord < 0xFFFF){
    printf("0x%x: ", instrWord);
    retval = decode_avr_instr_word((avr_instr_t*) &instrWord);
    if (0 == retval){
     printf("\n");
    }
    if (1 == retval) continue;
    if (-1 == retval) {
      printf("<-------FREE\n", instrWord);
      numFreeOpcodes++;
    }
    instrWord++;
  }

  printf("Number of free opcodes: %d\n", numFreeOpcodes);
  return 0;
}
