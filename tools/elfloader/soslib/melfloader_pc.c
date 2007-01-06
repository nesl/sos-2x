/**
 * \file melfloader_pc.c
 * \brief Mini-ELF loader for SOS on PC
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <string.h>
#include <minielf.h>
#include <melfloader.h>
#include <minielfendian.h>


//#define DBGMODE
#ifdef DBGMODE
#define STB_WEAK	2
#define STT_FILE	4
#include <stringconst.h>
#endif

mod_header_t* melf_get_mod_header(unsigned char* image_buf)
{
  Melf_Mhdr mhdr;
  Melf_Shdr *shdr, symshdr, progshdr;
  Melf_Sym *pmodhdrsym, modhdrsym;
  int i;

  memcpy((void*)&mhdr, (void*)image_buf, sizeof(Melf_Mhdr));
  entoh_Mhdr(&mhdr);
#ifdef DBGMODE
  printf("==== MELF HEADER =============\n");
  printf("No. of sections: %d \n", mhdr.m_shnum);
  printf("Index of mod_header: %d \n", mhdr.m_modhdrndx);
#endif

  shdr = (Melf_Shdr*)(&image_buf[sizeof(Melf_Mhdr)]);

#ifdef DBGMODE
  printf("===== SECTION HEADER TABLE =======\n");
  // Print Section Headers
  //      IIIITTTTTTTTTTTOOOOOOOOOSSSSSSLLLLIIIINNNNNNNNNNNNNNNNNNNN
  printf("Id. Type       Offset   Size  Li  Inf Name\n");
#endif

  for (i = 0; i < mhdr.m_shnum; i++){    
    if (SHT_SYMTAB == shdr[i].sh_type){
      memcpy((void*)&symshdr, (void*)&(shdr[i]), sizeof(Melf_Shdr));
      entoh_Shdr(&symshdr);
    }
    if (SHT_PROGBITS == shdr[i].sh_type){
      memcpy((void*)&progshdr, (void*)&(shdr[i]), sizeof(Melf_Shdr));
      entoh_Shdr(&progshdr);
    }
#ifdef DBGMODE
    {
      Melf_Shdr printshdr;
      memcpy((void*)&printshdr, (void*)&(shdr[i]), sizeof(Melf_Shdr));
      entoh_Shdr(&printshdr);
      // Print Section Id
      printf("%03d ", (int)printshdr.sh_id);
      // Print Section Type
      if (printshdr.sh_type <= SHT_DYNSYM)
	printf("%-10.10s ", SecTypeStrTab[printshdr.sh_type]);
      else
	printf("UNKNOWN   ");
      // Print Offset
      printf("%08d ", printshdr.sh_offset);
      // Print Size
      printf("%05d ", printshdr.sh_size);
      // Print Link
      printf("%03d ", printshdr.sh_link);
      // Print Info
      printf("%03d ", printshdr.sh_info);
      // Newline
      printf("\n");
    }
#endif
  }

  // Get the pointer to the mod header symbol
  pmodhdrsym = (Melf_Sym*)&(image_buf[symshdr.sh_offset]);
  pmodhdrsym += mhdr.m_modhdrndx;
  memcpy((void*)&modhdrsym, (void*)pmodhdrsym, sizeof(Melf_Sym));
  entoh_Sym(&modhdrsym);

#ifdef DBGMODE
  printf("======== MOD HEADER SYMBOL ========\n");
  //      NNNNNVVVVVVVVVBBBBBBBBBTTTTTTTTTIIII
  printf("No.  Value    Bind     Type     Id.\n");
  // Symbol No.
  printf("%03d: ", mhdr.m_modhdrndx);
  
  // Value
  printf("%08x ", modhdrsym.st_value);
  
  // Binding
  if (MELF_ST_BIND(modhdrsym.st_info) <= STB_WEAK)
    printf("%-8.8s ", SymBindStrTab[MELF_ST_BIND(modhdrsym.st_info)]);
  else
    printf("UNKNOWN  ");
  
  // Type
  if (MELF_ST_TYPE(modhdrsym.st_info) <= STT_FILE)
    printf("%-8.8s ", SymTypeStrTab[MELF_ST_TYPE(modhdrsym.st_info)]);
  else
    printf("UNKNOWN  ");
  
  // Id.
  printf("%3d ", modhdrsym.st_shid);

  // Newline
  printf("\n");
#endif

  return (mod_header_t*)(&(image_buf[progshdr.sh_offset + modhdrsym.st_value]));
  
}

//----------------------------------------------------------
