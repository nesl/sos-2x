/**
 * \file avrinstmatch.c
 * \brief Matches the AVR instruction type
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <inttypes.h>
#include <avrinstr.h>
#include <avrregs.h>
#include <avrinstmatch.h>

//#define DEBUG(arg...) printf(arg)
#define DEBUG(arg...)

//-----------------------------------------------------------------------
// OP TYPE1 FUNCTIONS
//-----------------------------------------------------------------------
uint16_t create_optype1(uint16_t opcode, uint8_t d, uint8_t r)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE1_MASK;
  if ((d > 31) || (r > 31)){
    fprintf(stderr, "create_optype1: Incorrect operand.\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_5(instr.rawVal, d);
  instr.rawVal = set_rr_5(instr.rawVal, r);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype1(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", (instr->rawVal & OP_TYPE1_MASK));
  switch (instr->rawVal & OP_TYPE1_MASK){
  case OP_ADC:  return 0;
  case OP_ADD:  return 0;
  case OP_AND:  return 0;
  case OP_CP:   return 0;
  case OP_CPC:  return 0;
  case OP_CPSE: return 0;
  case OP_EOR:  return 0;
  case OP_MOV:  return 0;
  case OP_MUL:  return 0;
  case OP_OR:   return 0;
  case OP_SBC:  return 0;
  case OP_SUB:  return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype1_dreg(avr_instr_t* instr)
{
  uint8_t dreg;
  dreg = get_rd_5(instr->rawVal);
  return dreg;
}
//-----------------------------------------------------------------------
uint8_t get_optype1_rreg(avr_instr_t* instr)
{
  return get_rr_5(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE2 FUNCTIONS
//-----------------------------------------------------------------------
uint16_t create_optype2(uint16_t opcode, uint8_t d, uint8_t k)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE2_MASK;
  switch (d){
  case R24: case R26: case R28: case R30:
    instr.rawVal = set_rd_2(instr.rawVal, d);
    break;
  default:
    fprintf(stderr, "create_optype2: Invalid Register Operand\n");
    return OP_NOP;
  }
  if (k > 63){
    fprintf(stderr,"create_optype2: Invalid Immediate Operand\n");
    return OP_NOP;
  }
  instr.rawVal = set_k_6(instr.rawVal, k);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype2(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE2_MASK);
  switch (instr->rawVal & OP_TYPE2_MASK){
  case OP_ADIW: return 0;
  case OP_SBIW: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype2_dreg(avr_instr_t* instr)
{
  return get_rd_2(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype2_k(avr_instr_t* instr)
{
  return get_k_6(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 3 FUNCTIONS
//-----------------------------------------------------------------------
uint16_t create_optype3(uint16_t opcode, uint8_t d, uint8_t k)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE3_MASK;
  if (d < R16){
    fprintf(stderr,"create_optype3: Invalid Input Register\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_4(instr.rawVal, d);
  instr.rawVal = set_k_8(instr.rawVal, k);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype3(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE3_MASK);
  switch (instr->rawVal & OP_TYPE3_MASK){
  case OP_ANDI: return 0;
  case OP_CPI:  return 0;
  case OP_LDI:  return 0;
  case OP_ORI:  return 0;
  case OP_SBCI: return 0;
    //  case OP_SBR:  return 0; // -- Duplicate Opcode as OP_ORI
  case OP_SUBI: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype3_dreg(avr_instr_t* instr)
{
  return get_rd_4(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype3_k(avr_instr_t* instr)
{
  return get_k_8(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE4 FUNCTIONS
//-----------------------------------------------------------------------
uint16_t create_optype4(uint16_t opcode, uint8_t d)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE4_MASK;
  if (d > 31){
    fprintf(stderr, "create_optype4: Invalid immediate operand value\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_5(instr.rawVal, d);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype4(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE4_MASK);
  switch (instr->rawVal & OP_TYPE4_MASK){
  case OP_ASR: return 0;
  case OP_COM: return 0;
  case OP_DEC: return 0;
  case OP_INC: return 0;
  case OP_ELPM_Z: return 0;
  case OP_ELPM_Z_INC: return 0; 
  case OP_LD_X: return 0;
  case OP_LD_X_INC: return 0;
  case OP_LD_X_DEC: return 0;
  case OP_LD_Y: return 0;
  case OP_LD_Y_INC: return 0;
  case OP_LD_Y_DEC: return 0;
  case OP_LD_Z: return 0;
  case OP_LD_Z_INC: return 0;
  case OP_LD_Z_DEC: return 0;
  case OP_LPM_Z: return 0;
  case OP_LPM_Z_INC: return 0;
  case OP_LSR: return 0;
  case OP_NEG: return 0;
  case OP_POP: return 0;
  case OP_PUSH: return 0;
  case OP_ROR: return 0;
  case OP_ST_X: return 0;
  case OP_ST_X_INC: return 0;
  case OP_ST_X_DEC: return 0;
  case OP_ST_Y: return 0;
  case OP_ST_Y_INC: return 0;
  case OP_ST_Y_DEC: return 0;
  case OP_ST_Z: return 0;
  case OP_ST_Z_INC: return 0;
  case OP_ST_Z_DEC: return 0;
  case OP_SWAP: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype4_dreg(avr_instr_t* instr)
{
  return get_rd_5(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE5 FUNCTIONS
//-----------------------------------------------------------------------
uint16_t create_optype5(uint16_t opcode, uint8_t s)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE5_MASK;
  if (s > 7){
    fprintf(stderr,"create_optype5: Invalid s field\n");
    return OP_NOP;
  }
  instr.rawVal = set_sreg_bit(instr.rawVal, s);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;  
} 
//-----------------------------------------------------------------------
int8_t match_optype5(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE5_MASK);
  switch (instr->rawVal & OP_TYPE5_MASK){
  case OP_BCLR: return 0;
  case OP_BSET: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype5_s(avr_instr_t* instr)
{
  return get_sreg_bit(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE6 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype6(uint16_t opcode, uint8_t d, uint8_t b)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE6_MASK;
  if (d > 31){
    fprintf(stderr,"create_optype6: Invalid Register Field\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_5(instr.rawVal, d);
  if (b > 7){
    fprintf(stderr,"create_optype6: Invalide B Field\n");
    return OP_NOP;
  }
  instr.rawVal = set_reg_bit(instr.rawVal, b);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
} 
//-----------------------------------------------------------------------
int8_t match_optype6(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE6_MASK);
  switch (instr->rawVal & OP_TYPE6_MASK){
  case OP_BLD: return 0;
  case OP_BST: return 0;
  case OP_SBRC: return 0;
  case OP_SBRS: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype6_dreg(avr_instr_t* instr)
{
  return get_rd_5(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype6_b(avr_instr_t* instr)
{
  return get_reg_bit(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE7 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype7(uint16_t opcode, int8_t k, uint8_t s)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE7_MASK;
  if ((k > 63) || (k < -64)){
    fprintf(stderr,"create_optype7: Invalid K field\n");
    return OP_NOP;
  }
  if (s > 7){
    fprintf(stderr,"create_optype7: Invalid s field\n");
    return OP_NOP;
  }
  instr.rawVal = set_k_7(instr.rawVal, k);
  instr.rawVal = set_reg_bit(instr.rawVal, s);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}   
//-----------------------------------------------------------------------
int8_t match_optype7(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE7_MASK);
  switch (instr->rawVal & OP_TYPE7_MASK){
  case OP_BRBC: return 0;
  case OP_BRBS: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
int8_t get_optype7_k(avr_instr_t* instr)
{
  return get_k_7(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype7_s(avr_instr_t* instr)
{
  return get_reg_bit(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE8 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype8(uint16_t opcode, int8_t k)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE8_MASK;
  if ((k > 63)|(k < -64)){
    fprintf(stderr,"create_optype8: Invalid K field\n");
    return OP_NOP;
  }
  instr.rawVal = set_k_7(instr.rawVal, k);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype8(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE8_MASK);
  switch (instr->rawVal & OP_TYPE8_MASK){
  case OP_BRCC: return 0;
  case OP_BRCS: return 0;
  case OP_BREQ: return 0;
  case OP_BRGE: return 0;
  case OP_BRHC: return 0;
  case OP_BRHS: return 0;
  case OP_BRID: return 0;
  case OP_BRIE: return 0;
    //  case OP_BRLO: return 0;
  case OP_BRLT: return 0;
  case OP_BRMI: return 0;
  case OP_BRNE: return 0;
  case OP_BRPL: return 0;
    //  case OP_BRSH: return 0;
  case OP_BRTC: return 0;
  case OP_BRTS: return 0;
  case OP_BRVC: return 0;
  case OP_BRVS: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
int8_t get_optype8_k(avr_instr_t* instr)
{
  //  return (instr->op_type8.k);
  return get_k_7(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 9 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype9(uint16_t opcode)
{
  avr_instr_t instr;
  instr.rawVal = opcode;
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;  
}
//-----------------------------------------------------------------------
int8_t match_optype9(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal);
  switch (instr->rawVal){
  case OP_BREAK: return 0;
  case OP_CLC: return 0;
  case OP_CLH: return 0;
  case OP_CLI: return 0;
  case OP_CLN: return 0;
  case OP_CLS: return 0;
  case OP_CLT: return 0;
  case OP_CLV: return 0;
  case OP_CLZ: return 0;
  case OP_EICALL: return 0;
  case OP_EIJMP: return 0;
  case OP_ELPM_Z_R0: return 0;
  case OP_ICALL: return 0;
  case OP_IJMP: return 0;
  case OP_LPM_Z_R0: return 0;
  case OP_NOP: return 0;
  case OP_RET: return 0;
  case OP_RETI: return 0;
  case OP_SEC: return 0;
  case OP_SEH: return 0;
  case OP_SEI: return 0;
  case OP_SEN: return 0;
  case OP_SES: return 0;
  case OP_SET: return 0;
  case OP_SEV: return 0;
  case OP_SEZ: return 0;
  case OP_SLEEP: return 0;
  case OP_SPM: return 0;
  case OP_WDR: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
// OP TYPE 10 FUNCTION
//-----------------------------------------------------------------------
uint32_t create_optype10(uint16_t opcode, uint32_t k)
{
  avr_instr_t instr;
  uint16_t k_lval;
  uint32_t longinstr;
  instr.rawVal = opcode & OP_TYPE10_MASK;
  if (k > 0x3FFFFF){
    fprintf(stderr, "create_optype10: Invalid address\n");
    return OP_NOP;
  }
  instr.rawVal = set_k_22(instr.rawVal, k);
  k_lval = (uint16_t)(k & 0x0000FFFF);
  longinstr = (uint32_t)((uint32_t)(instr.rawVal << 16)|(uint32_t)k_lval);
  DEBUG("Created Instruction: %x\n",longinstr);
  return longinstr;
}
//-----------------------------------------------------------------------
int8_t match_optype10(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE10_MASK);
  switch (instr->rawVal & OP_TYPE10_MASK){
  case OP_CALL: return 0;
  case OP_JMP : return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint32_t get_optype10_k(avr_instr_t* instr, avr_instr_t* nextinstr)
{
  uint32_t kval;
  kval = (uint32_t) nextinstr->rawVal;
  kval = kval + (get_k_22(instr->rawVal) << 16);
  return kval;
}
//-----------------------------------------------------------------------
// OP TYPE 11 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype11(uint16_t opcode, uint8_t a, uint8_t b)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE11_MASK;
  if (a > 31){
    fprintf(stderr,"create_optype11: Wrong A field\n");
    return OP_NOP;
  }
  if (b > 7){
    fprintf(stderr,"create_optype11: Wrong B field\n");
    return OP_NOP;
  }
  instr.rawVal = set_a_5(instr.rawVal, a);
  instr.rawVal = set_reg_bit(instr.rawVal, b);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;  
}
//-----------------------------------------------------------------------
int8_t match_optype11(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE11_MASK);
  switch (instr->rawVal & OP_TYPE11_MASK){
  case OP_CBI: return 0;
  case OP_SBI: return 0;
  case OP_SBIC: return 0;
  case OP_SBIS: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype11_a(avr_instr_t* instr)
{
  return get_a_5(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype11_b(avr_instr_t* instr)
{
  return get_reg_bit(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 12 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype12(uint16_t opcode, uint8_t d, uint8_t r)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE12_MASK;
  if ((d < 16)||(d > 23)){
    fprintf(stderr,"create_optype12: Invalid dreg \n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_3(instr.rawVal, d);
  if ((r < 16)||(r > 23)){
    fprintf(stderr,"create_optype12: Invalid rreg \n");
    return OP_NOP;
  }
  instr.rawVal = set_rr_3(instr.rawVal, r);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype12(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE12_MASK);
  switch (instr->rawVal & OP_TYPE12_MASK){
  case OP_FMUL: return 0;
  case OP_FMULS: return 0;
  case OP_FMULSU: return 0;
  case OP_MULSU: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype12_d(avr_instr_t* instr)
{
  return get_rd_3(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype12_r(avr_instr_t* instr)
{
  return get_rr_3(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 13 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype13(uint16_t opcode, uint8_t d, uint8_t a)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE13_MASK;
  if (d > 31){
    fprintf(stderr, "create_optype13: Wrong D field.\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_5(instr.rawVal, d);
  if (a > 63){
    fprintf(stderr,"create_optype13: Wrong A field\n");
    return OP_NOP;
  }
  instr.rawVal = set_a_6(instr.rawVal, a);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype13(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE13_MASK);
  switch (instr->rawVal & OP_TYPE13_MASK){
  case OP_IN: return 0;
  case OP_OUT: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype13_d(avr_instr_t* instr)
{
  return get_rd_5(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype13_a(avr_instr_t* instr)
{
  return get_a_6(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 14 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype14(uint16_t opcode, uint8_t d, uint8_t q)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE14_MASK;
  if (d > 31){
    fprintf(stderr, "create_optype14: Incorrect register operand\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_5(instr.rawVal, d);
  if (q > 63){
    fprintf(stderr,"create_optype14: Wrong Q field\n");
    return OP_NOP;
  }
  instr.rawVal = set_q(instr.rawVal, q);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype14(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE14_MASK);
  switch (instr->rawVal & OP_TYPE14_MASK){
  case OP_LDD_Y: return 0;
  case OP_LDD_Z: return 0;
  case OP_STD_Y: return 0;
  case OP_STD_Z: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype14_d(avr_instr_t* instr)
{
  return get_rd_5(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype14_q(avr_instr_t* instr)
{
  return get_q(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 15 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype15(uint16_t opcode, uint8_t d, uint8_t r)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE15_MASK;
  if ((d > 31)||(d < 16)){
    fprintf(stderr,"create_optype15: Invalid d register\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_4(instr.rawVal, d);
  if ((r > 31)||(r < 16)){
    fprintf(stderr,"create_optype15: Invalid r register\n");
    return OP_NOP;
  }
  instr.rawVal = set_rr_4(instr.rawVal, r);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype15(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE15_MASK);
  switch (instr->rawVal & OP_TYPE15_MASK){
  case OP_MULS: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype15_d(avr_instr_t* instr)
{
  return get_rd_4(instr->rawVal);
}
//-----------------------------------------------------------------------
uint8_t get_optype15_r(avr_instr_t* instr)
{
  return get_rr_4(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 16 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype16(uint16_t opcode, uint8_t d, uint8_t r)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE16_MASK;
  if (((d % 2) == 1)||(d > 31)){
    fprintf(stderr,"create_optype16: Invalid d register\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_4(instr.rawVal, (d >> 1) + 16); //<----NOTE
  if (((r % 2) == 1)||(r > 31)){
    fprintf(stderr,"create_optype16: Invalid d register\n");
    return OP_NOP;
  }
  instr.rawVal = set_rr_4(instr.rawVal, (r >> 1) + 16); //<----NOTE
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype16(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE16_MASK);
  switch (instr->rawVal & OP_TYPE16_MASK){
  case OP_MOVW: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype16_d(avr_instr_t* instr)
{
  return ((get_rd_4(instr->rawVal) - 16) << 1); //<----NOTE
}
//-----------------------------------------------------------------------
uint8_t get_optype16_r(avr_instr_t* instr)
{
  return ((get_rr_4(instr->rawVal) - 16) << 1); //<----NOTE
}
//-----------------------------------------------------------------------
// OP TYPE 17 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype17(uint16_t opcode, int16_t k)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE17_MASK;
  if ((k < -2048) || (k > 2047)){
    fprintf(stderr,"create_optype17: Invalid K value\n");
    return OP_NOP;
  }
  instr.rawVal = set_k_12(instr.rawVal, k);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype17(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE17_MASK);
  switch (instr->rawVal & OP_TYPE17_MASK){
  case OP_RCALL: return 0;
  case OP_RJMP: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
int16_t get_optype17_k(avr_instr_t* instr)
{
  return get_k_12(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 18 FUNCTION
//-----------------------------------------------------------------------
uint16_t create_optype18(uint16_t opcode, uint8_t d)
{
  avr_instr_t instr;
  instr.rawVal = opcode & OP_TYPE18_MASK;
  if ((d > 31)||(d < 16)){
    fprintf(stderr,"create_optype18: Invalid d register\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_4(instr.rawVal, d);
  DEBUG("Created Instruction: %x\n",instr.rawVal);
  return instr.rawVal;
}
//-----------------------------------------------------------------------
int8_t match_optype18(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE18_MASK);
  switch (instr->rawVal & OP_TYPE18_MASK){
  case OP_SER: return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype18_d(avr_instr_t* instr)
{
  return get_rd_4(instr->rawVal);
}
//-----------------------------------------------------------------------
// OP TYPE 19 FUNCTION
//-----------------------------------------------------------------------
uint32_t create_optype19(uint16_t opcode, uint8_t d, uint16_t k)
{
  avr_instr_t instr;
  uint32_t longinstr;
  instr.rawVal = opcode & OP_TYPE19_MASK;
  if (d > 31){
    fprintf(stderr,"create_optype19: Invalid d value\n");
    return OP_NOP;
  }
  instr.rawVal = set_rd_5(instr.rawVal, d);
  longinstr = (uint32_t)((uint32_t)(instr.rawVal << 16)|(uint32_t)k);
  DEBUG("Created Instruction: %x\n",longinstr);
  return longinstr;
}
//-----------------------------------------------------------------------
int8_t match_optype19(avr_instr_t* instr)
{
  DEBUG("OpCode: %x\n", instr->rawVal & OP_TYPE19_MASK);
  switch (instr->rawVal & OP_TYPE19_MASK){
  case OP_LDS: return 0;
  case OP_STS : return 0;
  default: return -1;
  }
  return -1;
}
//-----------------------------------------------------------------------
uint8_t get_optype19_d(avr_instr_t* instr)
{
  return get_rd_5(instr->rawVal);
}
//-----------------------------------------------------------------------
uint16_t get_optype19_k(avr_instr_t* instr)
{
  return (instr->rawVal);
}
