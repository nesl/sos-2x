/**
 * \file avr_control_flow.c
 * \brief Fix control flow of program after introducing sandbox
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
// STATIC FUNCTIONS
static void avr_update_cf_instr(avr_instr_t* instr, basicblk_t* cblk, int* cfupdateflag);
static void avr_fix_branch_instr(avr_instr_t* instrin, basicblk_t* cblk, uint16_t opcode, uint8_t bitpos);
static void avr_fix_cpse(avr_instr_t* instrin, basicblk_t* cblk);
static void avr_fix_skip_instr(avr_instr_t* instrin, basicblk_t* cblk);
//-------------------------------------------------------------------
void avr_update_cf(bblklist_t* blist, uint32_t startaddr)
{
  basicblk_t* cblk;
  uint32_t curraddr;
  int gcfupdateflag;
  int numpass = 0;
  DEBUG("========== Control Flow Update ==========\n");
  do{
    numpass++;
    DEBUG("Starting CF update pass %d ...\n", numpass);
    gcfupdateflag = 0;
    curraddr = startaddr;
    for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
      // Update the start address of the basic block
      // after the sandbox instructions have been introduced
      cblk->newaddr = curraddr;
      curraddr += cblk->newsize;    
    }
    
    for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
      avr_instr_t* instr;
      int numinstr;
      numinstr = cblk->newsize/sizeof(avr_instr_t);
      if (NULL == cblk->branch) continue;
      if (cblk->flag & TWO_WORD_INSTR_FLAG)
	instr = &(cblk->newinstr[numinstr - 2]);
      else
	instr = &(cblk->newinstr[numinstr-1]);
      avr_update_cf_instr(instr, cblk, &gcfupdateflag);
    }
  }while (gcfupdateflag);
  return;
}
//-------------------------------------------------------------------
static void avr_update_cf_instr(avr_instr_t* instrin, basicblk_t* cblk, int* gcfupdateflag)
{
  avr_instr_t* instr;
  instr = instrin;
  // OPTYPE 10
  if (((instr->rawVal & OP_TYPE10_MASK) == OP_JMP)||
      ((instr->rawVal & OP_TYPE10_MASK) == OP_CALL))
    {      
      uint32_t newinstr;
      uint32_t braddr;
      braddr = (cblk->branch)->newaddr;
      newinstr = create_optype10(instr->rawVal & OP_TYPE10_MASK, braddr);
      instr->rawVal = (uint16_t)(newinstr >> 16);
      instr++;
      instr->rawVal = (uint16_t)(newinstr);
      return;
    }
  // OPTYPE6 OPTYPE 1, 6 and 11
  if (((instr->rawVal & OP_TYPE1_MASK) == OP_CPSE)||
      ((instr->rawVal & OP_TYPE6_MASK) == OP_SBRC)||
      ((instr->rawVal & OP_TYPE6_MASK) == OP_SBRS)||
      ((instr->rawVal & OP_TYPE11_MASK) == OP_SBIC)||
      ((instr->rawVal & OP_TYPE11_MASK) == OP_SBIS))
    {
      basicblk_t* fallblk;
      fallblk = cblk->fall;
      if (fallblk->newsize != fallblk->size){
	DEBUG("SKIP INSTR.: Old Size: %d Old Addr.: 0x%x New Size: %d. New Addr: 0x%x\n", (int)fallblk->size, (int)(cblk->addr + cblk->size), (int)fallblk->newsize, (int)(cblk->newaddr+cblk->newsize));
	*gcfupdateflag = 1;
	if ((instr->rawVal & OP_TYPE1_MASK) == OP_CPSE){
	  avr_fix_cpse(instr, cblk);
	  return;
	}
	avr_fix_skip_instr(instr, cblk);
      }
      return;
    }
  // OPTYPE 7
  if (((instr->rawVal & OP_TYPE7_MASK) == OP_BRBC)||
      ((instr->rawVal & OP_TYPE7_MASK) == OP_BRBS))
    {
      int k;
      uint16_t newinstr;
      k = (int)(((int)(cblk->branch->newaddr) - (int)(cblk->newaddr + cblk->newsize))/2);
      if ((k < -64)||(k > 63)){
	DEBUG("OPTYPE7: Addr: 0x%x Branch: 0x%x k: %d\n", (int)(cblk->newaddr + cblk->newsize), (int)cblk->branch->newaddr, k);
	*gcfupdateflag = 1;
	if ((instr->rawVal & OP_TYPE7_MASK) == OP_BRBC)
	  avr_fix_branch_instr(instr, cblk, OP_SBRC, get_optype7_s(instr));
	else if ((instr->rawVal & OP_TYPE7_MASK) == OP_BRBS)
	  avr_fix_branch_instr(instr, cblk, OP_SBRS, get_optype7_s(instr));
	return;
      }
      newinstr = create_optype7(instr->rawVal & OP_TYPE7_MASK, (int8_t)k, get_optype7_s(instr));
      instr->rawVal = newinstr;
      return;
    }
  // OPTYPE 8
  switch (instr->rawVal & OP_TYPE8_MASK){
  case OP_BRCC: case OP_BRCS: case OP_BREQ: case OP_BRGE: case OP_BRHC: case OP_BRHS: 
  case OP_BRID: case OP_BRIE: case OP_BRLT: case OP_BRMI: case OP_BRNE: case OP_BRPL: 
  case OP_BRTC: case OP_BRTS: case OP_BRVC: case OP_BRVS: 
    {
      int k;
      uint16_t newinstr;
      k = (int)(((int)(cblk->branch->newaddr) - (int)(cblk->newaddr + cblk->newsize))/2);
      if ((k < -64)||(k > 63)){
	DEBUG("OPTYPE8: Addr: 0x%x Branch: 0x%x k: %d\n", (int)(cblk->newaddr + cblk->newsize),(int)cblk->branch->newaddr, k);
	*gcfupdateflag = 1;
	switch (instr->rawVal & OP_TYPE8_MASK){
	  // BRBC
	case OP_BRCC: avr_fix_branch_instr(instr, cblk, OP_SBRC, 0); break;
	case OP_BRNE: avr_fix_branch_instr(instr, cblk, OP_SBRC, 1); break;
	case OP_BRPL: avr_fix_branch_instr(instr, cblk, OP_SBRC, 2); break;
	case OP_BRVC: avr_fix_branch_instr(instr, cblk, OP_SBRC, 3); break;
	case OP_BRGE: avr_fix_branch_instr(instr, cblk, OP_SBRC, 4); break;
	case OP_BRHC: avr_fix_branch_instr(instr, cblk, OP_SBRC, 5); break;
	case OP_BRTC: avr_fix_branch_instr(instr, cblk, OP_SBRC, 6); break;
	case OP_BRID: avr_fix_branch_instr(instr, cblk, OP_SBRC, 7); break;
	  // BRBS
	case OP_BRCS: avr_fix_branch_instr(instr, cblk, OP_SBRS, 0); break;
	case OP_BREQ: avr_fix_branch_instr(instr, cblk, OP_SBRS, 1); break;
	case OP_BRMI: avr_fix_branch_instr(instr, cblk, OP_SBRS, 2); break;
	case OP_BRVS: avr_fix_branch_instr(instr, cblk, OP_SBRS, 3); break;
	case OP_BRLT: avr_fix_branch_instr(instr, cblk, OP_SBRS, 4); break;
	case OP_BRHS: avr_fix_branch_instr(instr, cblk, OP_SBRS, 5); break;
	case OP_BRTS: avr_fix_branch_instr(instr, cblk, OP_SBRS, 6); break;
	case OP_BRIE: avr_fix_branch_instr(instr, cblk, OP_SBRS, 7); break;
	}
	return;
      }
      newinstr = create_optype8(instr->rawVal & OP_TYPE8_MASK, (int8_t)k);
      instr->rawVal = newinstr;
      return;
    }
  }
  // OPTYPE 17
  if (((instr->rawVal & OP_TYPE17_MASK) == OP_RJMP)||
      ((instr->rawVal & OP_TYPE17_MASK) == OP_RCALL))
    {
      int k;
      uint16_t newinstr;
      k = (int)(((int)(cblk->branch->newaddr) - (int)(cblk->newaddr + cblk->newsize))/2);
      if ((k < -2048) || (k > 2047)){
	uint32_t newcalljmpinstr;
	uint32_t calljmpaddr;
	DEBUG("OPTYPE17: Addr: 0x%x Branch: 0x%x k: %d.\n", (int)(cblk->newaddr + cblk->newsize), (int)cblk->branch->newaddr, k);
	*gcfupdateflag = 1;
	cblk->flag = TWO_WORD_INSTR_FLAG;
	calljmpaddr = ((cblk->branch)->newaddr);
	if ((instr->rawVal & OP_TYPE17_MASK) == OP_RCALL){
	  cblk->flag |= CALL_INSTR_FLAG;
	  newcalljmpinstr = create_optype10(OP_CALL, calljmpaddr);
	}
	else{
	  cblk->flag |= JMP_INSTR_FLAG;
	  newcalljmpinstr = create_optype10(OP_JMP, calljmpaddr);
	}
	instr->rawVal = (uint16_t)(newcalljmpinstr >> 16);
	instr++;
	cblk->newsize += sizeof(avr_instr_t);
	instr->rawVal = (uint16_t)(newcalljmpinstr);
	return;
	/*
	fprintf(stderr, "OPTYPE17: Addr: 0x%x Branch: 0x%x k: %d.\n", (int)(cblk->newaddr + cblk->newsize), (int)cblk->branch->newaddr, k);
	exit(EXIT_FAILURE);
	*/
      }
      newinstr = create_optype17(instr->rawVal & OP_TYPE17_MASK, (int16_t)k);
      instr->rawVal = newinstr;
      return;
    }
  return;
}
//-------------------------------------------------------------------
static void avr_fix_cpse(avr_instr_t* instrin, basicblk_t* cblk)
{
  uint32_t curraddr;
  avr_instr_t* instr;
  uint8_t d, r;
  int k;
  
  instr = instrin;
  curraddr = cblk->newsize + cblk->newaddr - sizeof(avr_instr_t);
  d = get_optype1_dreg(instr);
  r = get_optype1_rreg(instr);
  instr->rawVal = create_optype1(OP_CP, d, r);

  instr++;
  cblk->newsize += sizeof(avr_instr_t);
  curraddr += sizeof(avr_instr_t);
  k = (int)(((int)cblk->branch->newaddr - (int)curraddr - (int)sizeof(avr_instr_t))/2);
  instr->rawVal = create_optype8(OP_BREQ, (int8_t)k);

  return;
}

