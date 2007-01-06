/**
 * \file sos_module_header_patch.c
 * \brief Routine to patch the SOS module header
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>

#include <libelf.h>

#include <fileutils.h>
#include <elfdisplay.h>
#include <elfhelper.h>
#include <basicblock.h>
#include <sos_mod_header_patch.h>
#include <sos_mod_patch_code.h>

/*
  SOS Module Header is manipulated as a byte array.
  This is because of the difference in the size of the data types on PC and micro-controllers.
  The definition of the SOS module header is obtained from sos_module_types.h
  XXX - This is a quick fix till I figure out a cleaner way.
typedef struct mod_header {
  sos_pid_t mod_id;        //!< module ID (used for messaging).  Set NULL_PID for system selected mod_id
  uint8_t state_size;      //!< module state size
  uint8_t num_timers;      //!< Number of timers to be reserved at module load time
  uint8_t num_sub_func;	   //!< number of functions to be subscribed
  uint8_t num_prov_func;   //!< number of functions provided
  uint8_t version;         //!< version number, for users bookkeeping
  uint8_t processor_type;  //!< processor type of this module
  uint8_t platform_type;   //!< platform type of this module
  sos_code_id_t code_id;   //!< module image identifier
  uint8_t padding;
  uint8_t padding2; 
  msg_handler_t module_handler;
  func_cb_t funct[];
} mod_header_t;


typedef struct func_cb {
	void *ptr;        //! function pointer                    
	uint8_t proto[4]; //! function prototype                  
	uint8_t pid;      //! function PID                                    
	uint8_t fid;      //! function ID                         
} func_cb_t;

 */

//----------------------------------------------------------------------------
// DEBUG
//#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

#define NUM_SUB_FUNC_POS   3
#define NUM_PROV_FUNC_POS  4
#define FUNC_CB_LIST_POS  14
#define MOD_HANDLER_LSB   12
#define MOD_HANDLER_MSB   13
#define sos_mod_hdr_num_sub_func(buff) buff[NUM_SUB_FUNC_POS]
#define sos_mod_hdr_num_prov_func(buff) buff[NUM_PROV_FUNC_POS]
#define sos_mod_hdr_func_cb_list_addr(buff) &buff[FUNC_CB_LIST_POS]
#define sos_mod_hdr_mod_handler(buff) (uint16_t)((uint16_t)((uint16_t)(buff[MOD_HANDLER_MSB]) << 8) + (uint16_t)(buff[MOD_HANDLER_LSB]))

#define SIZE_OF_FUNC_CB_T   8
#define FUNC_CB_FPTR_L_POS  0
#define FUNC_CB_FPTR_H_POS  1
#define sos_func_cb_get_fptr(buff) (uint16_t)((uint16_t)((uint16_t)(buff[FUNC_CB_FPTR_H_POS]) << 8) + (uint16_t)(buff[FUNC_CB_FPTR_L_POS]))
#define sos_func_cb_set_fptr(buff, addr) {			\
    buff[FUNC_CB_FPTR_L_POS] = (uint8_t)addr;			\
    buff[FUNC_CB_FPTR_H_POS] = (uint8_t)(uint16_t)(addr >> 8);	\
  }

//----------------------------------------------------------------------------
// STATIC FUNCTIONS
static uint32_t bin_find_sos_module_handler_addr(file_desc_t* fdesc);
static uint32_t elf_find_sos_module_handler_addr(file_desc_t* fdesc);


void sos_patch_mod_header(bblklist_t* blist, uint8_t* mhdr)
{
  int i;
  uint8_t *pfuncb;
  uint32_t oldaddr, newaddr;
  basicblk_t* bblk;

  DEBUG("========= Patching SOS Module Header ========\n");
  DEBUG("Number of functions: %d\n", sos_mod_hdr_num_prov_func(mhdr) + sos_mod_hdr_num_sub_func(mhdr));

  //  printf("Raw Dump of Mod Header:\n");
  //  for (i = 0; i < 14; i++) printf("%d ", mhdr[i]);
  //  printf("\n");

  for (i = 0, pfuncb = sos_mod_hdr_func_cb_list_addr(mhdr); 
       i < (sos_mod_hdr_num_prov_func(mhdr) + sos_mod_hdr_num_sub_func(mhdr));
       i++, pfuncb += SIZE_OF_FUNC_CB_T)
    {
      uint16_t addrval;

      addrval = sos_func_cb_get_fptr(pfuncb);
      oldaddr = (uint32_t)(((uint32_t)addrval) << 1);
      DEBUG("Old Addr: 0x%x ", (int)oldaddr);
      if (0 == oldaddr) continue; // If the ptr is NULL, leave it that way.
      bblk = find_block(blist, oldaddr);
      if (NULL == bblk){
	fprintf(stderr, "Cannot find function within module.\n");
	exit(EXIT_FAILURE);
      }
      newaddr = bblk->newaddr;
      
      // If the exported function is also called internally
      if (((bblk->newinstr[0].rawVal & OP_TYPE10_MASK) == OP_CALL) &&
	  ((bblk->newinstr[1].rawVal == KER_INTCALL_CODE)))
	newaddr += 4;
      

      addrval = (uint16_t)(uint32_t)(newaddr >> 1);
      sos_func_cb_set_fptr(pfuncb, addrval);
      DEBUG("New Addr: 0x%x\n", (int)newaddr);
    }
  return;
}

