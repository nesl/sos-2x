/**
 * \file sos_module_header_patch.c
 * \brief Routine to patch the SOS module header
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>

#include <libelf.h>

#include <fileutils.h>
#include <elfdisplay.h>
#include <elfhelper.h>
#include <basicblock.h>
#include <sos_mod_header_patch.h>
#include <sos_mod_patch_code.h>


typedef struct harbor_sos_func_cb_t {
  uint16_t ptr;    //! function pointer                    
  uint8_t proto[4]; //! function prototype                  
  uint8_t pid;      //! function PID                                    
  uint8_t fid;      //! function ID                         
} PACK_STRUCT  harbor_sos_func_cb_t;

typedef struct harbor_sos_mod_header_t {
  uint16_t state_size;     //!< module state size
  uint8_t mod_id;          //!< module ID (used for messaging).  Set NULL_PID for system selected mod_id
  uint8_t num_timers;      //!< Number of timers to be reserved at module load time
  uint8_t num_sub_func;	   //!< number of functions to be subscribed
  uint8_t num_prov_func;   //!< number of functions provided
  uint8_t num_dfunc;       //!< number of direct linked functions
  uint8_t version;         //!< version number, for users bookkeeping
  uint16_t code_id;        //!< module image identifier
  uint8_t processor_type;  //!< processor type of this module
  uint8_t platform_type;   //!< platform type of this module
  uint8_t num_out_port;     //!< Number of output ports exposed by the module in ViRe framework.
  uint8_t padding;          //!< Extra padding to make it word aligned.
  uint16_t module_handler;//!< Message Handler
  harbor_sos_func_cb_t funct[];
} PACK_STRUCT harbor_sos_mod_header_t;

//----------------------------------------------------------------------------
// DEBUG
//#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

//----------------------------------------------------------------------------
// STATIC FUNCTIONS
static uint32_t bin_find_module_start_addr(file_desc_t* fdesc);
static uint32_t elf_find_module_start_addr(file_desc_t* fdesc);


// Ram - This function will produce incorrect results
// This is due to the change in the module header
void sos_patch_mod_header(bblklist_t* blist, uint8_t* mhdr)
{
  int i;
  uint32_t oldaddr, newaddr;
  basicblk_t* bblk;
  harbor_sos_mod_header_t* modhdr;
  harbor_sos_func_cb_t* pfuncb;
  uint16_t numfuncs;
  uint16_t addrval;

  modhdr = (harbor_sos_mod_header_t*)mhdr;
  numfuncs = modhdr->num_prov_func + modhdr->num_sub_func;
  DEBUG("========= Patching SOS Module Header ========\n");
  DEBUG("Number of functions: %d\n", numfuncs);
       
  for (i = 0, pfuncb = modhdr->funct; i < numfuncs; i++, pfuncb ++){

    
    addrval = pfuncb->ptr;
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
    pfuncb->ptr = addrval;
    DEBUG("New Addr: 0x%x\n", (int)newaddr);
  }
  
  // Now patch message handler
  addrval = modhdr->module_handler;
  oldaddr = (uint32_t)(((uint32_t)addrval) << 1);
  DEBUG("Message Handler Old Addr: 0x%x\n", oldaddr);
  newaddr = find_updated_address(blist, oldaddr);
  DEBUG("Message Handler New Addr: 0x%x\n", newaddr);
  addrval = (uint16_t)(uint32_t)(newaddr >> 1);
  modhdr->module_handler = addrval;
  return;
}

uint32_t find_module_start_addr(file_desc_t* fdesc){
  if (BIN_FILE == fdesc->type)
    return bin_find_module_start_addr(fdesc);
  else 
    return elf_find_module_start_addr(fdesc);
}

static uint32_t bin_find_module_start_addr(file_desc_t* fdesc)
{
  fprintf(stderr, "bin_find_module_start_addr: This function is not implemented yet !!\n");
  exit(EXIT_FAILURE);
  return 0;

#if 0
  uint32_t handler_addr;
  uint8_t mhdr[FUNC_CB_LIST_POS];

  if (fread(mhdr, sizeof(uint8_t), FUNC_CB_LIST_POS, fdesc->fd) == 0){
    fprintf(stderr, "bin_find_sos_module_handler_addr: Premature end of file %s\n", fdesc->name);
    exit(EXIT_FAILURE);
  }

  handler_addr = (uint32_t)(sos_mod_hdr_mod_handler(mhdr));
  handler_addr <<= 1;
  return handler_addr;
#endif

}


static uint32_t elf_find_module_start_addr(file_desc_t* fdesc)
{
  Elf_Scn *scn, *symscn;
  Elf32_Shdr *shdr, *symshdr;
  Elf_Data *edata, *symdata;
  uint32_t modheader_addr;
  //  int modhdrndx;
  Elf32_Sym *msym, *symarr;
  uint8_t* etext;

  // Get module header
  symscn = NULL;
  while ((symscn = elf_nextscn(fdesc->elf, symscn)) != NULL){
    if ((symshdr = elf32_getshdr(symscn)) != NULL){
      if (SHT_SYMTAB == symshdr->sh_type){
	symdata = NULL;
	while ((symdata = elf_getdata(symscn, symdata))!= NULL){
	  if (ELF_T_SYM == symdata->d_type){
	    int numSymbols, i;
	    char* symNameRead;
	    symarr = (Elf32_Sym*)(symdata->d_buf);
	    numSymbols = (int)(symdata->d_size/symshdr->sh_entsize);
	    for (i = 0; i < numSymbols; i++){
	      symNameRead = elf_strptr(fdesc->elf, symshdr->sh_link, symarr[i].st_name);
	      if (strcmp(symNameRead, "mod_header") == 0){
		msym = &symarr[i];
		break;
	      }
	    }
	  }
	}
      }
    }
  }

  modheader_addr =  msym->st_value;
  

  scn = getELFSectionByName(fdesc->elf, ".text");
  if (NULL == scn){
    fprintf(stderr, "elf_find_sos_module_handler_addr: Cannot determine start address.\n");
    exit(EXIT_FAILURE);
  }

  if ((shdr = elf32_getshdr(scn)) != NULL){
    if (SHT_PROGBITS == shdr->sh_type){  
      edata = NULL;
      while ((edata = elf_getdata(scn, edata)) != NULL){
	if (ELF_T_BYTE == edata->d_type){
	  etext = (uint8_t*)edata->d_buf;
	  harbor_sos_mod_header_t* modhdr;
	  int numfuncs;
	  uint32_t startaddr;
	  modhdr = (harbor_sos_mod_header_t*)(&etext[modheader_addr]);
	  numfuncs = modhdr->num_prov_func + modhdr->num_sub_func;
	  /*
	  printf("Dom ID. = %d\n", modhdr->mod_id);
	  printf("Code ID. = %d\n", modhdr->code_id);
	  printf("Plat. Type = %d\n", modhdr->platform_type);
	  printf("Proc. Type = %d\n", modhdr->processor_type);
	  printf("State Size = %d\n", modhdr->state_size);
	  printf("Num. Sub Func. = %d\n", modhdr->num_sub_func);
	  printf("Num. Prov. Func. = %d\n", modhdr->num_prov_func); 
	  printf("Num Funcs. = %d\n", numfuncs);
	  */
	  printf("Offsetof %d\n", (int)offsetof(harbor_sos_mod_header_t, funct));
	  printf("Sizeof %d\n", (int)sizeof(harbor_sos_func_cb_t)*numfuncs);
	  startaddr = offsetof(harbor_sos_mod_header_t, funct) + sizeof(harbor_sos_func_cb_t)*numfuncs;
	  return startaddr;
	}
      }
    }
  }
  fprintf(stderr, "elf_find_sos_module_handler_addr: Cannot find messange handler.\n");
  exit(EXIT_FAILURE);
  return 0;
}

// Returns 0 if the call/jmp/rcall/rjmp has relocation record
int8_t check_calljmp_has_reloc_rec(file_desc_t* fdesc, uint32_t callInstrAddr, uint32_t* calltargetaddr)
{
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Rela *erela;
  int numRecs;

  if (BIN_FILE == fdesc->type) return 0;
  
  scn = getELFSectionByName(fdesc->elf, ".rela.text");
  if (NULL == scn){
    fprintf(stderr, "check_calljmp_has_reloc_rec: Cannot find relocation section in ELF file.\n");
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
		fprintf(stderr, "check_calljmp_has_reloc_rec: Invalid symbol table index in relocation entry\n");
		exit(EXIT_FAILURE);
	      }
	      sym = (Elf32_Sym*)((Elf32_Sym*)symdata->d_buf + symNdx);
	      *calltargetaddr = sym->st_value + erela[i].r_addend;
	      return 0;
	    }
	  }
	}
      }
    }
  }
  return -1;
}


