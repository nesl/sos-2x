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
typedef struct _str_ptr_reg {
  int binit;
  int reg;
  int lvalue;
  int uvalue;
  int cvalue;
} ptr_reg_t;

//-------------------------------------------------------------------
// STATIC FUNCTIONS
static inline void init_ptr_reg(ptr_reg_t* ptr, int reg);
static void update_ptr_reg(ptr_reg_t* ptr, avr_instr_t* instr);
//-------------------------------------------------------------------
void avr_dataflow_basic_block(basicblk_t* cblk)
{
  int i;
  ptr_reg_t x, y, z;
  init_ptr_reg(&x, R26);
  init_ptr_reg(&y, R28);
  init_ptr_reg(&z, R30);
  
  for (i = 0; i < (int)(cblk->size/sizeof(avr_instr_t)); i++){
    update_ptr_reg(&x, &cblk->instr[i]);
    update_ptr_reg(&y, &cblk->instr[i]);
    update_ptr_reg(&z, &cblk->instr[i]);
  }


  return;
}
//-------------------------------------------------------------------
static void init_ptr_reg(ptr_reg_t* ptr, int reg)
{
  ptr->binit = 0; ptr->lvalue = 0; ptr->uvalue = 0; ptr->cvalue = 0;
  ptr->reg = reg;
  return;
}
//-------------------------------------------------------------------
static void update_ptr_reg(ptr_reg_t* ptr, avr_instr_t* instr)
{
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
  // OPTYEP 6: BLD
  return;
}