uint32_t find_sos_module_handler_addr(file_desc_t* fdesc){
  if (BIN_FILE == fdesc->type)
    return bin_find_sos_module_handler_addr(fdesc);
  else 
    return elf_find_sos_module_handler_addr(fdesc);
}

static uint32_t bin_find_sos_module_handler_addr(file_desc_t* fdesc)
{
  uint32_t handler_addr;
  uint8_t mhdr[FUNC_CB_LIST_POS];

  if (fread(mhdr, sizeof(uint8_t), FUNC_CB_LIST_POS, fdesc->fd) == 0){
    fprintf(stderr, "bin_find_sos_module_handler_addr: Premature end of file %s\n", fdesc->name);
    exit(EXIT_FAILURE);
  }

  handler_addr = (uint32_t)(sos_mod_hdr_mod_handler(mhdr));
  handler_addr <<= 1;
  return handler_addr;
}


static uint32_t elf_find_sos_module_handler_addr(file_desc_t* fdesc)
{
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Rela *erela;
  int numRecs;
  uint32_t handler_addr;

  scn = getELFSectionByName(fdesc->elf, ".rela.text");
  if (NULL == scn){
    fprintf(stderr, "elf_find_sos_module_handler_addr: Cannot determine address of message handler.\n");
    exit(EXIT_FAILURE);
  }

  if ((shdr = elf32_getshdr(scn)) != NULL){
    if (SHT_RELA == shdr->sh_type){  
      edata = NULL;
      while ((edata = elf_getdata(scn, edata)) != NULL){
	if (ELF_T_RELA == edata->d_type){
	  erela = (Elf32_Rela*)edata->d_buf;
	  numRecs = edata->d_size/shdr->sh_entsize;
	  int i;
	  for (i = 0; i < numRecs; i++){
	    if (MOD_HANDLER_LSB == erela[i].r_offset){ 
	      handler_addr = erela[i].r_addend;
	      DEBUG("Handler Address: 0x%x\n", (int)handler_addr);
	      return handler_addr;
	    }
	  }
	}
      }
    }
  }
  fprintf(stderr, "elf_find_sos_module_handler_addr: Cannot find messange handler.\n");
  exit(EXIT_FAILURE);
  return 0;
}

// Returns 0 if the call instruction target is sys table else returns -1
int8_t sos_sys_call_check(file_desc_t* fdesc, uint32_t callInstrAddr, uint32_t* calltargetaddr)
{
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Rela *erela;
  int numRecs;

  if (BIN_FILE == fdesc->type) return -1;
  
  scn = getELFSectionByName(fdesc->elf, ".rela.text");
  if (NULL == scn){
    fprintf(stderr, "sos_sys_call_check: Cannot find relocation section in ELF file.\n");
    exit(EXIT_FAILURE);
  }

  if ((shdr = elf32_getshdr(scn)) != NULL){
    if (SHT_RELA == shdr->sh_type){  
      edata = NULL;
      while ((edata = elf_getdata(scn, edata)) != NULL){
	if (ELF_T_RELA == edata->d_type){
	  erela = (Elf32_Rela*)edata->d_buf;
	  numRecs = edata->d_size/shdr->sh_entsize;
	  int i;
	  for (i = 0; i < numRecs; i++){
	    if (callInstrAddr == erela[i].r_offset){ 
	      Elf_Scn* symscn;
	      Elf32_Shdr* symshdr;
	      Elf_Data *symdata;
	      Elf32_Sym* sym;
	      int numSyms, symNdx;
	      symscn = getELFSymbolTableScn(fdesc->elf);
	      symshdr = elf32_getshdr(symscn);
	      symdata = NULL;
	      symdata = elf_getdata(symscn, symdata);
	      numSyms = symdata->d_size/symshdr->sh_entsize;
	      symNdx = ELF32_R_SYM(erela[i].r_info);
	      if (symNdx >= numSyms){
		fprintf(stderr, "sos_sys_call_check: Invalid symbol table index in relocation entry\n");
		exit(EXIT_FAILURE);
	      }
	      sym = (Elf32_Sym*)((Elf32_Sym*)symdata->d_buf + symNdx);
	      *calltargetaddr = sym->st_value + erela[i].r_addend;
	      return -1;
	    }
	  }
	}
      }
    }
  }
  return 0;
}
