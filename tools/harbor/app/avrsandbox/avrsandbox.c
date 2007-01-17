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
#include <elfhelper.h>
#include <elfdisplay.h>
#include <avr_minielf.h>

#include <sos_mod_header_patch.h>
#include <sos_mod_patch_code.h>

//----------------------------------------------------------------------------
// DEFINES
//#define SBX_FIX_REGS        //! For binary re-writes assuming fixed registers
#ifdef SBX_FIX_REGS
#define AVR_SCRATCH_REG       R4
#define AVR_FIXED_REG_1       R2
#define AVR_FIXED_REG_2       R3
#else
#define AVR_SCRATCH_REG       R0
#define AVR_FIXED_REG_1       R26   //! Used by the memmap checker routine to store  
#define AVR_FIXED_REG_2       R27   //! the write address
#endif

#define AVR_SREG        0x3F  //! AVR IO address of processor status register
#define BLOCK_SLACK_BYTES 40  //! Slack due to control flow changes 
#define	Flip_int16(type)  (((type >> 8) & 0x00ff) | ((type << 8) & 0xff00))
#define MAX_SANDBOX_BLK_SIZE 30


//----------------------------------------------------------------------------
// DEBUG
#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

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
static void avr_update_cf(bblklist_t* blist, uint32_t startaddr);
static void avr_update_cf_instr(avr_instr_t* instr, basicblk_t* cblk, int* cfupdateflag);
static void avr_fix_branch_instr(avr_instr_t* instrin, basicblk_t* cblk, uint16_t opcode, uint8_t bitpos);
static void avr_fix_cpse(avr_instr_t* instrin, basicblk_t* cblk);
static void avr_fix_skip_instr(avr_instr_t* instrin, basicblk_t* cblk);
static void avr_write_binfile(bblklist_t* blist, char* outFileName, file_desc_t *fdesc, uint32_t startaddr);
static void avr_write_elffile(bblklist_t* blist, char* outFileName, file_desc_t* fdesc, uint32_t startaddr);
static void avr_create_new_elf_header(Elf32_Ehdr *ehdr, Elf32_Ehdr *nehdr);
static void avr_create_new_section_header(Elf32_Shdr *shdr, Elf32_Shdr *nshdr);
static void avr_create_new_data(Elf_Data *edata, Elf_Data *nedata);
static void avr_create_new_text_data(Elf_Data* edata, Elf_Data* nedata, 
				     file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr);
static void avr_create_new_rela_text_data(Elf_Data* nedata, Elf* elf,
					  Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist);
