/**
 * \file avr_sbx.c
 * \brief Routines to generate sandbox code
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>

#include <avrinstr.h>
#include <avrinstmatch.h>
#include <avrbinparser.h>
#include <basicblock.h>
#include <avrsandbox.h>
#include <sos_mod_patch_code.h>

//----------------------------------------------------------------------------
// STATIC FUNCTIONS
static void avr_gen_st_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx);
static void avr_gen_ret_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx);
static void avr_gen_icall_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx);



//-------------------------------------------------------------------
int avr_get_sandbox_desc(avr_instr_t* instr, sandbox_desc_t* sndbx)
{
  // An assumtion in this routine: 
  // 1. We use R26 and R27 for sandboxing.
  // 2. These registers are not fixed.
  typedef struct _str_st_sbx_desc{
    uint16_t opcode;
    int numnewinstr;
  } st_sbx_desc_t;
  #define LUT_SIZE 9
  static const st_sbx_desc_t st_sbx[LUT_SIZE] = {{OP_ST_X, 5}, {OP_ST_Y, 5}, {OP_ST_Z, 5},
						 {OP_ST_X_INC, 6}, {OP_ST_Y_INC, 6}, {OP_ST_Z_INC, 6},
						 {OP_ST_X_DEC, 6}, {OP_ST_Y_DEC, 6}, {OP_ST_Z_DEC, 6}};
  static int twowordinstr = 0;
  static avr_instr_t previnstr;

  
  // Two-word instructions
  if (twowordinstr){
    twowordinstr = 0;
    if ((previnstr.rawVal & OP_TYPE19_MASK) == OP_STS){
      sndbx->numnewinstr = 11;
      sndbx->sbxtype = ST_SBX_TYPE;
      sndbx->optype = OP_TYPE19;
      sndbx->instr.rawVal = previnstr.rawVal;
      sndbx->nextinstr.rawVal = instr->rawVal;
      return 0;
    }
    return -1;
  }
  // If it is a two-word instruction. 
  // We need to sandbox only the STS instruction
  // OPTYPE 19 Instruction: STS
  switch (instr->rawVal & OP_TYPE19_MASK){
  case OP_STS:
  case  OP_LDS:
    previnstr.rawVal = instr->rawVal;
    twowordinstr = 1;
    return -1;
  }
  // We don't care for the rest of the two word instructions
  switch (instr->rawVal & OP_TYPE10_MASK){
  case OP_CALL:
  case OP_JMP:
    previnstr.rawVal = instr->rawVal;
    twowordinstr = 1;
    return -1;
  }

  // One-word instructions
  sndbx->instr.rawVal = instr->rawVal;
  // OPTYPE 4 Instructions: All other store instructions
  switch (instr->rawVal & OP_TYPE4_MASK){
  case OP_ST_X: case OP_ST_Y: case OP_ST_Z:
  case OP_ST_X_INC: case OP_ST_Y_INC: case OP_ST_Z_INC:
  case OP_ST_X_DEC: case OP_ST_Y_DEC: case OP_ST_Z_DEC:
    {
      sndbx->optype = OP_TYPE4;
      sndbx->sbxtype = ST_SBX_TYPE;
      int i;
      for (i = 0; i < LUT_SIZE; i++){
	if (st_sbx[i].opcode == (instr->rawVal & OP_TYPE4_MASK)){
	  sndbx->numnewinstr = st_sbx[i].numnewinstr;
	  return 0;
	}
      }
    }
  }
  // OPTYPE 14 Instructions
  switch (instr->rawVal & OP_TYPE14_MASK){
  case OP_STD_Y: case OP_STD_Z:
    {
      sndbx->sbxtype = ST_SBX_TYPE;
      sndbx->optype = OP_TYPE14;
      sndbx->numnewinstr = 7;
      return 0;
    }
  }
  // OPTYPE 9 Instruction (RET)
  switch (instr->rawVal){
  case OP_RET:
    sndbx->sbxtype = RET_SBX_TYPE;
    sndbx->optype = OP_TYPE9;
    sndbx->numnewinstr = 2;
    return 0;
  case OP_ICALL:
    sndbx->sbxtype = ICALL_SBX_TYPE;
    sndbx->optype = OP_TYPE9;
    sndbx->numnewinstr = 2;
    return 0;
  }

  return -1;
}
//-------------------------------------------------------------------
void avr_gen_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx, uint16_t calladdr)
{
  switch (sndbx->sbxtype){
  case ST_SBX_TYPE:
    avr_gen_st_sandbox(sandboxblk, sndbx);
    break;
  case RET_SBX_TYPE:
    avr_gen_ret_sandbox(sandboxblk, sndbx);
    break;
  case ICALL_SBX_TYPE:
    avr_gen_icall_sandbox(sandboxblk, sndbx);
    break;
  default:
    fprintf(stderr, "Cannot sandbox this instruction. Bug in sandbox.\n");
    exit(EXIT_FAILURE);
  }
  return;
}
//-------------------------------------------------------------------
static void avr_gen_st_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx){

  #define ADDR_NO_CHANGE 0
  #define ADDR_PRE_DEC   1
  #define ADDR_POST_INC  2
  #define ADDR_OFFSET    3
  #define ADDR_CONST     4

  int i;
  uint8_t srcreg, skippushpop, addrmode, addrreg, addroffset;
  uint16_t sts_addr;
  uint32_t callinstr;

  i = 0;
  skippushpop = 0;
  addrmode = 0;
  addroffset = 0;
  sts_addr = 0;

  switch (sndbx->optype){
  case OP_TYPE4:
    {
      srcreg = get_optype4_dreg(&sndbx->instr);
      switch (sndbx->instr.rawVal & OP_TYPE4_MASK){
      case OP_ST_X: addrreg = R26; addrmode = ADDR_NO_CHANGE; break;
      case OP_ST_X_INC: addrreg = R26; addrmode = ADDR_POST_INC; break;
      case OP_ST_X_DEC: addrreg = R26; addrmode = ADDR_PRE_DEC; break;
      case OP_ST_Y: addrreg = R28; addrmode = ADDR_NO_CHANGE; break;
      case OP_ST_Y_INC: addrreg = R28; addrmode = ADDR_POST_INC; break;
      case OP_ST_Y_DEC: addrreg = R28; addrmode = ADDR_PRE_DEC; break;
      case OP_ST_Z: addrreg = R30; addrmode = ADDR_NO_CHANGE; break;
      case OP_ST_Z_INC: addrreg = R30; addrmode = ADDR_POST_INC; break;
      case OP_ST_Z_DEC: addrreg = R30; addrmode = ADDR_PRE_DEC; break;

      break;
      }
      if (AVR_FIXED_REG_1 == addrreg)
	skippushpop = 1;
      else
	skippushpop = 0;
      break;
    }
  case OP_TYPE14:
    {
      skippushpop = 0;
      srcreg = get_optype14_d(&sndbx->instr);
      addroffset = get_optype14_q(&sndbx->instr);
      addrmode = ADDR_OFFSET;
      switch (sndbx->instr.rawVal & OP_TYPE14_MASK){
      case OP_STD_Y: addrreg = R28; break;
      case OP_STD_Z: addrreg = R30; break;
      }
      break;
    }
  case OP_TYPE19:
    {
      skippushpop = 0;
      srcreg = get_optype19_d(&sndbx->instr);
      sts_addr = get_optype19_k(&sndbx->nextinstr);
      addrmode = ADDR_CONST;
      break;
    }
  };
  
#ifndef SBX_FIX_REGS
  // PUSH AVR_SCRATCH_REG
  sandboxblk->instr[i].rawVal = create_optype4(OP_PUSH, AVR_SCRATCH_REG);
  i++;
#endif
  // MOV AVR_SCRATCH_REG, Rsrc
  sandboxblk->instr[i].rawVal = create_optype1(OP_MOV, AVR_SCRATCH_REG, srcreg);
  i++;

#if 0 // No push pop of address registers required
#ifndef SBX_FIX_REGS
  if (!skippushpop){
    // PUSH AVR_FIXED_REG_1
    sandboxblk->instr[i].rawVal = create_optype4(OP_PUSH, AVR_FIXED_REG_1);
    i++;
    // PUSH AVR_FIXED_REG_1
    sandboxblk->instr[i].rawVal = create_optype4(OP_PUSH, AVR_FIXED_REG_2);
    i++;
  }
#endif
#endif 
  
  switch (addrmode){
  case ADDR_CONST:
    {
      // PUSH AVR_FIXED_REG_1
      sandboxblk->instr[i].rawVal = create_optype4(OP_PUSH, AVR_FIXED_REG_1);
      i++;
      // PUSH AVR_FIXED_REG_1
      sandboxblk->instr[i].rawVal = create_optype4(OP_PUSH, AVR_FIXED_REG_2);
      i++;
      // LDI AVR_FIXED_REG_1, lo8(sts_addr)
      sandboxblk->instr[i].rawVal = create_optype3(OP_LDI, AVR_FIXED_REG_1, (uint8_t)(sts_addr));
      i++;
      // LDI AVR_FIXED_REG_2, hi8(sts_addr)
      sandboxblk->instr[i].rawVal = create_optype3(OP_LDI, AVR_FIXED_REG_2, (uint8_t)((uint16_t)(sts_addr >> 8)));
      i++;
      break;
    }
  case ADDR_NO_CHANGE:
  case ADDR_POST_INC:
    {
#if 0 // No more moves of address regs. required
      if (!skippushpop){
	// MOVW AVR_FIXED_REG_1, (Y or Z)
	sandboxblk->instr[i].rawVal = create_optype16(OP_MOVW, AVR_FIXED_REG_1, addrreg);
	i++;
      }
#endif
      break;
    }
  case ADDR_PRE_DEC:
    {
      // SBIW (X/Y/Z), 1
      sandboxblk->instr[i].rawVal = create_optype2(OP_SBIW, addrreg, 1);
      i++;
#if 0 // No more moves of address regs. required
      if (!skippushpop){
	// MOVW AVR_FIXED_REG_1, (Y or Z)
	sandboxblk->instr[i].rawVal = create_optype16(OP_MOVW, AVR_FIXED_REG_1, addrreg);
	i++;
      }
#endif
      break;
    }
  case ADDR_OFFSET:
    {
      // ADIW (X/Y/Z), offset
      sandboxblk->instr[i].rawVal = create_optype2(OP_ADIW, addrreg, addroffset);
      i++;
#if 0 // No more moves of address regs. required
      // MOVW AVR_FIXED_REG_1, (X/Y/Z)
      sandboxblk->instr[i].rawVal = create_optype16(OP_MOVW, AVR_FIXED_REG_1, addrreg);
      i++;
#endif      
      break;
    }
  }

  // CALL ker_memmap_perms_check
  switch (addrreg){
  case R26:
    {
      callinstr = create_optype10(OP_CALL, KER_MEMMAP_PERMS_CHECK_X_CODE);
      break;
    }
  case R28:
    {
      callinstr = create_optype10(OP_CALL, KER_MEMMAP_PERMS_CHECK_Y_CODE);
      break;
    }
  case R30:
    {
      callinstr = create_optype10(OP_CALL, KER_MEMMAP_PERMS_CHECK_Z_CODE);
      break;
    }
  default:
    {
      fprintf(stderr, "Invalid pointer register being sandboxed.\n");
      exit(EXIT_FAILURE);
    }
  }
  sandboxblk->instr[i].rawVal = (uint16_t)(callinstr>>16);
  i++;
  sandboxblk->instr[i].rawVal = (uint16_t)(callinstr);
  i++;


  switch (addrmode){
  case ADDR_POST_INC:
    {
      // ADIW (X/Y/Z), 1
      sandboxblk->instr[i].rawVal = create_optype2(OP_ADIW, addrreg, 1);
      i++;
      break;
    }
  case ADDR_OFFSET:
    {
      // SBIW (X/Y/Z), offset
      sandboxblk->instr[i].rawVal = create_optype2(OP_SBIW, addrreg, addroffset);
      i++;
      break;
    }
  case ADDR_CONST:
    {
      // POP AVR_FIXED_REG_2
      sandboxblk->instr[i].rawVal = create_optype4(OP_POP, AVR_FIXED_REG_2);
      i++;
      // POP AVR_FIXED_REG_1
      sandboxblk->instr[i].rawVal = create_optype4(OP_POP, AVR_FIXED_REG_1);
      i++;
      break;
    }
  default:
    break;
  }

  
#if 0
#ifndef SBX_FIX_REGS
  if (!skippushpop){
    // POP AVR_FIXED_REG_2
    sandboxblk->instr[i].rawVal = create_optype4(OP_POP, AVR_FIXED_REG_2);
    i++;
    // POP AVR_FIXED_REG_1
    sandboxblk->instr[i].rawVal = create_optype4(OP_POP, AVR_FIXED_REG_1);
    i++;
  }
#endif
#endif

#ifndef SBX_FIX_REGS
  // POP AVR_SCRATCH_REG
  sandboxblk->instr[i].rawVal = create_optype4(OP_POP, AVR_SCRATCH_REG);
  i++;
#endif

  sandboxblk->size = i * (sizeof(avr_instr_t));
  return;
}
//-------------------------------------------------------------------
static void avr_gen_ret_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx)
{
  int i;
  uint32_t jmpinstr;

  i = 0;
  
  jmpinstr = create_optype10(OP_JMP, KER_RET_CHECK_CODE);
  sandboxblk->instr[i].rawVal = (uint16_t)(jmpinstr >> 16);
  i++;
  sandboxblk->instr[i].rawVal = (uint16_t)jmpinstr;
  i++;


  sandboxblk->size = i * (sizeof(avr_instr_t));
  return;
}
//-------------------------------------------------------------------
static void avr_gen_icall_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx)
{
  int i;
  uint32_t callinstr;
  
  i = 0;
  
  callinstr = create_optype10(OP_CALL, KER_ICALL_CHECK_CODE);
  sandboxblk->instr[i].rawVal = (uint16_t)(callinstr >> 16);
  i++;
  sandboxblk->instr[i].rawVal = (uint16_t)callinstr;
  i++;
  
  sandboxblk->size = i * (sizeof(avr_instr_t));
  return;
}
