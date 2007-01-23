/**
 * \file avrsandbox.c
 * \brief Sandbox for AVR binary modules
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

/*
 * TODO:
 * Sandbox the STS type instructions
 */

//----------------------------------------------------------------------------
// INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <libelf.h>

#include <avrinstr.h>
#include <avrinstmatch.h>
#include <avrbinparser.h>
#include <basicblock.h>
#include <fileutils.h>
#include <avrsandbox.h>


#include <sos_mod_patch_code.h>
#include <sos_mod_header_patch.h>

//----------------------------------------------------------------------------
// TYPEDEFS

enum 
  {
    ST_SBX_TYPE = 0,
    RET_SBX_TYPE,
    ICALL_SBX_TYPE,
  };

/**
 * Sandbox Type Descriptor
 */
typedef struct _str_sandbox_type {
  int sbxtype;           //! Type of sandbox code to be generated
  int optype;            //! Opcode type of the instr to be sandbox
  int numnewinstr;       //! No. of sandbox instr. added
  avr_instr_t instr;     //! Instruction to sandbox
  avr_instr_t nextinstr; //! Next instruction to sandbox
} sandbox_desc_t;

//----------------------------------------------------------------------------
// STATIC FUNCTIONS
static void printusage();
static int avrsandbox(file_desc_t *fdesc, char* outFileName, uint32_t startaddr, uint16_t calladdr);


static int avr_get_sandbox_desc(avr_instr_t* instr, sandbox_desc_t* sndbx);
static void avr_gen_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx, uint16_t calladdr);
static void avr_gen_st_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx);
static void avr_gen_ret_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx);
static void avr_gen_icall_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx);


static void avr_dataflow_basic_block(basicblk_t* cblk);





