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

typedef struct _str_ptr_reg {
  int binit;
  int lvalue;
  int uvalue;
  int cvalue;
} ptr_reg_t;

void avr_dataflow_basic_block(basicblk_t* cblk)
{
  ptr_reg_t x, y, z;
  x.binit = 0; x.lvalue = 0; x.uvalue = 0; x.cvalue = 0;
  y.binit = 0; y.lvalue = 0; y.uvalue = 0; y.cvalue = 0;
  z.binit = 0; z.lvalue = 0; z.uvalue = 0; z.cvalue = 0;
  return;
}
//-------------------------------------------------------------------
