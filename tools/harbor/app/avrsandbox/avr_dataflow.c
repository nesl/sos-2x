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
  return;
}
