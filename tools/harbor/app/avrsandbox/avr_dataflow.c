/**
 * \file avr_dataflow.c
 * \brief Dataflow optimizations for sandbox routines
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>

#include <avrinstr.h>
#include <avrinstmatch.h>
#include <avrbinparser.h>
#include <basicblock.h>
#include <avrsandbox.h>

//-------------------------------------------------------------------
// TYPEDEFS
/*
typedef enum _enum_val_type {
  NO_VAL  = 0,
  REG_VAL = 1,
  MEM_VAL = 2,
} valtype_t;

typedef struct _str_ptr_reg {
  int reg;
  valtype_t type;
  union {
    int regval;
    int memAddr;
  };
} ptr_reg_t;
*/

typedef struct _str_range{
  int min;
  int max;
} range_t;

#define MAX_RANGES 10
typedef struct _str_ptr_reg {
  int reg;
  int coffset;
  int st_active;
  range_t crange;
  int num_ranges;
  range_t ranges[MAX_RANGES];
} ptr_reg_t;

//-------------------------------------------------------------------
// STATIC FUNCTIONS
static inline void init_ptr_reg(ptr_reg_t* ptr, int reg);
static void print_ptr_reg(ptr_reg_t* ptr);
static void ptr_reset(ptr_reg_t* ptr);
static void ptr_update_range(ptr_reg_t* ptr, int st_offset);
static void update_ptr_reg(ptr_reg_t* ptr, avr_instr_t* instr);
//-------------------------------------------------------------------
void avr_dataflow_basic_block(basicblk_t* cblk)
{
  int i;
  ptr_reg_t x, y, z;
  init_ptr_reg(&x, R26);
  //  init_ptr_reg(&xh, R27);
  init_ptr_reg(&y, R28);
  //  init_ptr_reg(&yh, R29);
  init_ptr_reg(&z, R30);
  //  init_ptr_reg(&zh, R31);
  
  for (i = 0; i < (int)(cblk->size/sizeof(avr_instr_t)); i++){
    DEBUG("0x%x :", (int)(cblk->addr + (i * sizeof(avr_instr_t))));
    update_ptr_reg(&x, &cblk->instr[i]);
    //    update_ptr_reg(&xh, &cblk->instr[i]);
    update_ptr_reg(&y, &cblk->instr[i]);
    //    update_ptr_reg(&yh, &cblk->instr[i]);
    update_ptr_reg(&z, &cblk->instr[i]);
    //    update_ptr_reg(&zh, &cblk->instr[i]);
    DEBUG("\n");
  }
  print_ptr_reg(&x);
  print_ptr_reg(&y);
  print_ptr_reg(&z);

  return;
}
//-------------------------------------------------------------------
static void init_ptr_reg(ptr_reg_t* ptr, int reg)
{
  ptr->coffset = 0;
  ptr->reg = reg;
  ptr->num_ranges = 0;
  ptr->st_active = 0;
  return;
}
//-------------------------------------------------------------------
static void print_ptr_reg(ptr_reg_t* ptr)
{
  int i;
#ifdef DBGMODE
  printreg(ptr->reg);
#endif
  DEBUG(": \n");
  for (i = 0; i < ptr->num_ranges; i++)
    DEBUG("Ndx: %d  Min: %d  Max: %d\n", i, ptr->ranges[i].min, ptr->ranges[i].max);
  return;
}
//-------------------------------------------------------------------
static void ptr_reset(ptr_reg_t* ptr)
{
  ptr->coffset = 0;
  if (ptr->st_active == 0) return;
  if (MAX_RANGES == ptr->num_ranges){
    fprintf(stderr, "ptr_update_ranges: Max. limit reached.\n");
    exit(EXIT_FAILURE);
  }
  ptr->ranges[ptr->num_ranges].min = ptr->crange.min;
  ptr->ranges[ptr->num_ranges].max = ptr->crange.max;
  ptr->num_ranges++;
  ptr->st_active = 0;
  return;
}
//-------------------------------------------------------------------
static void ptr_update_range(ptr_reg_t* ptr, int st_offset)
{
  if (0 == ptr->st_active){
    ptr->crange.min = ptr->coffset + st_offset;
    ptr->crange.max = ptr->coffset + st_offset;
    ptr->st_active = 1;
    return;
  }
  if ((ptr->coffset + st_offset) < ptr->crange.min){
    ptr->crange.min = ptr->coffset + st_offset;
  }
  if ((ptr->coffset + st_offset) > ptr->crange.max){
    ptr->crange.max = ptr->coffset + st_offset;
  }
  return;
}

