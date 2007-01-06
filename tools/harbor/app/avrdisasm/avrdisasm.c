/**
 * \file avrdisasm.c
 * \brief A Dis-assember for the AVR binary
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \note FileIO sections of the code are from loader_pc in SOS-1.x
 */
#include <stdio.h>
#include <inttypes.h>
#include <avrinstr.h>
#include <avrbinparser.h>

#define	Flip_int16(type)  (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))
int avrdisasm(char* binFileName);


int main(int argc, char* argv[])
{
  if (argc != 2){
    printf("Usage: avrdisasm <filename>\n");
    printf("Input file should be a pure binary.\n");
    return 0;
  }
  return avrdisasm(argv[1]);
}

int avrdisasm(char* binFileName)
{
  FILE *binFile;
  int byte1, byte2;
  uint16_t instrWord;
  uint32_t currAddr;

  binFile = fopen((char*)binFileName, "r");

  if (NULL == binFile){
    fprintf(stderr, "%s does not exist\n", binFileName);
    return -1;
  }

  currAddr = 0;

  while (fread(&instrWord, sizeof(uint16_t), 1, binFile) != 0){
#ifdef BBIG_ENDIAN
    instrWord = Flip_int16(instrWord);
#endif
      printf("0x%5x: %04x ", currAddr, instrWord);
      decode_avr_instr_word((avr_instr_t*) &instrWord);
      printf("\n");
      currAddr += 2;
  }
  return 0;
}
