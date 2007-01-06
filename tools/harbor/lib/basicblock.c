/**
 * \file basicblock.c
 * \brief Basic Block Manipulation Library
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <linklist.h>
#include <avrinstr.h>
#include <avrinstmatch.h>
#include <avrbinparser.h>
#include <basicblock.h>
#include <fileutils.h>
#include <sos_mod_header_patch.h>

//#define DBGMODE
//#define DBG_FIND_BLK_BND
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif


//--------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------
typedef struct _succ_str {
  uint8_t branchflag;
  uint32_t branchaddr;
  uint8_t fallflag;
  uint32_t falladdr;
  uint8_t calljmpflag;  // Flag set if last instr. is JMP or CALL (Two word CF instr.)
} succ_t;

#define	Flip_int16(type)  (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))


//--------------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------------
static int8_t insert_blkbnd(bblklist_t* blist, uint32_t addr);
static void find_block_boundaries(file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr);
static uint8_t find_next_instr_size(file_desc_t* fdesc);
static void link_basic_blocks(file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr);
static void fill_basic_block(file_desc_t* fdesc, basicblk_t* currblk, basicblk_t* nextblk, uint32_t currAddr);
static void find_succ(file_desc_t* fdesc, uint32_t currAddr, avr_instr_t* instr, succ_t* succ);


//static void sort_block_boundaries(bblklist_t* blist);
//static int ltcmp_blkbnd(list_t* lhop, list_t* rhop);
//static void swap_blkbnd(list_t* a, list_t* b);

//--------------------------------------------------------------
bblklist_t* create_cfg(file_desc_t* fdesc, uint32_t startaddr)
{
  bblklist_t* blist;


  if ((blist = (bblklist_t*)malloc(sizeof(bblklist_t))) == NULL){
    fprintf(stderr, "create_cfg: Malloc out of memory.\n");
    exit(EXIT_FAILURE);
  }
  blist->blk_1 = NULL;
  blist->blk_n = NULL;
  blist->cnt = 0;
  find_block_boundaries(fdesc, blist, startaddr);
#ifdef DBGMODE
  disp_blkbndlist(blist);
#endif
  link_basic_blocks(fdesc, blist, startaddr);
#ifdef DBGMODE
  disp_blocks(blist, startaddr);
#endif

  return blist;
}
//--------------------------------------------------------------
static void find_block_boundaries(file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr)
{
  avr_instr_t instr;
  uint32_t currAddr;
  succ_t succ;

  // Set the file pointer to the start of first instruction
  obj_file_seek(fdesc, startaddr, SEEK_SET);

  // Insert the first boundary
  insert_blkbnd(blist, startaddr);
#if defined(DBGMODE) && defined(DBG_FIND_BLK_BND)
  printf("\n");
#endif

  currAddr = startaddr;
  succ.branchflag = 0;
  succ.fallflag = 0;

  // Start reading the instructions - First pass through the file
  while (obj_file_read(fdesc, &instr, sizeof(avr_instr_t), 1) != 0){
#ifdef BBIG_ENDIAN
    instr.rawVal = Flip_int16(instr.rawVal);
#endif

#if defined(DBGMODE) && defined(DBG_FIND_BLK_BND)
    printf("0x%5x: %4x ", currAddr, instr.rawVal);
    decode_avr_instr_word(&instr);
#endif

    find_succ(fdesc, currAddr, &instr, &succ);

    if (succ.branchflag == 1){
      insert_blkbnd(blist, succ.branchaddr);
    }
    if (succ.fallflag == 1){
      insert_blkbnd(blist, succ.falladdr);
    }
    currAddr += 2;
#if defined(DBGMODE) && defined(DBG_FIND_BLK_BND)
    printf("\n");
#endif

  }
  // Insert a dummy boundary at the last address
  insert_blkbnd(blist, currAddr);
  // Basic Block with StartAddress
  blist->blk_st = find_block(blist, startaddr);
  return;
}
//--------------------------------------------------------------
static void link_basic_blocks(file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr)
{
  avr_instr_t instr;
  uint32_t currAddr;
  succ_t succ;
  basicblk_t *currblk, *nextblk;

  // Set the file pointer to the start of first instruction
  obj_file_seek(fdesc, startaddr, SEEK_SET);

  currAddr = startaddr;
  currblk = find_block(blist, startaddr);
  nextblk = (basicblk_t*)(currblk->link.next);
  succ.branchflag = 0;
  succ.fallflag = 0;

  // Start reading the instructions - Second pass through the file
  while (obj_file_read(fdesc, &instr, sizeof(avr_instr_t), 1) != 0){
#ifdef BBIG_ENDIAN
    instr.rawVal = Flip_int16(instr.rawVal);
#endif
    DEBUG("Addr: 0x%x\n", currAddr);

    
    find_succ(fdesc, currAddr, &instr, &succ);
    currAddr += 2;
    
    if ((succ.branchflag == 1) || (succ.fallflag == 1)){
      // We have a successor
      currblk->calljmpflag = succ.calljmpflag;
      if (succ.branchflag == 1){
	if ((currblk->branch = find_block(blist, succ.branchaddr)) == NULL){
	  fprintf(stderr, "Blist is not formed correctly.\n");
	  exit(EXIT_FAILURE);
	}
      }
      else
	currblk->branch = NULL;
      if (succ.fallflag == 1){
	if ((currblk->fall = find_block(blist, succ.falladdr)) == NULL){
	  fprintf(stderr, "Blist is not formed correctly.\n");
	  exit(EXIT_FAILURE);
	}
      }
      else
	currblk->fall = NULL;  
      fill_basic_block(fdesc, currblk, nextblk, currAddr);
      currblk = nextblk;
      nextblk = (basicblk_t*)(currblk->link.next);
      continue;
    }
    
    if (nextblk->addr == currAddr){
      // We crossed a block boundary
      currblk->calljmpflag = 0;
      currblk->fall = nextblk;
      currblk->branch = NULL;
      fill_basic_block(fdesc, currblk, nextblk, currAddr);
      currblk = nextblk;
      nextblk = (basicblk_t*)(currblk->link.next);
      continue;
    }
  }
  return;
}
//--------------------------------------------------------------
static void fill_basic_block(file_desc_t* fdesc, basicblk_t* currblk, basicblk_t* nextblk, uint32_t currAddr)
{
  if (nextblk != NULL)
    currblk->size = nextblk->addr - currblk->addr;
  
  if (currblk->size > 0){
    avr_instr_t instr;
    int i, numinstr;

    // Set the file pointer and read
    obj_file_seek(fdesc, currblk->addr, SEEK_SET);


    // Assert currblk->instr is NULL
    if (currblk->instr != NULL){
      fprintf(stderr, "Memory corruption. Current Block Start Address: 0x%x\n", (int)currblk->addr);
      exit(EXIT_FAILURE);
    }
    // Allocate memory for currblk->instr
    if ((currblk->instr = malloc(currblk->size * sizeof(uint8_t))) == NULL){
      fprintf(stderr, "fill_basic_block: malloc out of memory\n");
      exit(EXIT_FAILURE);
    }

    numinstr = (int)(currblk->size/sizeof(avr_instr_t));
    for (i = 0; i < numinstr; i++){
      if (obj_file_read(fdesc, &instr, sizeof(avr_instr_t), 1) == 0){
	fprintf(stderr, "fill_basic_block: Error reading from file.\n");
	exit(EXIT_FAILURE);
      }
#ifdef BBIG_ENDIAN
      currblk->instr[i].rawVal = Flip_int16(instr.rawVal);
#endif      
    }
    // Allocate memory for addrmap
    if ((currblk->addrmap = malloc(numinstr * sizeof(uint32_t))) == NULL){
      fprintf(stderr,"fill_basic_block: Malloc out of memory for address map.\n");
      exit(EXIT_FAILURE);
    }
  }

  // Set the file pointer back to the current address
  obj_file_seek(fdesc, currAddr, SEEK_SET);
  
  return;
}
//--------------------------------------------------------------
// This function will determine the control flow successor addresses for a given instruction
// The file pointer will point to the next instruction in the stream
static void find_succ(file_desc_t* fdesc, uint32_t currAddr, avr_instr_t* instrin, succ_t* succ)
{
  static avr_instr_t previnstr;
  static int twowordinstr;
  avr_instr_t instr;

  instr.rawVal = instrin->rawVal;

  if ((match_optype10(&instr) == 0) || (match_optype19(&instr) == 0)){
    twowordinstr = 1;
    previnstr.rawVal = instr.rawVal;
    succ->branchflag = 0;
    succ->fallflag = 0;
    succ->calljmpflag = 0;
    return;
  }

  if (twowordinstr){
    twowordinstr = 0;
    // OPTYPE 10 - Call and Jump - Two word instr.
    switch (previnstr.rawVal & OP_TYPE10_MASK){
    case OP_JMP:
      {
	succ->branchaddr = (2 * get_optype10_k(&previnstr, &instr));
	succ->branchflag = 1;
	succ->fallflag = 0;
	succ->calljmpflag = 1;
	return;
      }
    case OP_CALL:
      {
	uint32_t calltargetaddr;
	if (sos_sys_call_check(fdesc, currAddr - sizeof(avr_instr_t), &calltargetaddr) != 0){
	  // Call internal to a module i.e. forms a basic block
	  if (BIN_FILE == fdesc->type)
	    succ->branchaddr = (2 * get_optype10_k(&previnstr, &instr));
	  else
	    succ->branchaddr = calltargetaddr;
	  succ->branchflag = 1;
	  succ->falladdr = currAddr + 2;
	  succ->fallflag = 1;
	  succ->calljmpflag = 1;
	  return;
	}
	DEBUG("Call into system jump table. Do not insert basic block boundary\n");
	break;
      }
    default:
      break;
    }   
    succ->branchflag = 0;
    succ->fallflag = 0;
    succ->calljmpflag = 0;
    return;
  }

  // OPTYPE1, OPTYPE6, OPTYPE11 - Skip Instruction Types
  if (((instr.rawVal & OP_TYPE1_MASK) == OP_CPSE)||
      ((instr.rawVal & OP_TYPE6_MASK) == OP_SBRC)||
      ((instr.rawVal & OP_TYPE6_MASK) == OP_SBRS)||
      ((instr.rawVal & OP_TYPE11_MASK) == OP_SBIC)||
      ((instr.rawVal & OP_TYPE11_MASK) == OP_SBIS))
    {
      uint8_t skip;
      skip = find_next_instr_size(fdesc);
      succ->branchaddr = currAddr + ((1 + skip) * 2);
      succ->branchflag = 1;
      succ->falladdr = currAddr + 2;
      succ->fallflag = 1;
      succ->calljmpflag = 0;
      return;
    }
    
  // OPTYPE 7 - Branch (BRBS, BRBC)
  switch (instr.rawVal & OP_TYPE7_MASK){
  case OP_BRBC:
  case OP_BRBS:
    {
      succ->branchaddr = currAddr + ((1 + get_optype7_k(&instr))*2);
      succ->branchflag = 1;
      succ->falladdr = currAddr + 2;
      succ->fallflag = 1;
      succ->calljmpflag = 0;
      return;
    }
  default:
    break;
  }
  
  // OPTYPE 8 - Remaining Branch Instructions
  switch (instr.rawVal & OP_TYPE8_MASK){
  case OP_BRCC: 
  case OP_BRCS: 
  case OP_BREQ: 
  case OP_BRGE: 
  case OP_BRHC: 
  case OP_BRHS: 
  case OP_BRID: 
  case OP_BRIE: 
    //  case OP_BRLO: 
  case OP_BRLT: 
  case OP_BRMI: 
  case OP_BRNE: 
  case OP_BRPL: 
    //  case OP_BRSH: 
  case OP_BRTC: 
  case OP_BRTS: 
  case OP_BRVC: 
  case OP_BRVS: 
    {
      succ->branchaddr = currAddr + ((1 + get_optype8_k(&instr))*2);
      succ->branchflag = 1;
      succ->falladdr = currAddr + 2;
      succ->fallflag = 1;
      succ->calljmpflag = 0;
      return;
    }
  default:
    break;
  }
  
  // OPTYPE 9 - Invalid Instr. and Return
  switch (instr.rawVal){
  case OP_EIJMP:
  case OP_IJMP:
    {
      fprintf(stderr, "find_succ: EIJMP/IJMP instructions are not permitted.\n");
      exit(EXIT_FAILURE);
      return;
    }
  case OP_ICALL:
  case OP_EICALL:
    {
      DEBUG("find_succ: ICALL/EICALL destination should be outside the module.\n");
      break;
    }
  case OP_RETI:
    {
      // Instruction not permitted. We still add the block corresponding to it.
      fprintf(stderr, "find_succ: RETI instruction not permitted.\n");
      exit(EXIT_FAILURE);
      break;
    }
  case OP_RET:
    {
      succ->branchflag = 0;
      succ->fallflag = 1;
      succ->falladdr = currAddr + 2;
      succ->calljmpflag = 0;
      return;
    }
  default:
    break;
  }
  
  // OPTYPE 17 - Relative Jump and Relative Call
  switch (instr.rawVal & OP_TYPE17_MASK){
  case OP_RJMP:
    {
      succ->branchaddr = currAddr + 2 + (2 * get_optype17_k(&instr));
      succ->branchflag = 1;
      succ->fallflag = 0;
      succ->calljmpflag = 0;
      return;
    }
  case OP_RCALL:
    {
      succ->branchaddr = currAddr + 2 + (2 * get_optype17_k(&instr));
      succ->branchflag = 1;
      succ->falladdr = currAddr + 2;
      succ->fallflag = 1;
      succ->calljmpflag = 0;
      return;
      }
  default:
    break;
  }
  
  succ->branchflag = 0;
  succ->fallflag = 0;
  succ->calljmpflag = 0;
  return;
}
//--------------------------------------------------------------
static int8_t insert_blkbnd(bblklist_t* blist, uint32_t addr)
{
  basicblk_t *newblk;
  basicblk_t *currblk;
#if defined(DBGMODE) && defined(DBG_FIND_BLK_BND)
  printf("Addr: 0x%x ", addr);
#endif
  // Initialize new block boundary
  if ((newblk = malloc(sizeof(basicblk_t))) == NULL){
    fprintf(stderr, "Out of memory\n");
    exit(EXIT_FAILURE);
  }
  newblk->addr = addr;
  newblk->size = 0;
  newblk->instr = NULL;
  newblk->fall = NULL;
  newblk->branch = NULL;

  // If the list is empty
  if ((blist->blk_1 == NULL) && (blist->blk_n == NULL)){
    blist->blk_1 = newblk;
    blist->blk_n = newblk;
    newblk->link.prev = NULL;
    newblk->link.next = NULL;
    return 0;
  }
  
  
  currblk = blist->blk_n;
  while (currblk->addr > newblk->addr){
    currblk = (basicblk_t*)currblk->link.prev;
    if (NULL == currblk) break;
  }
  
  // Insert at beginning
  if (NULL == currblk){
    /*
    fprintf(stderr, "Error in the program. Reaching an address before the start of program.\n");
    exit(EXIT_FAILURE);
    */
    fprintf(stderr,"Inserting at address 0x%x. Before start address.\n", (int)newblk->addr);
    newblk->link.next = (list_t*)blist->blk_1;
    newblk->link.prev = NULL;
    blist->blk_1->link.prev = (list_t*)newblk;
    blist->blk_1 = newblk;
    blist->cnt++;
    return 0;
    
  }

  // Do not insert duplicate
  if (currblk->addr == newblk->addr){
    free(newblk);
    return 0;
  }

  // Insert at end
  if (currblk == blist->blk_n){
    blist->blk_n->link.next = (list_t*)newblk;
    newblk->link.prev = (list_t*)blist->blk_n;
    newblk->link.next = NULL;
    blist->blk_n = newblk;
    blist->cnt++;
    return 0;
  }

  // Insert after currblk
  newblk->link.prev = (list_t*)currblk;
  newblk->link.next = currblk->link.next;
  currblk->link.next->prev = (list_t*)newblk;
  currblk->link.next = (list_t*)newblk;
  blist->cnt++;
  return 0;

}

