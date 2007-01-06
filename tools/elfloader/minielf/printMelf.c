/**
 * \file printmelf.c
 * \brief Routines to pretty print MELF files
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>
#include <minielflib.h>
#include <minielf.h>
#include <libelf.h>
#include <stringconst.h>
#include <printMelf.h>
#include <elfhelper.h>

int PrintMelfHdr(Melf* MelfDesc)
{
  Melf_Mhdr* mhdr;
  if ((mhdr = melf_getmhdr(MelfDesc)) == NULL){
    fprintf(stderr, "Error getting MELF header\n");
    return -1;
  }
  printf("==== MELF HEADER =============\n");
  printf("No. of sections: %d \n", mhdr->m_shnum);
  printf("Index of mod_header: %d \n", mhdr->m_modhdrndx);
  return 0;
}


int PrintMelfSecTable(Melf* MelfDesc)
{
  Melf_Scn* pCurrScn;
  

  printf("===== SECTION HEADER TABLE =======\n");
  // Print Section Headers
  //      IIIITTTTTTTTTTTOOOOOOOOOSSSSSSLLLLIIIINNNNNNNNNNNNNNNNNNNN
  printf("Id. Type       Offset   Size  Li  Inf Name\n");
  pCurrScn = NULL;
  while ((pCurrScn = melf_nextscn(MelfDesc, pCurrScn)) != NULL){
    // Print Section Id
    printf("%03d ", (int)melf_idscn(pCurrScn));
    // Print Section Type
    if (pCurrScn->m_shdr.sh_type <= SHT_DYNSYM)
      printf("%-10.10s ", SecTypeStrTab[pCurrScn->m_shdr.sh_type]);
    else
      printf("UNKNOWN   ");
    // Print Offset
    printf("%08d ", pCurrScn->m_shdr.sh_offset);
    // Print Size
    printf("%05d ", pCurrScn->m_shdr.sh_size);
    // Print Link
    printf("%03d ", pCurrScn->m_shdr.sh_link);
    // Print Info
    printf("%03d ", pCurrScn->m_shdr.sh_info);
    /*
    // Print Section Name
    if (NULL != elf){
      char* secName = getELFSectionName(elf, pCurrScn->m_shdr.sh_id);
      if (NULL != secName)
	printf("%-20.20s", secName);
    }
    */
    // Newline
    printf("\n");
  }
  printf("\n");
  return 0;
}


int PrintMelfSymTable(Melf* MelfDesc)
{
  Melf_Scn* scn;
  Melf_Shdr* shdr;
  Melf_Data* mdata;
  Melf_Sym* msym;
  
  scn = NULL;
  while ((scn = melf_nextscn(MelfDesc, scn)) != NULL){
    if ((shdr = melf_getshdr(scn)) != NULL){
      if (SHT_SYMTAB == scn->m_shdr.sh_type){
	if ((mdata = melf_getdata(scn)) != NULL){
	  if (ELF_T_SYM == mdata->d_type){
	    int i;
	    msym = (Melf_Sym*)mdata->d_buf;
	    printf("======== SYMBOL TABLE ========\n");
	    printf("Symbol Table Size (Bytes) %d\n", mdata->d_size);
	    printf("Number of symbols: %d\n", mdata->d_numData);
	    //      NNNNNVVVVVVVVVBBBBBBBBBTTTTTTTTTIIII
	    printf("No.  Value    Bind     Type     Id.\n");
	    for (i = 0; i < mdata->d_numData; i++){
	      // Symbol No.
	      printf("%03d: ", i);

	      // Value
	      printf("%08x ", msym[i].st_value);

	      // Binding
	      if (MELF_ST_BIND(msym[i].st_info) <= STB_WEAK)
		printf("%-8.8s ", SymBindStrTab[MELF_ST_BIND(msym[i].st_info)]);
	      else
		printf("UNKNOWN  ");

	      // Type
	      if (MELF_ST_TYPE(msym[i].st_info) <= STT_FILE)
		printf("%-8.8s ", SymTypeStrTab[MELF_ST_TYPE(msym[i].st_info)]);
	      else
		printf("UNKNOWN  ");

	      // Id.
	      printf("%3d ", msym[i].st_shid);
	      
	      // Newline
	      printf("\n");
	    }
	  }
	}
      }
    }
  }
  return 0;
}



int PrintMelfRelaTable(Melf* MelfDesc)
{
  Melf_Scn* scn;
  Melf_Shdr* shdr;
  Melf_Data* mdata;
  Melf_Rela* mrela;

  scn = NULL;
  while ((scn = melf_nextscn(MelfDesc, scn)) != NULL){
    if ((shdr = melf_getshdr(scn)) != NULL){
      if (SHT_RELA == scn->m_shdr.sh_type){
	printf("==== RELA: %d =========\n", shdr->sh_id);
	if ((mdata = melf_getdata(scn)) != NULL){
	  if (ELF_T_RELA == mdata->d_type){
	    int i;
	    printf("Relocation for section: %d\n", shdr->sh_info);
	    printf("Relocation Table Size: %d\n", mdata->d_size);
	    printf("Number of records: %d\n", mdata->d_numData);
	    mrela = (Melf_Rela*)mdata->d_buf;
	    //      OOOOOOOOOTTTTSSSSSSSSAAAAAAAAA
	    printf("Offset   Typ Symbl.  Addend\n");
	    for (i = 0; i < mdata->d_numData; i++){
	      // Offset
	      printf("%08x ",mrela[i].r_offset);
	      // Type
	      printf("%03x ", mrela[i].r_type);
	      // Symbol
	      printf("%07d ", mrela[i].r_symbol);
	      // Addend
	      printf("%08x ", mrela[i].r_addend);
	      // Newline
	      printf("\n");
	    }
	  }
	}
      }
    }
  }
  return 0;
}
