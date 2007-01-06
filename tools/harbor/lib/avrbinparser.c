/**
 * \file avrbinparser.c
 * \brief Binary Parser For AVR Instruction Set
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <avrinstr.h>
#include <avrregs.h>
#include <avrinstmatch.h>
#include <avrbinparser.h>
#include <dispinstr.h>

uint8_t twowordinstr;
avr_instr_t previnstr;

int8_t decode_avr_instr_word(avr_instr_t* instr)
{
  if (twowordinstr){
    twowordinstr = 0;
    if (match_optype10(&previnstr) == 0){
      print_optype10(&previnstr, instr);
      return 0;
    }
    if (match_optype19(&previnstr) == 0){
      print_optype19(&previnstr, instr);
      return 0;
    }
  }


  if (match_optype1(instr) == 0){
    print_optype1(instr);
    return 0;
  }
  if (match_optype2(instr) == 0){
    print_optype2(instr);
    return 0;
  }
  if (match_optype3(instr) == 0){
    print_optype3(instr);
    return 0;
  }
  if (match_optype4(instr) == 0){
    print_optype4(instr);
    return 0;
  }
  if (match_optype5(instr) == 0){
    print_optype5(instr);
    return 0;
  }
  if (match_optype6(instr) == 0){
    print_optype6(instr);
    return 0;
  }
  if (match_optype7(instr) == 0){
    print_optype7(instr);
    return 0;
  }
  if (match_optype8(instr) == 0){
    print_optype8(instr);
    return 0;
  }
  if (match_optype9(instr) == 0){
    print_optype9(instr);
    return 0;
  }
  if (match_optype10(instr) == 0){
    twowordinstr = 1;
    previnstr.rawVal = instr->rawVal;
    return 1;
  }
  if (match_optype11(instr) == 0){
    print_optype11(instr);
    return 0;
  }
  if (match_optype12(instr) == 0){
    print_optype12(instr);
    return 0;
  }
  if (match_optype13(instr) == 0){
    print_optype13(instr);
    return 0;
  }
  if (match_optype14(instr) == 0){
    print_optype14(instr);
    return 0;
  }
  if (match_optype15(instr) == 0){
    print_optype15(instr);
    return 0;
  }
  if (match_optype16(instr) == 0){
    print_optype16(instr);
    return 0;
  }
  if (match_optype17(instr) == 0){
    print_optype17(instr);
    return 0;
  }
  if (match_optype18(instr) == 0){
    print_optype18(instr);
    return 0;
  }
  if (match_optype19(instr) == 0){
    twowordinstr = 1;
    previnstr.rawVal = instr->rawVal;
    return 1;
  }

  printf("Invalid Opcode\n");
  return -1;
}