//-------------------------------------------------------------------
static void avr_fix_skip_instr(avr_instr_t* instrin, basicblk_t* cblk)
{
  uint32_t curraddr;
  avr_instr_t* instr;
  int k;

  instr = instrin;
  curraddr = cblk->newsize + cblk->newaddr - sizeof(avr_instr_t);

  instr++;
  cblk->newsize += sizeof(avr_instr_t);
  curraddr += sizeof(avr_instr_t);
  instr->rawVal = create_optype17(OP_RJMP, 1);

  instr++;
  cblk->newsize += sizeof(avr_instr_t);
  curraddr += sizeof(avr_instr_t);
  k = (int)(((int)cblk->branch->newaddr - (int)curraddr - (int)sizeof(avr_instr_t))/2);
  instr->rawVal = create_optype17(OP_RJMP, (int16_t)k);

  return;
}
//-------------------------------------------------------------------
static void avr_fix_branch_instr(avr_instr_t* instrin, basicblk_t* cblk, uint16_t opcode, uint8_t bitpos)
{
  uint32_t curraddr;
  avr_instr_t* instr;
  int k;
  instr = instrin;
  curraddr = cblk->newsize + cblk->newaddr - sizeof(avr_instr_t);
  instr->rawVal = create_optype13(OP_IN, AVR_SCRATCH_REG, AVR_SREG);

  instr++;
  cblk->newsize += sizeof(avr_instr_t);
  curraddr += sizeof(avr_instr_t);
  instr->rawVal = create_optype6(opcode & OP_TYPE6_MASK, AVR_SCRATCH_REG, bitpos);
  
  instr++;
  cblk->newsize += sizeof(avr_instr_t);
  curraddr += sizeof(avr_instr_t);
  instr->rawVal = create_optype17(OP_RJMP, 1);

  instr++;
  cblk->newsize += sizeof(avr_instr_t);
  curraddr += sizeof(avr_instr_t);
  k = (int)(((int)cblk->branch->newaddr - (int)curraddr - (int)sizeof(avr_instr_t))/2);
  instr->rawVal = create_optype17(OP_RJMP, (int16_t)k);

  return;
}