//--------------------------------------------------------------
static uint8_t find_next_instr_size(file_desc_t* fdesc)
{
  avr_instr_t instr;
  
  if (obj_file_read(fdesc, &instr, sizeof(avr_instr_t), 1) == 0){
    fprintf(stderr, "Error in find_next_instr_size\n");
    exit(EXIT_FAILURE);
  }
  obj_file_seek(fdesc, -sizeof(avr_instr_t), SEEK_CUR);
#ifdef BBIG_ENDIAN
    instr.rawVal = Flip_int16(instr.rawVal);
#endif
  // Check if it is a two word instruction
    if ((match_optype10(&instr) == 0) || (match_optype19(&instr) == 0)){
      return 2;
    }
  
  return 1;
}
//--------------------------------------------------------------
basicblk_t* find_block(bblklist_t* blist, uint32_t addr)
{
  basicblk_t* cblk;
  for (cblk = blist->blk_1; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    if (cblk->addr == addr)
      return cblk;
  }
  return NULL;
}
//--------------------------------------------------------------
uint32_t find_updated_address(bblklist_t* blist, uint32_t oldaddr)
{
  basicblk_t* cblk;
  for (cblk = blist->blk_1; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    if ((cblk->addr <= oldaddr) && (oldaddr < (cblk->addr + cblk->size))){
      // Found the basic block
      uint32_t blkoffset, instrndx;
      blkoffset = oldaddr - cblk->addr;
      instrndx = blkoffset/sizeof(avr_instr_t);
      /*
      printf("Size: %d  Addr: 0x%x ", cblk->size, (int)cblk->addr);
      printf("NumInstr:%d  InstrNdx:%d  AddrMap:%d  NewAddr:0x%x\n",(int)(cblk->size/sizeof(avr_instr_t)),
	     instrndx, (int)cblk->addrmap[instrndx], (int)cblk->newaddr);
      */
      return (cblk->addrmap[instrndx] + cblk->newaddr);
    }
    if (cblk->addr == oldaddr) return cblk->newaddr;
  }
  fprintf(stderr, "find_updated_address: Cannot find address 0x%x in program.\n", oldaddr);
  exit(EXIT_FAILURE);
  return 0;
}
//--------------------------------------------------------------
void disp_blocks(bblklist_t* blist, uint32_t startaddr)
{
  basicblk_t* cblk;

  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    int i;
    
    printf("----");    
    printf("Start: 0x%x Size: %d NumInstr: %d ", (int)cblk->addr, (int)cblk->size, (int)(cblk->size/sizeof(avr_instr_t)));
    if (cblk->branch == NULL) 
      printf("Branch: NULL ");
    else
      printf("Branch: 0x%x ", (int)(cblk->branch)->addr);
    if (cblk->fall == NULL) 
      printf("Fall: NULL ");
    else
      printf("Fall: 0x%x ", (int)(cblk->fall)->addr);
    printf("-----\n");

    for (i = 0; i < (int)(cblk->size/sizeof(avr_instr_t)); i++){
      printf("0x%5x: %4x ", (int)(cblk->addr + (sizeof(avr_instr_t) * i)), cblk->instr[i].rawVal);
      decode_avr_instr_word(&(cblk->instr[i]));
      printf("\n");
    }
  }
  return;
}
//--------------------------------------------------------------
void disp_blkbndlist(bblklist_t* blist)
{
  basicblk_t* cblk;
  printf("BList: ");
  for (cblk = blist->blk_1; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    printf("0x%x ", (int)cblk->addr);
  }
  printf("\n Blist Size: %d\n", blist->cnt);
  return;
}
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
//--------------------------------------------------------------
/*
static int ltcmp_blkbnd(list_t* lhop, list_t* rhop)
{
  basicblk_t *lh, *rh;
  lh = (basicblk_t*)lhop;
  rh = (basicblk_t*)rhop;
  if (lh->addr < rh->addr)
    return 1;
  return 0;
}
//--------------------------------------------------------------
static void swap_blkbnd(list_t* a, list_t* b)
{
  basicblk_t *aop, *bop;
  basicblk_t temp;
  aop = (basicblk_t*)a;
  bop = (basicblk_t*)b;
  temp.addr = aop->addr;
  aop->addr = bop->addr;
  bop->addr = temp.addr;
  return;
}
//--------------------------------------------------------------
static void sort_block_boundaries(bblklist_t* blist)
{
  basicblk_t *currBnd, *dupBnd;

  qsortlist((list_t*)(blist->blk_1), (list_t*)(blist->blk_n),
	    ltcmp_blkbnd, swap_blkbnd);


  disp_blkbndlist(blist);

  // Remove duplicate elements from list
  currBnd = blist->blk_1;
  while (currBnd != NULL){
    dupBnd = (basicblk_t*)(currBnd->link.next);
    if (NULL == dupBnd){
      blist->blk_n = currBnd;
      break;
    }
    while (dupBnd->addr == currBnd->addr){
      list_t* delBnd;
      delBnd = (list_t*)dupBnd;
      dupBnd = (basicblk_t*)(dupBnd->link.next);
      // Unlink and free the duplicate
      (delBnd->prev)->next = delBnd->next;
      (delBnd->next)->prev = delBnd->prev;
      free(delBnd);
      if (NULL == dupBnd){
	blist->blk_n = currBnd;
	break;
      }
    }
    currBnd = dupBnd;
  }

  disp_blkbndlist(blist);

  return;
}
*/