//----------------------------------------------------------------------------
int main(int argc, char** argv)
{
  int ch;
  uint32_t startaddr;
  uint16_t calladdr;
  char *inputFileName, *outFileName;
  int ipstartaddr;
  file_desc_t fdesc;
  
  ipstartaddr = -1;
  inputFileName = NULL;
  startaddr = 0;
  calladdr = 0;
  outFileName = NULL;


  while ((ch = getopt(argc, argv, "s:f:o:c:w:h")) != -1){
    switch (ch){
    case 's': ipstartaddr = (int)strtol(optarg, NULL, 0); break;
    case 'c': calladdr = (uint16_t)((uint32_t)(strtol(optarg, NULL, 0)) >> 1); break;
    case 'f': inputFileName = optarg; break;
    case 'o': outFileName = optarg; break;
    case 'h': printusage(); return 0;
    case '?': printusage(); return 0;
    }
  }
  
  // Check input file name
  if (NULL == inputFileName){
    printusage();
    return 0;
  }


  // Open input file
  if (obj_file_open(inputFileName, &fdesc) != 0){
    fprintf(stderr, "Error opening input file.\n");
    exit(EXIT_FAILURE);
  }

  if (-1 == ipstartaddr){
    startaddr = find_sos_module_handler_addr(&fdesc);
  }
  else{
    startaddr = (uint32_t)ipstartaddr;
  }
  


  
  printf("===== STARTING SANDBOX ======\n");
  printf("Input File Name: %s\n", inputFileName);
  printf("Output File Name: %s\n", outFileName);
  printf("Start Address: 0x%x\n", (int)startaddr);

  avrsandbox(&fdesc, outFileName, startaddr, calladdr);


  // Close input file
  obj_file_close(&fdesc);
  
  return 0;
}
//----------------------------------------------------------------------------
// Top level function
static int avrsandbox(file_desc_t *fdesc, char* outFileName, uint32_t startaddr, uint16_t calladdr)
{
  // Stats
  int sandboxcnt ;         // Count of number of instructions sandboxed for stats
  int stsandboxcnt;        // Number of sandbox of type Store
  int bStInstr;            // Flag to indicate that store instructions are present in a basic block

  bblklist_t* blist;       // List of basic blocks in the program
  basicblk_t* cblk;        // Basic block interator
  basicblk_t sandboxblk;   // Basic block used to store the sandbox instructions
  sandbox_desc_t sndbx;    // Sandbox descriptor


  // Create a control flow graph of basic blocks
  blist = create_cfg(fdesc, startaddr);



  // Code introduced by sandboxing is stored in a basic block
  // Initialize that basic block
  sandboxcnt = 0;
  stsandboxcnt = 0;
  if ((sandboxblk.instr = malloc(MAX_SANDBOX_BLK_SIZE)) == NULL){
    fprintf(stderr, "avrsandbox: malloc -> Out of memory.\n");
    exit(EXIT_FAILURE);
  };
  sandboxblk.size = MAX_SANDBOX_BLK_SIZE;
  



  // Traverse all the basic blocks in the program
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    int i, j;
    bStInstr = 0;
    cblk->newsize = 0;

#ifdef DBGMODE    
    DEBUG("----");    
    DEBUG("Start: 0x%x Size: %d NumInstr: %d ", (int)cblk->addr, (int)cblk->size, (int)(cblk->size/sizeof(avr_instr_t)));
    if (cblk->branch == NULL) 
      DEBUG("Branch: NULL ");
    else
      DEBUG("Branch: 0x%x ", (int)(cblk->branch)->addr);
    if (cblk->fall == NULL) 
      DEBUG("Fall: NULL ");
    else
      DEBUG("Fall: 0x%x ", (int)(cblk->fall)->addr);
    DEBUG("-----\n");
#endif

    // First pass through the basic block
    // 1. Find the number of instructions to be sandboxed
    // 2. Find the new size of the basic block with the sandboxed instructions
    for (i = 0; i < (int)(cblk->size/sizeof(avr_instr_t)); i++){

      // Just store offsets within the block
      cblk->addrmap[i] = cblk->newsize;

#ifdef DBGMODE
      DEBUG("0x%5x: %4x ", (int)(cblk->addr + (sizeof(avr_instr_t) * i)), cblk->instr[i].rawVal);
      decode_avr_instr_word(&(cblk->instr[i]));
#endif
      if (avr_get_sandbox_desc((avr_instr_t*)&(cblk->instr[i]), &sndbx) ==  0){
	cblk->newsize += sndbx.numnewinstr * sizeof(avr_instr_t);
	sandboxcnt++;
	if (ST_SBX_TYPE == sndbx.sbxtype){
	  stsandboxcnt++;
	  bStInstr = 1;
	}
#ifdef DBGMODE
	printf("\n Sandbox Size: %d\n", sndbx.numnewinstr);
	// Generate the sandbox instructions for pretty print
	avr_gen_sandbox(&sandboxblk, &sndbx, calladdr);
	for (j = 0; j < (int)(sandboxblk.size/sizeof(avr_instr_t)); j++){
	  if (decode_avr_instr_word(&(sandboxblk.instr[j])) == 0)
	    DEBUG("\n");
	}
#endif
      }
      else{
	cblk->newsize += sizeof(avr_instr_t);
      }
#ifdef DBGMODE
      DEBUG("\n");
#endif
    }
   
#ifdef DBGMODE
    DEBUG("*** New Size: %d ****\n", (int)cblk->newsize);
#endif

    // If we encountered a store instruction, then perform dataflow analysis on the basic block
    if (bStInstr)
      avr_dataflow_basic_block(cblk);

    
    // We have a slack to introduce some extra control flow instructions
    if ((cblk->newinstr = malloc(cblk->newsize + BLOCK_SLACK_BYTES)) == NULL){
      fprintf(stderr,"avr_sandbox: malloc -> out of memory\n");
      exit(EXIT_FAILURE);
    }
    
    // Second pass through the basic block
    // 1. Insert the sandbox instructions into the basic block
    // 2. Copy over to the new basic block instructions
    j = 0;
    for (i = 0; i < (int)(cblk->size/sizeof(avr_instr_t)); i++){
      if (avr_get_sandbox_desc((avr_instr_t*)&(cblk->instr[i]), &sndbx) ==  0){
	int cnt;
	// Generate the sandbox instructions
	avr_gen_sandbox(&sandboxblk, &sndbx, calladdr);
	for (cnt = 0; cnt < (int)(sandboxblk.size/sizeof(avr_instr_t)); cnt++){
	  cblk->newinstr[j] = sandboxblk.instr[cnt];
	  j++;
	}
      }
      else{
	cblk->newinstr[j] = cblk->instr[i];
	j++;
      }
    }
    // Assertion
    if (cblk->newsize != (j * sizeof(avr_instr_t))){
      fprintf(stderr, "avr_sandbox: Assertion on new block size failed. Block Addr: 0x%x.",(int)cblk->addr);
    }    
  }

  // Second pass through basic blocks to patch rcall and internal calls
  DEBUG("============ Second Pass for RCalls/Calls ================\n");
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    int ndxlastinstr, ndx;
    basicblk_t* intcalltargetblk;
    uint32_t callinstr;

    // If basic block ends in call or jump then it cannot end in RCALL
    if (cblk->size == 0) continue;

    if (cblk->calljmpflag){
      // Basic Block ending in call implies that it is internal
      ndxlastinstr = (cblk->size/sizeof(avr_instr_t)) - 2;
      if ((cblk->instr[ndxlastinstr].rawVal & OP_TYPE10_MASK) != OP_CALL) continue;
    }
    else{
      // Compute index of last instruction
      ndxlastinstr = (cblk->size/sizeof(avr_instr_t)) - 1;
      // Compare last instruction with RCALL
      if ((cblk->instr[ndxlastinstr].rawVal & OP_TYPE17_MASK) != OP_RCALL) continue;
    }

    // Fun starts now
    intcalltargetblk = cblk->branch;
    DEBUG("Found internal call site: 0x%x Target: 0x%x\n", (int)(cblk->addr + (cblk->size-2)),(int)intcalltargetblk->addr);
    // Check if it has already been patched
    if (((intcalltargetblk->newinstr[0].rawVal & OP_TYPE10_MASK) == OP_CALL) &&
	((intcalltargetblk->newinstr[1].rawVal == KER_INTCALL_CODE)))
      continue;

    ndxlastinstr = (intcalltargetblk->newsize/sizeof(avr_instr_t)) - 1;
    for (ndx = ndxlastinstr; ndx >= 0; ndx--)
      intcalltargetblk->newinstr[ndx+2].rawVal = intcalltargetblk->newinstr[ndx].rawVal;
    // Update Address Map
    for (ndx = 0; ndx < (intcalltargetblk->size/sizeof(avr_instr_t)); ndx++)
      intcalltargetblk->addrmap[ndx] += 2 * sizeof(avr_instr_t); 
    callinstr = create_optype10(OP_CALL, KER_INTCALL_CODE);
    intcalltargetblk->newinstr[0].rawVal = (uint16_t)(callinstr >> 16);
    intcalltargetblk->newinstr[1].rawVal = (uint16_t)callinstr;
    intcalltargetblk->newsize += 2 * sizeof(avr_instr_t);
    sandboxcnt++;
    DEBUG("Patched internal call site: 0x%x Target: 0x%x\n", (int)(cblk->addr + (cblk->size-2)),(int)intcalltargetblk->addr);
  }
  
  // Third pass to mark basic blocks that target rcall target but are NOT rcalls  
  DEBUG("============ Third Pass for RCall/Call Targets ================\n");
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    int ndxlastinstr;
    basicblk_t *branchblk;// *fallblk;

    if (cblk->size == 0) continue;
    // Compute index of last instruction
    if (cblk->calljmpflag)
      ndxlastinstr = (cblk->size/sizeof(avr_instr_t)) - 2;
    else
      ndxlastinstr = (cblk->size/sizeof(avr_instr_t)) - 1;
    // Skip RCALL Blocks
    if ((cblk->instr[ndxlastinstr].rawVal & OP_TYPE17_MASK) == OP_RCALL) continue;
    // Skip CALL Blocks
    if ((cblk->instr[ndxlastinstr].rawVal & OP_TYPE10_MASK) == OP_CALL) continue;

    // Fun starts now
    branchblk = cblk->branch;
    if ((NULL != branchblk) && (branchblk->size > 0)){
      if (((branchblk->newinstr[0].rawVal & OP_TYPE10_MASK) == OP_CALL) &&
	  ((branchblk->newinstr[1].rawVal == KER_INTCALL_CODE))){
	DEBUG("Found a funky one ! Site: 0x%x\n", (int)(cblk->addr + sizeof(avr_instr_t)*ndxlastinstr));
      }
    }
    /*
    fallblk = cblk->fall;
    if (NULL != fallblk){
      if (((fallblk->newinstr[0].rawVal & OP_TYPE10_MASK) == OP_CALL) &&
	  ((fallblk->newinstr[1].rawVal == KER_INTCALL_CODE))){
	DEBUG("Found a funky one ! Site: 0x%x\n", (uint32_t)(cblk->addr + sizeof(avr_instr_t)*ndxlastinstr));
      }
    }
    */
  }


  avr_update_cf(blist, startaddr);

  if (BIN_FILE == fdesc->type)
    avr_write_binfile(blist, outFileName, fdesc, startaddr);
  else
    avr_write_elffile(blist, outFileName, fdesc, startaddr);

  printf("==== SANDBOX DONE ======\n");
  printf("Number of instructions sandboxed: %d\n", sandboxcnt);
  printf("   Number of STORE instructions: %d\n", stsandboxcnt);
  printf("Output written to file: %s\n", outFileName);