static void avr_create_new_symbol_table(Elf_Data* nedata, Elf* elf, Elf* nelf,
					Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist);
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
  int sandboxcnt ;         // Count of number of instructions sandboxed for stats
  bblklist_t* blist;       // List of basic blocks in the program
  basicblk_t* cblk;        // Basic block interator
  basicblk_t sandboxblk;   // Basic block used to store the sandbox instructions
  sandbox_desc_t sndbx;    // Sandbox descriptor


  // Create a control flow graph of basic blocks
  blist = create_cfg(fdesc, startaddr);



  // Code introduced by sandboxing is stored in a basic block
  // Initialize that basic block
  sandboxcnt = 0;
  if ((sandboxblk.instr = malloc(MAX_SANDBOX_BLK_SIZE)) == NULL){
    fprintf(stderr, "avrsandbox: malloc -> Out of memory.\n");
    exit(EXIT_FAILURE);
  };
  sandboxblk.size = MAX_SANDBOX_BLK_SIZE;
  



  // Traverse all the basic blocks in the program
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    int i, j;

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
static void avr_update_cf(bblklist_t* blist, uint32_t startaddr)
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
      if (cblk->calljmpflag)
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
	DEBUG("SKIP INSTR.: Old Size: %d New Size: %d. Addr: 0x%x\n", (int)fallblk->size, (int)fallblk->newsize, (int)(cblk->newaddr+cblk->newsize));
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
	fprintf(stderr, "OPTYPE17: Addr: 0x%x Branch: 0x%x k: %d.\n", (int)(cblk->newaddr + cblk->newsize), (int)cblk->branch->newaddr, k);
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
//-------------------------------------------------------------------
static void avr_write_binfile(bblklist_t* blist, char* outFileName, file_desc_t* fdesc, uint32_t startaddr)
{
  FILE *outbinFile;
  basicblk_t* cblk;
  int i;
  avr_instr_t instr;
  uint8_t* buff;

  /*
  inbinFile = fopen(inFileName, "r");
  if (NULL == inbinFile){
    fprintf(stderr, "Error opening file %s\n", inFileName);
    exit(EXIT_FAILURE);
  }  
  */
  if (fdesc->type != BIN_FILE) return;
  
  outbinFile = fopen(outFileName, "w");
  if (NULL == outbinFile){
    fprintf(stderr, "Error opening file %s\n", outFileName);
    exit(EXIT_FAILURE);
  }  
  
  if ((buff = malloc(sizeof(uint8_t)*startaddr)) == NULL){
    fprintf(stderr, "Out of memory.\n");
    exit(EXIT_FAILURE);
  }

  obj_file_seek(fdesc, 0, SEEK_SET);
  if (obj_file_read(fdesc, buff, sizeof(uint8_t), startaddr) == 0){
    fprintf(stderr, "avr_write_binfile: Premature end of input file.\n");
    exit(EXIT_FAILURE);
  }

  // Patch the SOS module header
  sos_patch_mod_header(blist, buff);

  if (fwrite(buff, sizeof(uint8_t), startaddr, outbinFile) == 0){
      fprintf(stderr, "avr_write_binfile: Premature end of output file.\n");
      exit(EXIT_FAILURE);
  }

  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    for (i = 0; i < (int)(cblk->newsize/sizeof(avr_instr_t)); i++){
      instr.rawVal = cblk->newinstr[i].rawVal;
#ifdef BBIG_ENDIAN
      instr.rawVal = Flip_int16(instr.rawVal);
#endif
      if (fwrite(&instr, sizeof(avr_instr_t),1, outbinFile) != 1){
	fprintf(stderr, "avr_write_binfile: Error during file write.\n");
	exit(EXIT_FAILURE);
      }
    }    
  }  
  fclose(outbinFile);
  return;
}
//-------------------------------------------------------------------
static void avr_write_elffile(bblklist_t* blist, char* outFileName, file_desc_t* fdesc, uint32_t startaddr)
{
  int fd;
  Elf *elf, *nelf;
  Elf32_Ehdr *ehdr, *nehdr;
  Elf_Scn *scn, *nscn;
  Elf32_Shdr *shdr, *nshdr;
  Elf_Data *edata, *nedata;

  DEBUG("=========== ELF File Write ===========\n");

  elf = fdesc->elf;
  
  fd = open(outFileName, O_RDWR|O_TRUNC|O_CREAT, 0666);
  if ((nelf = elf_begin(fd, ELF_C_WRITE, (Elf *)NULL)) == 0){
    fprintf(stderr, "avr_write_elffile: Error creating output ELF archive.\n");
    exit(EXIT_FAILURE);
  }
  
  if ((nehdr = elf32_newehdr(nelf)) == NULL){
    fprintf(stderr, "avr_write_elffile: Error creating new ELF header.\n");
    exit(EXIT_FAILURE);
  }
  if ((ehdr = elf32_getehdr(elf)) == NULL){
    fprintf(stderr, "avr_write_elfffile: Error reading ELF header.\n");
    exit(EXIT_FAILURE);
  }
  avr_create_new_elf_header(ehdr, nehdr);
  
  
  scn = NULL;
  while ((scn = elf_nextscn(fdesc->elf, scn)) != NULL){
    nscn = elf_newscn(nelf);
    nshdr = elf32_getshdr(nscn);
    shdr = elf32_getshdr(scn);
    avr_create_new_section_header(shdr, nshdr);
    edata = NULL;
    edata = elf_getdata(scn, edata);
    nedata = elf_newdata(nscn);
    
    // Get name of current section
    char* CurrSecName = NULL;
    CurrSecName = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
    // Compare with .text section
    if (strcmp(CurrSecName, ".text") == 0)
      avr_create_new_text_data(edata, nedata, fdesc, blist, startaddr);
    // Comare with .rela.text section
    else if (strcmp(CurrSecName, ".rela.text") == 0){
      avr_create_new_data(edata, nedata);
      avr_create_new_rela_text_data(nedata, elf, nshdr, startaddr, blist);
    }
    else if (strcmp(CurrSecName, ".symtab") == 0){
      avr_create_new_data(edata, nedata);
      avr_create_new_symbol_table(nedata, elf, nelf, nshdr, startaddr, blist);
    }
    else
      avr_create_new_data(edata, nedata);
    elf_update(nelf, ELF_C_WRITE);
  }
  elf_end(nelf);
}
//-------------------------------------------------------------------
static void avr_create_new_symbol_table(Elf_Data* nedata, Elf* elf, Elf* nelf,
					Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist)
{
  Elf32_Ehdr* ehdr;
  Elf32_Sym* nsym;
  Elf_Data* nreladata;
  Elf32_Rela *nerela;
  int numSyms, numRecs, i, txtscnndx, btxtscnfound;
  Elf_Scn *txtscn, *nrelascn;
  Elf32_Shdr *txtshdr, *nrelashdr;

  DEBUG("Determine .text section index ...\n");
  // Get the ELF Header
  if ((ehdr = elf32_getehdr(elf)) == NULL){
    fprintf(stderr, "avr_create_new_symbol_table: Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }  
  txtscn = NULL;
  txtscnndx = 1;
  btxtscnfound = 0;
  while ((txtscn = elf_nextscn(elf, txtscn)) != NULL){
    if ((txtshdr = elf32_getshdr(txtscn)) != NULL){
      char* CurrSecName = NULL;
      CurrSecName = elf_strptr(elf, ehdr->e_shstrndx, txtshdr->sh_name);
      if (strcmp(CurrSecName, ".text") == 0){
	btxtscnfound = 1;
	break;
      }
    }
    txtscnndx++;
  }
  if (0 == btxtscnfound){
    fprintf(stderr, "avr_create_new_symbol_table: Cannot find .text section.\n");
    exit(EXIT_FAILURE);
  }
  DEBUG(".text section index: %d\n", txtscnndx);
  
  // Get .rela.text section
  nrelascn = getELFSectionByName(nelf, ".rela.text");
  nrelashdr = elf32_getshdr(nrelascn);
  nreladata = NULL;
  nreladata = elf_getdata(nrelascn, nreladata);
  numRecs = nreladata->d_size/nrelashdr->sh_entsize;

  numSyms = nedata->d_size/nshdr->sh_entsize;
  nsym = (Elf32_Sym*)(nedata->d_buf);
  for (i = 0; i < numSyms; i++){
    if ((nsym->st_shndx == txtscnndx) && (ELF32_ST_TYPE(nsym->st_info) != STT_SECTION)){
      if (nsym->st_value > startaddr){
	uint32_t oldValue = nsym->st_value;
	nsym->st_value = find_updated_address(blist, oldValue);
	// Check if we have to further modify this symbol (if it is used as a call target value)
	int j;
	nerela = (Elf32_Rela*)nreladata->d_buf;
	for (j = 0; j < numRecs; j++){
	  if ((ELF32_R_SYM(nerela->r_info) == i) && (ELF32_R_TYPE(nerela->r_info) == R_AVR_CALL)){
	    nsym->st_value -= sizeof(avr_instr_t) * 2;
	    DEBUG("Call target symbol. Modify to accomodate safe stack store.\n");
	    break;
	  }
	  // Follwing is only for producing a more readable elf.lst
	  if ((ELF32_ST_BIND(nsym->st_info) == STB_LOCAL) &&
	      (ELF32_ST_TYPE(nsym->st_info) == STT_FUNC) &&
	      (ELF32_R_TYPE(nerela->r_info) == R_AVR_CALL) &&
	      (nerela->r_addend == (nsym->st_value - (2*sizeof(avr_instr_t))))){
	    nsym->st_value -= sizeof(avr_instr_t) * 2;
	    DEBUG("Call target symbol. Modified for ELF pretty print.\n");
	  }
	  nerela++;
	}
	DEBUG("Entry: %d Old Value: 0x%x New Value: 0x%x\n", i, (int)oldValue, (int)nsym->st_value);
      }
    }
    nsym++;
  }
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_rela_text_data(Elf_Data* nedata, Elf* elf,
					  Elf32_Shdr *nshdr, uint32_t startaddr, bblklist_t* blist)
{
  Elf_Scn* symscn;
  Elf32_Shdr* symshdr;
  Elf_Data *symdata;
  Elf32_Sym* sym;
  Elf32_Rela *nerela;
  int numRecs, numSyms, i, textNdx;
  
  symscn = getELFSymbolTableScn(elf);
  symshdr = elf32_getshdr(symscn);
  symdata = NULL;
  symdata = elf_getdata(symscn, symdata);
  sym = (Elf32_Sym*)symdata->d_buf;
  numSyms = symdata->d_size/symshdr->sh_entsize;
  textNdx = -1;
  DEBUG("Locating symbol table index for .text\n");
  DEBUG("Rela Info: %d\n", (int)nshdr->sh_info);
  for (i = 0; i < numSyms; i++){
    #ifdef DBGMODE
    ElfPrintSymbol(sym);
    DEBUG("\n");
    #endif
    if ((sym->st_shndx == nshdr->sh_info) && (ELF32_ST_TYPE(sym->st_info) == STT_SECTION)){
      textNdx = i;
      break;
    }
    sym++;
  }
  if (-1 == textNdx){
    fprintf(stderr, "avr_create_new_rela_text_data: Could not locate .text section symbol\n");
    exit(EXIT_FAILURE);
  }
  DEBUG(".text symbol table index: %d\n", textNdx);

  DEBUG("Fixing .rela.text entries ...\n");
  nerela = (Elf32_Rela*)nedata->d_buf;
  numRecs = nedata->d_size/nshdr->sh_entsize;
  
  for (i = 0; i < numRecs; i++){
    // Change offset field for all records .rela.text
    // Change addend if symbol field is .text
    if (nerela->r_offset > startaddr){
      uint32_t oldaddr = nerela->r_offset;
      nerela->r_offset = find_updated_address(blist, oldaddr);
      DEBUG("Entry: %d Old Offset: 0x%x New Offset: 0x%x\n", i, (int)oldaddr, (int)nerela->r_offset);
    }
    if (ELF32_R_SYM(nerela->r_info) == textNdx){
      if (nerela->r_addend > startaddr){
	uint32_t old_addend = nerela->r_addend;
	nerela->r_addend = find_updated_address(blist, old_addend);
	// If relocation type is of R_AVR_CALL, then modify addend.
	// The addend should point to two words before the actual function 
	// so as to invoke the call to the safe stack save function
	if (ELF32_R_TYPE(nerela->r_info) == R_AVR_CALL){
	  nerela->r_addend -= sizeof(avr_instr_t) * 2;
	  DEBUG("Internal call target -> Modify addend to include safe stack.\n");
	}
	DEBUG("Entry: %d Old Addend: 0x%x New Addend: 0x%x\n", i, (int)old_addend, nerela->r_addend);
      }
    }
    nerela++;
  }
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_text_data(Elf_Data* edata, Elf_Data* nedata, 
				     file_desc_t* fdesc, bblklist_t* blist, uint32_t startaddr)
{
  basicblk_t* cblk;
  int i;
  avr_instr_t instr;
  avr_instr_t* wrptr;
    
  nedata->d_type = edata->d_type;
  nedata->d_align = edata->d_align;
  nedata->d_size = startaddr;

  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next)
    nedata->d_size += cblk->newsize;
  nedata->d_buf = (uint8_t*)malloc(nedata->d_size);

  obj_file_seek(fdesc, 0, SEEK_SET);
  if (obj_file_read(fdesc, nedata->d_buf, sizeof(uint8_t), startaddr) == 0){
    fprintf(stderr, "avr_write_binfile: Premature end of input file.\n");
    exit(EXIT_FAILURE);
  }
  wrptr = (avr_instr_t*)((uint8_t*)((uint8_t*)nedata->d_buf + startaddr));
 
  for (cblk = blist->blk_st; cblk != NULL; cblk = (basicblk_t*)cblk->link.next){
    for (i = 0; i < (int)(cblk->newsize/sizeof(avr_instr_t)); i++){
      instr.rawVal = cblk->newinstr[i].rawVal;
#ifdef BBIG_ENDIAN
      instr.rawVal = Flip_int16(instr.rawVal);
#endif
      if (((uint8_t*)wrptr - (uint8_t*)(nedata->d_buf) + sizeof(avr_instr_t)) <= nedata->d_size){
	wrptr->rawVal = instr.rawVal;
	wrptr++;
      }
      else
	fprintf(stderr, "avr_create_new_text_data: Buffer overflow while writing sandbox .text section.\n");
    }    
  }  
  return;
}

//-------------------------------------------------------------------
static void avr_create_new_elf_header(Elf32_Ehdr *ehdr, Elf32_Ehdr *nehdr)
{
  nehdr->e_ident[EI_CLASS] = ehdr->e_ident[EI_CLASS];
  nehdr->e_ident[EI_DATA] = ehdr->e_ident[EI_DATA];
  nehdr->e_type = ehdr->e_type;
  nehdr->e_machine = ehdr->e_machine;
  nehdr->e_flags = ehdr->e_flags;
  nehdr->e_shstrndx = ehdr->e_shstrndx;
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_section_header(Elf32_Shdr *shdr, Elf32_Shdr *nshdr)
{
  nshdr->sh_name = shdr->sh_name;
  nshdr->sh_type = shdr->sh_type;
  nshdr->sh_flags = shdr->sh_flags;
  nshdr->sh_addr = shdr->sh_addr;
  nshdr->sh_size = shdr->sh_size;
  nshdr->sh_link = shdr->sh_link;
  nshdr->sh_info = shdr->sh_info;
  nshdr->sh_addralign = shdr->sh_addralign;
  nshdr->sh_entsize = shdr->sh_entsize;
  return;
}
//-------------------------------------------------------------------
static void avr_create_new_data(Elf_Data *edata, Elf_Data *nedata)
{
  nedata->d_type = edata->d_type;
  nedata->d_size = edata->d_size;
  nedata->d_align = edata->d_align;
  if (edata->d_size > 0){
    nedata->d_buf = malloc(edata->d_size);
    memcpy(nedata->d_buf, edata->d_buf, edata->d_size);
  }
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
