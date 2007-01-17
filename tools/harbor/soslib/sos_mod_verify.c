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
#include <sfi_jumptable.h>

#ifdef MINIELF_LOADER
#include <melfloader.h>

//---------------------------------------------------
// STATIC FUNCTIONS
static inline int8_t verify_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset, 
				  uint16_t mod_lb, uint16_t mod_ub);
//---------------------------------------------------
int8_t ker_verify_module(codemem_t h)
{
  uint16_t code_offset;
  avr_instr_t instr;
  mod_header_ptr pmhdr;
  uint16_t mod_start_word_addr, mod_handler_sfi_word_addr, mod_handler_word_addr;
  melf_desc_t mdesc;
  Melf_Shdr progshdr;
  uint16_t mod_ub;


  melf_begin(&mdesc, h);
  mod_start_word_addr = (mdesc.base_addr >> 1); // Module begins here (Word Addr)
  melf_read_progbits_shdr(&mdesc, &progshdr);
  mod_ub = mod_start_word_addr + (uint16_t)(progshdr.sh_offset >> 1) 
    + (uint16_t)(progshdr.sh_size >> 1); // Module .text section ends here (Word Addr)

  pmhdr = ker_codemem_get_header_address(h);
  mod_handler_sfi_word_addr = sos_read_header_ptr(pmhdr, offsetof(mod_header_t, module_handler));
  mod_handler_word_addr = sfi_modtable_get_real_addr(mod_handler_sfi_word_addr); // Module .text starts here (mod_lb)
  

  // Single pass patch and verify
  for (code_offset = (mod_handler_word_addr - mod_start_word_addr) * sizeof(avr_instr_t); 
       code_offset < (progshdr.sh_offset + progshdr.sh_size); 
       code_offset += sizeof(avr_instr_t)){
    ker_codemem_read(h, KER_DFT_LOADER_PID, &instr, sizeof(avr_instr_t), code_offset);
    if (verify_instr(instr, h, code_offset, mod_handler_word_addr, mod_ub) != SOS_OK){
      return -EINVAL;
    }
    /*
    if (patch_instr(instr, h, code_offset, init_offset, code_size, mod_start_word_addr) != SOS_OK){
      return -EINVAL;
    }
    */
    watchdog_reset();
  }
  // Flush final result
  ker_codemem_flush(h, KER_DFT_LOADER_PID);
  return SOS_OK;
}
//---------------------------------------------------
static inline int8_t verify_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset, 
				  uint16_t mod_lb, uint16_t mod_ub)
{
#define TWO_WORD_FLAG                 0x01 //00000001
#define CALL_FLAG                     0x02 //00000010
#define JMP_FLAG                      0x04 //00000100
  static uint8_t verify_state = 0;
  uint16_t writeval;
  
  // Check for two word instructions
  if (verify_state & TWO_WORD_FLAG){
    // Check for JUMP instruction
    if (verify_state & JMP_FLAG){
      writeval = (uint16_t)(ker_restore_ret_addr);
      if (KER_RET_CHECK_CODE == instr.rawVal){
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			  sizeof(avr_instr_t), code_offset);
	goto verify_success;
      }
      if (writeval != instr.rawVal) return -EINVAL;
    }  
    
    // Check for CALL instruction
    if (verify_state & CALL_FLAG){
      // Check if call is internal to module
      if ((instr.rawVal >= mod_lb) && (instr.rawVal < mod_ub)){
	// Ram - TODO - Add check to see if the call target is a call to save_ret_addr
	// Also - Add a similar check for RCALL
	goto verify_success;
      }
      // Check if call is into the system jump table
      if (((instr.rawVal*sizeof(avr_instr_t)) >= SYS_JUMP_TBL_START) &&
	  ((instr.rawVal*sizeof(avr_instr_t)) < (SYS_JUMP_TBL_START + (SYS_JUMP_TBL_SIZE * 64)))){ 
	goto verify_success;
      }

      // MEMMAP XPTR
      writeval = (uint16_t)(ker_memmap_perms_check_xptr);
      if (KER_MEMMAP_PERMS_CHECK_X_CODE == instr.rawVal){
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			  sizeof(avr_instr_t), code_offset);
	goto verify_success;
      }
      if (instr.rawVal == writeval) goto verify_success;

      // MEMMAP YPTR
      writeval = (uint16_t)(ker_memmap_perms_check_yptr);
      if (KER_MEMMAP_PERMS_CHECK_Y_CODE == instr.rawVal){
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			  sizeof(avr_instr_t), code_offset);
	goto verify_success;
      }
      if (instr.rawVal == writeval) goto verify_success;

      // MEMMAP ZPTR
      writeval = (uint16_t)(ker_memmap_perms_check_zptr);
      if (KER_MEMMAP_PERMS_CHECK_Z_CODE == instr.rawVal){
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			  sizeof(avr_instr_t), code_offset);
	goto verify_success;
      }
      if (instr.rawVal == writeval) goto verify_success;

      // INTERNAL CALL - SAVE RET ADDR TO SAFE STACK
      writeval = (uint16_t)(ker_save_ret_addr);
      if (KER_INTCALL_CODE == instr.rawVal){
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			  sizeof(avr_instr_t), code_offset);
	goto verify_success;
      }
      if (instr.rawVal == writeval) goto verify_success;

      // ICALL_CHECK
      writeval = (uint16_t)(ker_icall_check);
      if (KER_ICALL_CHECK_CODE == instr.rawVal){
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, 
			  sizeof(avr_instr_t), code_offset);
	goto verify_success;
      }
      if (instr.rawVal == writeval) goto verify_success;
      return -EINVAL;
    }
  verify_success:
    verify_state = 0;
    return SOS_OK;
  }

  switch (instr.rawVal & OP_TYPE10_MASK){
  case OP_JMP:
    verify_state = TWO_WORD_FLAG | JMP_FLAG;
    return SOS_OK;
  case OP_CALL:
    verify_state = TWO_WORD_FLAG | CALL_FLAG;
    return SOS_OK;
  }
  // LDS Ok, STS Not allowed
  switch (instr.rawVal & OP_TYPE19_MASK){
  case OP_LDS:
    {
      verify_state = TWO_WORD_FLAG;
      return SOS_OK;
    }
  case OP_STS:
    {
      verify_state = TWO_WORD_FLAG;
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
  verify_state = 0;
  // Allow this instruction
  return SOS_OK;
}
//---------------------------------------------------



/*
static inline int8_t patch_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset, 
				 uint16_t init_offset, uint16_t code_size, uint16_t mod_start_word_addr)
{

#define TWO_WORD_FLAG                 0x01 //00000001
#define PATCH_JMP_RET_CHECK_FLAG      0x02 //00000010
#define PATCH_CALL_MEMMAP_CHECK_FLAG  0x04 //00000100
#define PATCH_CALL_ICALL_CHECK_FLAG   0x08 //00001000
#define INTERNAL_CALL_FLAG            0x10 //00010000
  
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
    else if (patch_state & INTERNAL_CALL_FLAG){
      avr_instr_t saveretcall[2];
      uint16_t calloffset;
      calloffset = (instr.rawVal - mod_start_word_addr) * sizeof(avr_instr_t);
      if (ker_codemem_read(h, KER_DFT_LOADER_PID, &saveretcall, 2*sizeof(avr_instr_t), calloffset) != SOS_OK)
	return -EINVAL;
      if ((saveretcall[0].rawVal & OP_TYPE10_MASK) != OP_CALL)
	return -EINVAL;
      if (saveretcall[1].rawVal != (uint16_t)ker_save_ret_addr){
	writeval = (uint16_t)ker_save_ret_addr;
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, sizeof(avr_instr_t), calloffset + sizeof(avr_instr_t));
      }
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
      uint16_t calladdr, mod_lb, mod_ub;
      mod_lb = mod_start_word_addr + (init_offset >> 1);
      mod_ub = mod_lb + (code_size >> 1);
      // Read call address i.e. Read the word following the current word
      ker_codemem_read(h, KER_DFT_LOADER_PID, &calladdr, sizeof(avr_instr_t), code_offset + sizeof(avr_instr_t));
      // Check if call is internal to module
      if ((calladdr < mod_lb) || (calladdr > mod_ub)){
	// Check if call is into the system jump table
	if (((calladdr*sizeof(avr_instr_t)) < SYS_JUMP_TBL_START) ||
	    ((calladdr*sizeof(avr_instr_t)) >= (SYS_JUMP_TBL_START + (SYS_JUMP_TBL_SIZE * 64)))){ 
	  // Patch this only if this address has not been already patched
	  if (calladdr != (uint16_t)ker_save_ret_addr)
	    patch_state |= PATCH_CALL_ICALL_CHECK_FLAG;
	}
      }
      else {
	patch_state |= INTERNAL_CALL_FLAG;
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
*/

#else
//---------------------------------------------------
// STATIC FUNCTIONS
/*
static inline int8_t verify_instr(avr_instr_t instr);
static inline int8_t patch_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset, 
				 uint16_t init_offset, uint16_t code_size, uint16_t mod_start_word_addr);
*/
//---------------------------------------------------
/*
int8_t ker_verify_module(codemem_t h, uint16_t init_offset, uint16_t code_size)
{
  uint16_t code_offset;
  avr_instr_t instr;
  uint16_t mod_start_word_addr;

  mod_start_word_addr = (ker_codemem_get_start_address(h) >> 1);
  // Single pass patch and verify
  for (code_offset = init_offset; code_offset < code_size; code_offset += sizeof(avr_instr_t)){
    ker_codemem_read(h, KER_DFT_LOADER_PID, &instr, sizeof(avr_instr_t), code_offset);
    if (verify_instr(instr) != SOS_OK){
      return -EINVAL;
    }
    if (patch_instr(instr, h, code_offset, init_offset, code_size, mod_start_word_addr) != SOS_OK){
      return -EINVAL;
    }
    watchdog_reset();
  }
  // Flush final result
  ker_codemem_flush(h, KER_DFT_LOADER_PID);
  return SOS_OK;
}
*/
//---------------------------------------------------
/*
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
*/
//---------------------------------------------------
/*
static inline int8_t patch_instr(avr_instr_t instr, codemem_t h, uint16_t code_offset, 
				 uint16_t init_offset, uint16_t code_size, uint16_t mod_start_word_addr)
{

#define TWO_WORD_FLAG                 0x01 //00000001
#define PATCH_JMP_RET_CHECK_FLAG      0x02 //00000010
#define PATCH_CALL_MEMMAP_CHECK_FLAG  0x04 //00000100
#define PATCH_CALL_ICALL_CHECK_FLAG   0x08 //00001000
#define INTERNAL_CALL_FLAG            0x10 //00010000
  
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
    else if (patch_state & INTERNAL_CALL_FLAG){
      avr_instr_t saveretcall[2];
      uint16_t calloffset;
      calloffset = (instr.rawVal - mod_start_word_addr) * sizeof(avr_instr_t);
      if (ker_codemem_read(h, KER_DFT_LOADER_PID, &saveretcall, 2*sizeof(avr_instr_t), calloffset) != SOS_OK)
	return -EINVAL;
      if ((saveretcall[0].rawVal & OP_TYPE10_MASK) != OP_CALL)
	return -EINVAL;
      if (saveretcall[1].rawVal != (uint16_t)ker_save_ret_addr){
	writeval = (uint16_t)ker_save_ret_addr;
	ker_codemem_write(h, KER_DFT_LOADER_PID, &writeval, sizeof(avr_instr_t), calloffset + sizeof(avr_instr_t));
      }
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
      uint16_t calladdr, mod_lb, mod_ub;
      mod_lb = mod_start_word_addr + (init_offset >> 1);
      mod_ub = mod_start_word_addr + (code_size >> 1);
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
      else {
	patch_state |= INTERNAL_CALL_FLAG;
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
*/
#endif //MINIELF_LOADER
//---------------------------------------------------