#ifdef SBX_FIX_REGS
  printf("Sandbox using fixed registers.\n");
#else
  printf("Sandbox using PUSH/POP of registers.\n");
#endif
  printf("Registers Used: ");
  printreg(AVR_FIXED_REG_1); printf("and ");
  printreg(AVR_FIXED_REG_2);
  printf("\n");
  printf("========================\n");
  return 0;
}
//-------------------------------------------------------------------
static int avr_get_sandbox_desc(avr_instr_t* instr, sandbox_desc_t* sndbx)
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
static void avr_gen_sandbox(basicblk_t* sandboxblk, sandbox_desc_t* sndbx, uint16_t calladdr)
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
//-------------------------------------------------------------------

typedef struct _str_ptr_reg {
  int binit;
  int lvalue;
  int uvalue;
  int cvalue;
} ptr_reg_t;

static void avr_dataflow_basic_block(basicblk_t* cblk)
{
  ptr_reg_t x, y, z;
  x.binit = 0; x.lvalue = 0; x.uvalue = 0; x.cvalue = 0;
  y.binit = 0; y.lvalue = 0; y.uvalue = 0; y.cvalue = 0;
  z.binit = 0; z.lvalue = 0; z.uvalue = 0; z.cvalue = 0;

  
  return;
}
//-------------------------------------------------------------------
 static void printusage()
 {
   printf("Usage avrsandbox [options]\n");
   printf("Options:\n");
   printf("-s <startaddr>: Entry byte address into SOS module.\n");
   printf("-c <calladdr>: Byte address of the memmap checker.\n");
   printf("-f <Filename>: Input SOS binary file.\n");
   printf("-o <OutputFilename>: Name of the output file.\n");
   return;  
 }
//-------------------------------------------------------------------
