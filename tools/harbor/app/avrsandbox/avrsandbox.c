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
#include <soself.h>

#include <avrinstr.h>
#include <avrinstmatch.h>
#include <avrbinparser.h>
#include <basicblock.h>
#include <fileutils.h>
#include <avrsandbox.h>


#include <sos_mod_patch_code.h>
#include <sos_mod_header_patch.h>

//----------------------------------------------------------------------------
// STATIC FUNCTIONS
static void printusage();
static int avrsandbox(file_desc_t *fdesc, char* outFileName, uint32_t startaddr, uint16_t calladdr);

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
  //  int bStInstr;            // Flag to indicate that store instructions are present in a basic block

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
    //    bStInstr = 0;
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
	  //  bStInstr = 1;
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
    /*
    if (bStInstr)
      avr_dataflow_basic_block(cblk);
    */
    
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

    if ((cblk->flag & TWO_WORD_INSTR_FLAG)&&(cblk->flag & CALL_INSTR_FLAG)){
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
    if (cblk->flag & TWO_WORD_INSTR_FLAG){
      ndxlastinstr = (cblk->size/sizeof(avr_instr_t)) - 2;
      // Skip CALL Blocks
      if ((cblk->instr[ndxlastinstr].rawVal & OP_TYPE10_MASK) == OP_CALL) continue;
    }
    else{
      ndxlastinstr = (cblk->size/sizeof(avr_instr_t)) - 1;
      // Skip RCALL Blocks
      if ((cblk->instr[ndxlastinstr].rawVal & OP_TYPE17_MASK) == OP_RCALL) continue;
    }


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

  // Update Control Flow
  avr_update_cf(blist, startaddr);

  // Write sandbox binary to file
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
