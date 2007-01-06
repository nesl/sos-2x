/**
 * \file sos_mod_verify.c
 * \brief Verify the safety of SOS module
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <sos_types.h>
#include <sos_module_types.h>
#include <codemem.h>
#include <memmap.h>
#include <memmap_checker_const.h>
#include <avrinstr.h>
#include <sos_mod_verify.h>
#include <cross_domain_cf.h>


//---------------------------------------------------
// STATIC FUNCTIONS
static inline int8_t verify_instr(avr_instr_t instr);
static inline int8_t patch_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset, 
				 uint16_t mod_lb, uint16_t mod_ub);

//---------------------------------------------------
int8_t ker_verify_module(codemem_t h, uint16_t init_offset, uint16_t code_size)
{
  uint16_t code_offset;
  avr_instr_t instr;
  uint16_t mod_lb, mod_ub;
  mod_lb = (ker_codemem_get_start_address(h) >> 1) + init_offset;
  mod_ub = (ker_codemem_get_start_address(h) >> 1) + code_size;
  // Single pass patch and verify
  for (code_offset = init_offset; code_offset < code_size; code_offset += sizeof(avr_instr_t)){
    ker_codemem_read(h, KER_DFT_LOADER_PID, &instr, sizeof(avr_instr_t), code_offset);
    if (verify_instr(instr) != SOS_OK){
      return -EINVAL;
    }
    if (patch_instr(instr, h, code_offset, mod_lb, mod_ub) != SOS_OK){
      return -EINVAL;
    }
    watchdog_reset();
  }
  // Flush final result
  ker_codemem_flush(h, KER_DFT_LOADER_PID);
  return SOS_OK;
}
//---------------------------------------------------
static inline int8_t verify_instr(avr_instr_t instr)
{
  static uint8_t twowordinstr = 0;

  // Mark two work instr.
  if (twowordinstr){
    twowordinstr = 0;
    return SOS_OK;
  }
  switch (instr.rawVal & OP_TYPE10_MASK){
  case OP_JMP:
  case OP_CALL:
    twowordinstr = 1;
    return SOS_OK;
  }
  // LDS Ok, STS Not allowed
  switch (instr.rawVal & OP_TYPE19_MASK){
  case OP_LDS:
    {
      twowordinstr = 1;
      return SOS_OK;
    }
  case OP_STS:
    {
      twowordinstr = 1;
      return -EINVAL;
    }
  }
  // No Store Instructions Allowed
  switch (instr.rawVal & OP_TYPE4_MASK){
  case OP_ST_X:
  case OP_ST_Y:
  case OP_ST_Z:
  case OP_ST_X_INC:
  case OP_ST_Y_INC:
  case OP_ST_Z_INC:
  case OP_ST_X_DEC:
  case OP_ST_Y_DEC:
  case OP_ST_Z_DEC:
    return -EINVAL;
  }
  switch (instr.rawVal & OP_TYPE14_MASK){
  case OP_STD_Y:
  case OP_STD_Z:
    return -EINVAL;
  }
  // No Ret or icall Instruction Allowed
  switch (instr.rawVal){
  case OP_RET:
    return -EINVAL;
  case OP_ICALL:
    return -EINVAL;
  }
  // Allow this instruction
  return SOS_OK;
}
//---------------------------------------------------
static inline int8_t patch_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset,
				 uint16_t mod_lb, uint16_t mod_ub)
{

#define TWO_WORD_FLAG 0x01
#define PATCH_JMP_RET_CHECK_FLAG 0x02
#define PATCH_CALL_MEMMAP_CHECK_FLAG  0x04
#define PATCH_CALL_ICALL_CHECK_FLAG 0x08
  
  static uint8_t patch_state = 0;
  uint16_t writeval;
  
  // Patch CALL or JMP Instruction Targets
  if (patch_state & TWO_WORD_FLAG){

    if (patch_state & PATCH_CALL_MEMMAP_CHECK_FLAG){
      writeval = (uint16_t)(ker_memmap_perms_check);
      ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			sizeof(avr_instr_t), code_offset);
    }
    else if (patch_state & PATCH_CALL_ICALL_CHECK_FLAG){
      writeval = (uint16_t)(ker_icall_check);
      ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval,
			sizeof(avr_instr_t), code_offset);
    }
    else if (patch_state & PATCH_JMP_RET_CHECK_FLAG){
      writeval = (uint16_t)(ker_restore_ret_addr);
      ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			sizeof(avr_instr_t), code_offset);      
    }
    patch_state = 0;
    return SOS_OK;
  }

  // Mark Calls to memmap checker
  // If we detect a PUSH R0, then 
  // we are into the memmap sandbox routine
  if ((instr.rawVal & OP_TYPE4_MASK) == OP_PUSH){
    if (0 == get_rd_5(instr.rawVal)){
      patch_state |= PATCH_CALL_MEMMAP_CHECK_FLAG;
      return SOS_OK;
    }
  }

// Patch calls to ker_save_ret_addr
  // All RCALL instruction targets must be patched (If they are not already patched !)
  // If RCALL target is NOT a CALL instruction, then bin-rewriter botched up 
  if ((instr.rawVal & OP_TYPE17_MASK) == OP_RCALL){
    avr_instr_t saveretcall[2];
    int16_t rcalloffset;
    int16_t reloffset = get_k_12(instr.rawVal) * 2;
    rcalloffset = (int16_t)code_offset + reloffset + sizeof(avr_instr_t);
    if (ker_codemem_read(h, KER_DFT_LOADER_PID, &saveretcall, 2*sizeof(avr_instr_t), rcalloffset) != SOS_OK)
      return -EINVAL;
    if ((saveretcall[0].rawVal & OP_TYPE10_MASK) != OP_CALL)
      return -EINVAL;
    if (saveretcall[1].rawVal != (uint16_t)ker_save_ret_addr){
      writeval = (uint16_t)ker_save_ret_addr;
      ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, sizeof(avr_instr_t), rcalloffset + sizeof(avr_instr_t));
    }
    return SOS_OK;
  }



  // Mark CALL and JMP Instructions
  switch (instr.rawVal & OP_TYPE10_MASK){
  case OP_JMP:
    patch_state |= TWO_WORD_FLAG;
    patch_state |= PATCH_JMP_RET_CHECK_FLAG;
    return SOS_OK;
  case OP_CALL:
    patch_state |= TWO_WORD_FLAG;
    // Check if the call is for mem map check
    if ((patch_state & PATCH_CALL_MEMMAP_CHECK_FLAG) == 0){
      uint16_t calladdr;
      // Read call address i.e. Read the word following the current word
      ker_codemem_read(h, KER_DFT_LOADER_PID, &calladdr, sizeof(avr_instr_t), code_offset + sizeof(avr_instr_t));
      // Check if call is internal to module
      if ((calladdr < mod_lb) || (calladdr > mod_ub)){
	// Check if call is into the system jump table
	if ((calladdr < SYS_JUMP_TBL_START) ||
	    (calladdr >= (SYS_JUMP_TBL_START + (SYS_JUMP_TBL_SIZE * 64)))){ 
	  // Patch this only if this address has not been already patched
	  if (calladdr != (uint16_t)ker_save_ret_addr)
	    patch_state |= PATCH_CALL_ICALL_CHECK_FLAG;
	}
      }
    }
    return SOS_OK;
  }
  // Mark other two word instructions
  switch (instr.rawVal & OP_TYPE19_MASK){
  case OP_LDS:
  case OP_STS:
    patch_state |= TWO_WORD_FLAG;
    return SOS_OK;
  }

  return SOS_OK;
}
//---------------------------------------------------