//-------------------------------------------------------------------
static void update_ptr_reg(ptr_reg_t* ptr, avr_instr_t* instr)
{
  static uint8_t twowordinstr = 0;
  // Arithmetic and Logic Instructions
  // OPTYPE 1: ADD, ADC, SUB, SBC, AND, OR, EOR,  
  // OPTYPE 2: ADIW, SBIW
  // OPTYPE 3: SUBI, SBCI, ANDI, ORI, SBR, 
  // OPTYPE 4: COM, NEG, INC, DEC, 
  // OPTYPE 18: SER
  // Data Transfer Instructions
  // OPTYPE 1: MOV,
  // OPTYPE 3: LDI,
  // OPTYPE 4: LD X, LD X+, LD -X, LD Y, LD Y+, LD -Y, LD Z, LD Z+, LD -Z
  //           ST X+, ST -X, ST Y+, ST -Y, ST Z+, ST -Z, 
  //           LPM_Z, LPM Z+, ELPM_Z, ELPM_Z+
  //           POP
  // OPTYPE 13: IN,
  // OPTYPE 14: LDD Y+q, LDD Z+q,
  // OPTYPE 16: MOVW,
  // OPTYPE 19: LDS,
  // Bit and Bit-Test Instructions
  // OPTYPE 4: LSR, ROR, ASR, SWAP
  // OPTYPE 6: BLD 

  // 1, 2, 3, 4, 6, 13, 14, 16, 18, 19

  // Handle two word instructions
  if (twowordinstr){
    twowordinstr = 0;
    return;
  }
  if (match_optype19(instr) == 0)
    twowordinstr = 1;
  if (match_optype10(instr) == 0){
    twowordinstr = 1;
    return;
  }

  // OPTYPE 1: ADD, ADC, SUB, SBC, AND, OR, EOR, MOV
  switch (instr->rawVal & OP_TYPE1_MASK){
  case OP_ADD: case OP_ADC: case OP_SUB: case OP_SBC:
  case OP_AND: case OP_OR: case OP_EOR: case OP_MOV:
    {
      if ((get_optype1_dreg(instr) == ptr->reg) || 
	  (get_optype1_dreg(instr) == (ptr->reg + 1)))
	{	  
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  } 
  // OPTYPE 3: SUBI, SBCI, ANDI, ORI, LDI
  switch (instr->rawVal & OP_TYPE3_MASK){
  case OP_SUBI: case OP_SBCI: case OP_ANDI:
  case OP_ORI: case OP_LDI:
    {
      if  ((get_optype3_dreg(instr) == ptr->reg) || 
	   (get_optype3_dreg(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 4
  switch (instr->rawVal & OP_TYPE4_MASK){
  case OP_COM: case OP_NEG: case OP_INC: case OP_DEC:
  case OP_LSR: case OP_ROR: case OP_ASR: case OP_SWAP:
  case OP_LD_X: case OP_LD_X_INC: case OP_LD_X_DEC:
  case OP_ST_X_INC: case OP_ST_X_DEC:
  case OP_LD_Y: case OP_LD_Y_INC: case OP_LD_Y_DEC:
  case OP_ST_Y_INC: case OP_ST_Y_DEC:
  case OP_LD_Z: case OP_LD_Z_INC: case OP_LD_Z_DEC:
  case OP_ST_Z_INC: case OP_ST_Z_DEC:
  case OP_LPM_Z: case OP_LPM_Z_INC:
  case OP_ELPM_Z: case OP_ELPM_Z_INC:
  case OP_POP:
    {
      if  ((get_optype4_dreg(instr) == ptr->reg) || 
	   (get_optype4_dreg(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 6: BLD
  switch (instr->rawVal & OP_TYPE6_MASK){
  case OP_BLD:
    {
      if  ((get_optype6_dreg(instr) == ptr->reg) || 
	   (get_optype6_dreg(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 13: IN
  switch (instr->rawVal & OP_TYPE13_MASK){
  case OP_IN:
    {
      if  ((get_optype13_d(instr) == ptr->reg) || 
	   (get_optype13_d(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 14: LDD Y+q, LDd Z+q
  switch (instr->rawVal & OP_TYPE14_MASK){
  case OP_LDD_Y: case OP_LDD_Z:
    {
      if  ((get_optype14_d(instr) == ptr->reg) || 
	   (get_optype14_d(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 16: MOVW
  switch (instr->rawVal & OP_TYPE16_MASK){
  case OP_MOVW:
    {
      if  (get_optype16_d(instr) == ptr->reg) 
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 18: SER
  switch (instr->rawVal & OP_TYPE18_MASK){
  case OP_SER:
    {
      if  ((get_optype18_d(instr) == ptr->reg) || 
	   (get_optype18_d(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }
  // OPTYPE 19: LDS
  switch (instr->rawVal & OP_TYPE19_MASK){
  case OP_LDS:
    {
      if  ((get_optype19_d(instr) == ptr->reg) || 
	   (get_optype19_d(instr) == (ptr->reg + 1)))
	{
	  ptr_reset(ptr);
	  return;
	}
    }
  default: break;
  }


  // OPTYPE 2: ADIW, SBIW
  if (get_optype2_dreg(instr) == ptr->reg){
    switch(instr->rawVal & OP_TYPE2_MASK){
    case OP_ADIW:
      ptr->coffset += get_optype2_k(instr);
      break;
    case OP_SBIW:
      ptr->coffset -= get_optype2_k(instr);
      break;
    default:
      break;
    }
    return;
  }

  // OPTYPE 4
  switch (instr->rawVal & OP_TYPE4_MASK){
  case OP_LD_X_INC:
    {
      if (R26 == ptr->reg){
	ptr->coffset++;
	return;
      }
    }
  case OP_LD_X_DEC:
    {
      if (R26 == ptr->reg){
	ptr->coffset--;
	return;
      }
    }
  case OP_LD_Y_INC:
    {
      if (R28 == ptr->reg){
	ptr->coffset++;
	return;
      }
    }
  case OP_LD_Y_DEC:
    {
      if (R28 == ptr->reg){
	ptr->coffset--;
	return;
      }
    }
  case OP_LD_Z_INC:
    {
      if (R30 == ptr->reg){
	ptr->coffset++;
	return;
      }
    }
  case OP_LD_Z_DEC:
    {
      if (R30 == ptr->reg){
	ptr->coffset--;
	return;
      }
    }
  case OP_LPM_Z_INC:
    {
      if (R30 == ptr->reg){
	ptr->coffset++;
	return;
      }
    }
  case OP_ELPM_Z_INC:
    {
      if (R30 == ptr->reg){
	ptr->coffset++;
	return;
      }
    }
  case OP_ST_X:
    {
      if (R26 == ptr->reg){
	ptr_update_range(ptr, 0);
	return;
      }
    }
  case OP_ST_X_INC:
    {
      if (R26 == ptr->reg){
	ptr_update_range(ptr, 0);
	ptr->coffset++;
	return;
      }
    }
  case OP_ST_X_DEC:
    {
      if (R26 == ptr->reg){
	ptr->coffset--;
	ptr_update_range(ptr, 0);
	return;
      }
    }
  case OP_ST_Y:
    {
      if (R28 == ptr->reg){
	ptr_update_range(ptr, 0);
	return;
      }
    }
  case OP_ST_Y_INC:
    {
      if (R28 == ptr->reg){
	ptr_update_range(ptr, 0);
	ptr->coffset++;
	return;
      }
    }
  case OP_ST_Y_DEC:
    {
      if (R28 == ptr->reg){
	ptr->coffset--;
	ptr_update_range(ptr, 0);
	return;
      }
    }
  case OP_ST_Z:
    {
      if (R30 == ptr->reg){
	ptr_update_range(ptr, 0);
	return;
      }
    }
  case OP_ST_Z_INC:
    {
      if (R30 == ptr->reg){
	ptr_update_range(ptr, 0);
	ptr->coffset++;
	return;
      }
    }
  case OP_ST_Z_DEC:
    {
      if (R30 == ptr->reg){
	ptr->coffset--;
	ptr_update_range(ptr, 0);
	return;
      }
    }
  default:
    break;
  }

  switch (instr->rawVal & OP_TYPE14_MASK){
  case OP_STD_Y:
    {
      if (R28 == ptr->reg){
	ptr_update_range(ptr, get_optype14_q(instr));
	return;
      }
    }
  case OP_STD_Z:
    {
      if (R30 == ptr->reg){
	ptr_update_range(ptr, get_optype14_q(instr));
	return;
      }
    }
  }

  return;
}
