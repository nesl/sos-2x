/**
 * \file avrregs.c
 * \brief Register Handling utilities for AVR
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <inttypes.h>
#include <avrregs.h>




int8_t printreg(uint8_t regnum)
{
  switch (regnum){
  case R0: printf("R0 "); break;
  case R1: printf("R1 "); break;
  case R2: printf("R2 "); break;
  case R3: printf("R3 "); break;
  case R4: printf("R4 "); break;
  case R5: printf("R5 "); break;
  case R6: printf("R6 "); break;
  case R7: printf("R7 "); break;
  case R8: printf("R8 "); break;
  case R9: printf("R9 "); break;
  case R10: printf("R10 "); break;
  case R11: printf("R11 "); break;
  case R12: printf("R12 "); break;
  case R13: printf("R13 "); break;
  case R14: printf("R14 "); break;
  case R15: printf("R15 "); break;
  case R16: printf("R16 "); break;
  case R17: printf("R17 "); break;
  case R18: printf("R18 "); break;
  case R19: printf("R19 "); break;
  case R20: printf("R20 "); break;
  case R21: printf("R21 "); break;
  case R22: printf("R22 "); break;
  case R23: printf("R23 "); break;
  case R24: printf("R24 "); break;
  case R25: printf("R25 "); break;
  case R26: printf("R26 "); break;
  case R27: printf("R27 "); break;
  case R28: printf("R28 "); break;
  case R29: printf("R29 "); break;
  case R30: printf("R30 "); break;
  case R31: printf("R31 "); break;
  default: printf("Invalid Register "); break;
  }
  return 0;
}
