/**
 * \file elfdisplay.c
 * \brief Routines to display ELF files
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libelf.h>
#include <stringconst.h>

#include <elfdisplay.h>
#include <elfhelper.h>

//extern const char* archStrTab[];
//extern const int archNum[];
//extern const int archStrTabLen;
//extern int archNumNdx;

//--------------------------------------------------------------------------------
int ElfPrintHeader(Elf32_Ehdr *ehdr, 
		   int archNum[], int archStrTablLen, int archNumNdx, char* archStrTab[])
{
  printf("======= ELF HEADER ======================\n");
  // Print the indent information
  // Print magic number
  printf("Magic Number: ");
  printf("%x ", ehdr->e_ident[EI_MAG0]);
  printf("%c", ehdr->e_ident[EI_MAG1]);
  printf("%c", ehdr->e_ident[EI_MAG2]);
  printf("%c\n", ehdr->e_ident[EI_MAG3]);
  // Print class information
  printf("File Class: ");
  switch (ehdr->e_ident[EI_CLASS]){
  case ELFCLASSNONE: printf("Invalid Class\n"); break;
  case ELFCLASS32: printf("32-bit objects\n"); break;
  case ELFCLASS64: printf("64-bit objects\n"); break;
  default: printf("Corrupted ELF file\n"); break;
  }
  // Print file encoding
  printf("Data Encoding: ");
  switch (ehdr->e_ident[EI_DATA]){
  case ELFDATANONE: printf("Invalid data encoding\n"); break;
  case ELFDATA2LSB: printf("Little-Endian encoding\n"); break;
  case ELFDATA2MSB: printf("Big-Endian encoding\n"); break;
  default: printf("Corrupted ELF file\n"); break;
  }
  // Print File Version
  printf("File Version: %d\n", ehdr->e_ident[EI_VERSION]);
  // Print Object File Type
  printf("Object File Type: ");
  switch(ehdr->e_type){
  case ET_NONE: printf("No file type.\n"); break;
  case ET_REL: printf("Relocatable file\n"); break;
  case ET_EXEC: printf("Executable file\n"); break;
  case ET_DYN: printf("Shared object file\n"); break;
  case ET_CORE: printf("Core file\n"); break;
  case ET_LOPROC: printf("Processor specific\n"); break;
  case ET_HIPROC: printf("Processor specific\n"); break;
  default: printf("Corrupted ELF file\n"); break;
  }
  // Print Machine Architecture
  printf("Machine: ");
  if (ehdr->e_machine != archNum[archNumNdx]){
    printf("Not %s machine type.\n", archStrTab[archNumNdx]);
    exit(EXIT_FAILURE);
  }
  printf("%s Processor\n", archStrTab[archNumNdx]);
  // Version Number
  printf("Version: ");
  switch (ehdr->e_version){
  case EV_NONE: printf("Invalid Version\n"); break;
  case EV_CURRENT: printf("Current Version 1.0\n"); break;
  default: printf("Corrupted ELF file\n");
  }
  printf("\n");
  return 0;
}

//--------------------------------------------------------------------------------
int ElfPrintSections(Elf *elf)
{
  Elf32_Ehdr* ehdr;
  Elf_Scn *scn;
  Elf32_Shdr *shdr;



  // Get the ELF Header
  ehdr = elf32_getehdr(elf);
  if (NULL == ehdr){
    fprintf(stderr, "Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }  
  printf("======SECTION HEADER========\n");
  // Setion Header Table Offset
  printf("Section Header Offset: %d\n", ehdr->e_shoff);
  // Section Header Size
  printf("Section Header Size: %d\n", ehdr->e_shentsize);
  // Number of entries in section table
  printf("Section Header Entries: %d\n", ehdr->e_shnum);
  printf("\n");
  
  // Print Section Headers
  //      NNNNNSSSSSSSSSSSSSSSSSSSSTTTTTTTTTTAAAAAAAAAOOOOOOOOOSSSSSSSSSFFFFEEEEEAAALLLIIIII
  printf("No.  Name                Type      Addr     Offset   Size     Flg ES   Al Li Inf\n");

  scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL){
    if ((shdr = elf32_getshdr(scn)) != NULL){
      // Section Number
      printf("[%02d] ", (int)elf_ndxscn(scn));

      // Section Name
      char* secname = NULL;
      secname = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
      if (NULL != secname)
	printf("%-20.20s", secname);
      else
	printf("NULL");

      // Section Type
      if (shdr->sh_type <= SHT_DYNSYM)
	printf("%-10.10s", SecTypeStrTab[shdr->sh_type]);
      else
	printf("UNKNOWN");

      // Address
      printf("%08x ", shdr->sh_addr);

      // Offset
      printf("%08x ", shdr->sh_offset);

      // Size
      printf("%08x ", shdr->sh_size);

      // Flags
      printf("%-4.4s", SecFlagStrTab[shdr->sh_flags & 0x07]);

      // Entry Size
      printf("%-4x ", shdr->sh_entsize);

      // Alignment
      printf("%-2x ", shdr->sh_addralign);

      // Link
      printf("%-2x ", shdr->sh_link);

      // Info
      printf("%-5x ", shdr->sh_info);

      
      // Newline
      printf("\n");  
    }
  }
  printf("\n");
  return 0;
}

//--------------------------------------------------------------------------------
int ElfPrintSymTab(Elf *elf)
{
  Elf32_Ehdr* ehdr;
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Sym *esym;
  int numSymbols;

  // Get the ELF Header
  ehdr = elf32_getehdr(elf);
  if (NULL == ehdr){
    fprintf(stderr, "Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }  
  scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL){
    if ((shdr = elf32_getshdr(scn)) != NULL){
      if (SHT_SYMTAB == shdr->sh_type){
	edata = NULL;
	while ((edata = elf_getdata(scn, edata)) != NULL){
	  printf("======SYMBOL TABLE=============\n");
	  if (ELF_T_SYM == edata->d_type){
	    esym = (Elf32_Sym*)edata->d_buf;
	    numSymbols = edata->d_size/shdr->sh_entsize;
	    printf("Symbol table size (Bytes): %d\n", shdr->sh_size);
	    printf("Number of symbols: %d\n", numSymbols);
	    
	    int i;
	    printf("\n");
	    //      NNNNNVVVVVVVVVZZZZZZZBBBBBBBBTTTTTTTTXXXXSSSSSSSSSSSSSSS
	    printf("Num. Value      Size Bind    Type    Ndx Name\n");
	    for (i = 1; i < numSymbols; i++){
	      char* symName;
	      // Symbol No.
	      printf("%03d: ", i);

	      // Print the symbol
	      ElfPrintSymbol(&esym[i]);
	      
	      // Symbol Name
	      if (esym[i].st_name != 0){
		symName = elf_strptr(elf, shdr->sh_link, esym[i].st_name);
		if (NULL != symName)
		  printf("%-20.20s", symName);
	      }

	      // Newline
	      printf("\n");
	    }
	  }
	}
      }
    }
  }
  printf("\n");
  return 0;
}

//--------------------------------------------------------------------------------
int ElfPrintReloc(Elf* elf)
{
  Elf32_Ehdr* ehdr;
  Elf_Scn *scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;
  Elf32_Rela *erela;
  int numRecs;

  // Get the ELF Header
  ehdr = elf32_getehdr(elf);
  if (NULL == ehdr){
    fprintf(stderr, "Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }  
  scn = NULL;
  while ((scn = elf_nextscn(elf, scn)) != NULL){
    if ((shdr = elf32_getshdr(scn)) != NULL){
      if (SHT_RELA == shdr->sh_type){  
	char* relasecname;
	relasecname = elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name);
	printf("===== RELA: %s ========\n", relasecname);
	edata = NULL;
	while ((edata = elf_getdata(scn, edata)) != NULL){
	  if (ELF_T_RELA == edata->d_type){
	    erela = (Elf32_Rela*)edata->d_buf;
	    numRecs = edata->d_size/shdr->sh_entsize;
	    printf("Relocation for section %d\n", shdr->sh_info);
	    printf("Relocation Table size (Bytes): %d\n", shdr->sh_size);
	    printf("Number of records: %d\n", numRecs);

	    int i;
	    printf("\n");
	    //      OOOOOOOOOTTTTSSSSSSSSAAAAAAAAA
	    printf("Offset   Typ Symbl.  Addend\n");
	    for (i = 0; i < numRecs; i++){
	      // Offset
	      printf("%08x ", erela[i].r_offset);
	      // Type
	      printf("%03x ", ELF32_R_TYPE(erela[i].r_info));
	      // Sym
	      printf("%07d ", ELF32_R_SYM(erela[i].r_info));
	      // Addend
	      printf("%08x ", (int)erela[i].r_addend);
	      // Newline
	      printf("\n");
	    }

	  }
	}
      }
    }
  }
  printf("\n");
  return 0;
}
//--------------------------------------------------------------------------------
int ElfPrintSymbol(Elf32_Sym *esym)
{
  // Value
  printf("%08x ", esym->st_value);
  
  // Size
  printf("%6d ", esym->st_size);
  
  // Binding
  if (ELF32_ST_BIND(esym->st_info) <= STB_WEAK)
    printf("%-8.8s", SymBindStrTab[ELF32_ST_BIND(esym->st_info)]);
  else
    printf("UNKNOWN ");
  
  // Type
  if (ELF32_ST_TYPE(esym->st_info) <= STT_FILE)
    printf("%-8.8s", SymTypeStrTab[ELF32_ST_TYPE(esym->st_info)]);
  else
    printf("UNKNOWN ");
  
  // Index
  switch (esym->st_shndx){
  case SHN_ABS: printf("ABS "); break;
  case SHN_COMMON: printf("CMN "); break;
  case SHN_UNDEF: printf("UND "); break;
  default: printf("%3d ", esym->st_shndx); break;
  }

  return 0;
}
