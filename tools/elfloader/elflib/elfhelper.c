/**
 * \file elfhelper.c
 * \brief Useful ELF file manipulation routines
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <string.h>
#include <libelf.h>
#include <elfhelper.h>

//#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

//---------------------------------------------------------------------
Elf_Scn* getELFSymbolTableScn(Elf* elf)
{
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  
  scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL){
    if ((shdr = elf32_getshdr(scn)) != NULL){
      if (SHT_SYMTAB == shdr->sh_type){
	edata = NULL;
	while ((edata = elf_getdata(scn, edata)) != NULL){
  	  if (ELF_T_SYM == edata->d_type){
	    DEBUG("No. of symbols: %d\n", (int)(edata->d_size/shdr->sh_entsize));
	    return scn;
	    //return (Elf32_Sym*)(edata->d_buf);
	  }
	}
      }
    }
  }
  return NULL;
}
//---------------------------------------------------------------------
int getELFSymbolTableNdx(Elf* elf, char* symName)
{
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Sym *esym;


  scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL){
    if ((shdr = elf32_getshdr(scn)) != NULL){
      if (SHT_SYMTAB == shdr->sh_type){
	edata = NULL;
	while ((edata = elf_getdata(scn, edata)) != NULL){
  	  if (ELF_T_SYM == edata->d_type){
	    int numSymbols, i;
	    char* symNameRead;
	    esym = (Elf32_Sym*)(edata->d_buf);
	    numSymbols = (int)(edata->d_size/shdr->sh_entsize);
	    for (i = 0; i < numSymbols; i++){
	      symNameRead = elf_strptr(elf, shdr->sh_link, esym[i].st_name);
	      if (strcmp(symNameRead, symName) == 0){
		return i;
	      }
	    }
	  }
	}
      }
    }
  }
  fprintf(stderr, "getElfSymbolTableNdx: Could not locate symbol in the symbol table.\n");
  return -1;
}

//---------------------------------------------------------------------
Elf_Scn* getELFSectionByName(Elf* elf, char* secName)
{
  Elf32_Ehdr* ehdr;
  Elf_Scn *scn;
  Elf32_Shdr *shdr;

  // Get the ELF Header
  ehdr = elf32_getehdr(elf);
  if (NULL == ehdr){
    fprintf(stderr, "getELFSectionByName: Error reading ELF header\n");
    return NULL;
  }  

  scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL){
    if ((shdr = elf32_getshdr(scn)) != NULL){
      char* CurrSecName = NULL;
      CurrSecName = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
      if (strcmp(CurrSecName, secName) == 0)
	return scn;
    }
  }

  return NULL;
}
//---------------------------------------------------------------------
char* getELFSectionName(Elf* elf, int ndx)
{
  Elf32_Ehdr* ehdr;
  Elf_Scn *scn;
  Elf32_Shdr *shdr;

  if ((ehdr = elf32_getehdr(elf)) == NULL){
    fprintf(stderr, "Error reading ELF header\n");
    return NULL;
  }  
  if ((scn = elf_getscn(elf, ndx)) == NULL){
    fprintf(stderr, "Error reading ELF section table.\n");
    return NULL;
  }
  if ((shdr = elf32_getshdr(scn)) == NULL){
    fprintf(stderr, "Error reading ELF section header.\n");
    return NULL;
  };

  return elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
}
//---------------------------------------------------------------------
