/**
 * \file dispinstr.c
 * \brief Routines to print instructions
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <avrinstr.h>
#include <avrregs.h>
#include <avrinstmatch.h>
//-----------------------------------------------------------------------
int8_t print_optype1(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE1_MASK){
  case OP_ADC:  printf("ADC "); break;
  case OP_ADD:  printf("ADD "); break;
  case OP_AND:  printf("AND "); break;
  case OP_CP:   printf("CP ");  break;
  case OP_CPC:  printf("CPC "); break;
  case OP_CPSE: printf("CPSE ");break;
  case OP_EOR:  printf("EOR "); break;
  case OP_MOV:  printf("MOV "); break;
  case OP_MUL:  printf("MUL "); break;
  case OP_OR:   printf("OR ");  break;
  case OP_SBC:  printf("SBC "); break;
  case OP_SUB:  printf("SUB "); break;
  default:      printf("Wrong Opcode "); break;
  }
  printreg(get_optype1_dreg(instr));
  printreg(get_optype1_rreg(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype2(avr_instr_t* instr)
{
  switch(instr->rawVal & OP_TYPE2_MASK){
  case OP_ADIW: printf("ADIW "); break;
  case OP_SBIW: printf("SBIW "); break;
  default: printf("Wrong Opcode "); break;
  }
  printreg(get_optype2_dreg(instr));
  printf("%d ", get_optype2_k(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype3(avr_instr_t* instr)
{
  switch(instr->rawVal & OP_TYPE3_MASK){
  case OP_ANDI:printf("ANDI "); break;
  case OP_CPI: printf("CPI ");  break;
  case OP_LDI: printf("LDI ");  break;
  case OP_ORI: printf("ORI ");  break;
  case OP_SBCI:printf("SBCI "); break;
    //  case OP_SBR: printf("SBR ");  break; // Same as ORI
  case OP_SUBI:printf("SUBI "); break;
  default: printf("Wrong Opcode "); break;
  }  
  printreg(get_optype3_dreg(instr));
  printf("0x%x ",get_optype3_k(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype4(avr_instr_t* instr)
{
  uint8_t reg;
  reg = get_optype4_dreg(instr);
  switch(instr->rawVal & OP_TYPE4_MASK){
  case OP_ASR: printf("ASR "); printreg(reg); break;
  case OP_COM: printf("COM "); printreg(reg); break;
  case OP_DEC: printf("DEC "); printreg(reg); break;
  case OP_INC: printf("INC "); printreg(reg); break;
  case OP_ELPM_Z: printf("ELPM "); printreg(reg); printf("Z"); break;
  case OP_ELPM_Z_INC: printf("ELPM "); printreg(reg); printf("Z+"); break; 
  case OP_LD_X: printf("LD "); printreg(reg); printf("X"); break;
  case OP_LD_X_INC: printf("LD "); printreg(reg); printf("X+"); break;
  case OP_LD_X_DEC: printf("LD "); printreg(reg); printf("-X"); break;
  case OP_LD_Y: printf("LD "); printreg(reg); printf("Y"); break;
  case OP_LD_Y_INC: printf("LD "); printreg(reg); printf("Y+"); break;
  case OP_LD_Y_DEC: printf("LD "); printreg(reg); printf("-Y");break;
  case OP_LD_Z: printf("LD "); printreg(reg); printf("Z");break;
  case OP_LD_Z_INC: printf("LD "); printreg(reg); printf("Z+"); break;
  case OP_LD_Z_DEC: printf("LD "); printreg(reg); printf("-Z");break;
  case OP_LPM_Z: printf("LPM "); printreg(reg); printf("Z"); break;
  case OP_LPM_Z_INC: printf("LPM "); printreg(reg); printf("Z+");break;
  case OP_LSR: printf("LSR "); printreg(reg); break;
  case OP_NEG: printf("NEG "); printreg(reg); break;
  case OP_POP: printf("POP "); printreg(reg); break;
  case OP_PUSH: printf("PUSH "); printreg(reg); break;
  case OP_ROR: printf("ROR "); printreg(reg); break;
  case OP_ST_X: printf("ST X "); printreg(reg); break;
  case OP_ST_X_INC: printf("ST X+ "); printreg(reg); break;
  case OP_ST_X_DEC: printf("ST -X "); printreg(reg); break;
  case OP_ST_Y: printf("ST Y "); printreg(reg); break;
  case OP_ST_Y_INC: printf("ST Y+ "); printreg(reg); break;
  case OP_ST_Y_DEC: printf("ST -Y "); printreg(reg); break;
  case OP_ST_Z: printf("ST Z "); printreg(reg); break;
  case OP_ST_Z_INC: printf("ST Z+ "); printreg(reg); break;
  case OP_ST_Z_DEC: printf("ST -Z "); printreg(reg); break;
  case OP_SWAP: printf("SWAP "); printreg(reg); break;    
  default: printf("Wrong Opcode "); printreg(reg); break;
  }  
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype5(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE5_MASK){
  case OP_BCLR: printf("BCLR "); break;
  case OP_BSET: printf("BSET "); break;
  default: printf("Wrong Opcode "); break;
  }
  printf("%d ", get_optype5_s(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype6(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE6_MASK){
  case OP_BLD: printf("BLD "); break;
  case OP_BST: printf("BST "); break;
  case OP_SBRC: printf("SBRC "); break;
  case OP_SBRS: printf("SBRS "); break;
  default: printf("Wrong OpCode "); break;
  }
  printreg(get_optype6_dreg(instr));
  printf("%d ", get_optype6_b(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype7(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE7_MASK){
  case OP_BRBC: printf("BRBC "); break;
  case OP_BRBS: printf("BRBS "); break;
  default: printf("Wrong OpCode "); break;
  }
  printf("%d,", get_optype7_s(instr));
  printf("%d ", 2 * get_optype7_k(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype8(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE8_MASK){
  case OP_BRCC: printf("BRCC "); break;
  case OP_BRCS: printf("BRCS "); break;
  case OP_BREQ: printf("BREQ "); break;
  case OP_BRGE: printf("BRGE "); break;
  case OP_BRHC: printf("BRHC "); break;
  case OP_BRHS: printf("BRHS "); break;
  case OP_BRID: printf("BRID "); break;
  case OP_BRIE: printf("BRIE "); break;
    //case OP_BRLO: printf("BRLO "); break;
  case OP_BRLT: printf("BRLT "); break;
  case OP_BRMI: printf("BRMI "); break;
  case OP_BRNE: printf("BRNE "); break;
  case OP_BRPL: printf("BRPL "); break;
    //case OP_BRSH: printf("BRSH "); break;
  case OP_BRTC: printf("BRTC "); break;
  case OP_BRTS: printf("BRTS "); break;
  case OP_BRVC: printf("BRVC "); break;
  case OP_BRVS: printf("BRVS "); break;
  default: printf("Wrong OpCode "); break;
  }
  printf(".%d ", 2*get_optype8_k(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype9(avr_instr_t* instr)
{
  switch (instr->rawVal){
  case OP_BREAK: printf("BREAK "); break;
  case OP_CLC: printf("CLC "); break;
  case OP_CLH: printf("CLH "); break;
  case OP_CLI: printf("CLI "); break;
  case OP_CLN: printf("CLN "); break;
  case OP_CLS: printf("CLS "); break;
  case OP_CLT: printf("CLT "); break;
  case OP_CLV: printf("CLV "); break;
  case OP_CLZ: printf("CLZ "); break;
  case OP_EICALL: printf("EICALL "); break;
  case OP_EIJMP: printf("EIJMP "); break;
  case OP_ELPM_Z_R0: printf("ELPM Z, R0 "); break;
  case OP_ICALL: printf("ICALL "); break;
  case OP_IJMP: printf("IJMP "); break;
  case OP_LPM_Z_R0: printf("LPM Z, R0 "); break;
  case OP_NOP: printf("NOP "); break;
  case OP_RET: printf("RET "); break;
  case OP_RETI: printf("RETI "); break;
  case OP_SEC: printf("SEC "); break;
  case OP_SEH: printf("SEH "); break;
  case OP_SEI: printf("SEI "); break;
  case OP_SEN: printf("SEN "); break;
  case OP_SES: printf("SES "); break;
  case OP_SET: printf("SET "); break;
  case OP_SEV: printf("SEV "); break;
  case OP_SEZ: printf("SEZ "); break;
  case OP_SLEEP: printf("SLEEP "); break;
  case OP_SPM: printf("SPM "); break;
  case OP_WDR: printf("WDR "); break;
  default: printf("Wrong Opcode ");
  }
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype10(avr_instr_t* instr, avr_instr_t* nextinstr)
{
  switch (instr->rawVal & OP_TYPE10_MASK){
  case OP_CALL: printf("CALL "); break;
  case OP_JMP: printf("JMP "); break;
  default: printf("Wrong Opcode ");
  }
  printf("0x%x ", (int)(2*get_optype10_k(instr, nextinstr)));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype11(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE11_MASK){
  case OP_CBI: printf("CBI "); break;
  case OP_SBI: printf("SBI "); break;
  case OP_SBIC: printf("SBIC "); break;
  case OP_SBIS: printf("SBIS "); break;
  default: printf("Wrong Opcode ");
  }
  printreg(get_optype11_a(instr));
  printf(",0x%x ", get_optype11_b(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype12(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE12_MASK){
  case OP_FMUL: printf("FMUL "); break;
  case OP_FMULS: printf("FMULS "); break;
  case OP_FMULSU: printf("FMULSU "); break;
  case OP_MULSU: printf("MULSU "); break;
  default: printf("Wrong Opcode "); break;
  }
  printreg(get_optype12_d(instr));
  printreg(get_optype12_r(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype13(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE13_MASK){
  case OP_IN: printf("IN "); break;
  case OP_OUT: printf("OUT "); break;
  default: printf("Wrong Opcode "); break;
  }
  printreg(get_optype13_d(instr));
  printf("%x ", get_optype13_a(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype14(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE14_MASK){
  case OP_LDD_Y:
    { 
      printf("LDD "); 
      printreg(get_optype14_d(instr)); 
      printf("Y+");  
      printf("0x%x ", get_optype14_q(instr));
      break;
    }
  case OP_LDD_Z:
    { 
      printf("LDD "); 
      printreg(get_optype14_d(instr)); 
      printf("Z+");  
      printf("0x%x ", get_optype14_q(instr));
      break;
    }
  case OP_STD_Y:
    { 
      printf("STD "); 
      printf("Y+");  
      printf("0x%x,", get_optype14_q(instr));
      printreg(get_optype14_d(instr)); 
      break;
    }
  case OP_STD_Z:
    { 
      printf("STD "); 
      printf("Z+");  
      printf("0x%x,", get_optype14_q(instr));
      printreg(get_optype14_d(instr)); 
      break;
    }
  default: printf("Wrong Opcode "); break;
  }
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype15(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE15_MASK){
  case OP_MULS: printf("MULS "); break;
  default: printf("Invalid Opcode "); break;
  }
  printreg(get_optype15_d(instr));
  printreg(get_optype15_r(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype16(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE16_MASK){
  case OP_MOVW: printf("MOVW "); break;
  default: printf("Invalid Opcode "); break;
  }
  printreg(get_optype16_d(instr));
  printreg(get_optype16_r(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype17(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE17_MASK){
  case OP_RCALL: printf("RCALL "); break;
  case OP_RJMP: printf("RJMP "); break;
  default: printf("Invalid Opcode "); break;
  }
  printf(".%d ",2*get_optype17_k(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype18(avr_instr_t* instr)
{
  switch (instr->rawVal & OP_TYPE18_MASK){
  case OP_SER: printf("SER "); break;
  default: printf("Invalid Opcode "); break;
  }
  printreg(get_optype18_d(instr));
  
  return 0;
}
//-----------------------------------------------------------------------
int8_t print_optype19(avr_instr_t* instr, avr_instr_t* nextinstr)
{
  switch (instr->rawVal & OP_TYPE19_MASK){
  case OP_LDS:
    {
      printf("LDS "); 
      printreg(get_optype19_d(instr)); 
      printf("0x%x", get_optype19_k(nextinstr));
      break;
    }
  case OP_STS: 
    {
      printf("STS "); 
      printf("0x%x ", get_optype19_k(nextinstr));
      printreg(get_optype19_d(instr)); 
      break;
    }
  default: printf("Wrong Opcode ");
  }
  
  return 0;
}
