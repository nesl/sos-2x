/**
 * \file elfread.c
 * \brief Reads an ELF32 format file
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <libelf.h>
#include <stringconst.h>
#include <elfdisplay.h>
#include <elfhelper.h>

#define PELFHDR 0x0001
#define PSECHDR 0x0002
#define PSYMTAB 0x0004
#define PRELSEC 0x0008

static int ElfRead(char* fname, int poptions);
static int printusage();

char* archStrTab[] = {"avr"};
int archNum[] = {EM_AVR};
int archStrTabLen = 1;
int archNumNdx;

int main(int argc, char** argv)
{
  int ch;
  int i;
  int poptions = 0;
  char* archname;

  while ((ch = getopt(argc, argv, "haesyrm:")) != -1){
    switch (ch){
    case 'a': poptions = (PELFHDR | PSECHDR | PSYMTAB | PRELSEC); break;
    case 'e': poptions |= PELFHDR; break;
    case 's': poptions |= PSECHDR; break;
    case 'y': poptions |= PSYMTAB; break;
    case 'r': poptions |= PRELSEC; break;
    case 'm': archname = optarg; break;
    case 'h': case '?':
      printusage();
      exit(EXIT_FAILURE);
    }
  }
  argc -= optind;
  argv += optind;
  
  for (i = 0; i < archStrTabLen; i++){
    if (strcmp(archname, archStrTab[i]) == 0){
      archNumNdx = i;
      ElfRead(argv[0], poptions);
      return 0;
    }
  }
  printusage();
  return 0;
}

//--------------------------------------------------------------------------------
static int printusage()
{
  int i;
  printf("ELF Reader Command Line Options:\n");
  printf("elfread [-aesyh] -m <arch name> elf-file-name\n");
  printf("-h       Print this help\n");
  printf("-a       Print all headers\n");
  printf("-e       Print ELF header\n");
  printf("-s       Print Section Table\n");
  printf("-y       Print Symbol Table\n");
  printf("-r       Print Relocation Table\n");
  printf("-m       Architecture Name\n");
  printf("Supported Arch: ");
  for (i = 0; i < archStrTabLen; i++){
    printf("%s ", archStrTab[i]);
  }
  printf("\n");
  return 0;
}

//--------------------------------------------------------------------------------
static int ElfRead(char* fname, int poptions)
{
  int fd;
  Elf* elf;
  Elf32_Ehdr *ehdr;

  printf("Filename: %s\n", fname);

  if (elf_version(EV_CURRENT) == EV_NONE){
    fprintf(stderr, "Library version is out of date.\n");
    exit(EXIT_FAILURE);
  }


  // Open ELF file for reading
  fd = open(fname, O_RDONLY);
  if (fd < 0){
    perror("fopen:");
    exit(EXIT_FAILURE);
  }
  
  // Get the ELF descriptor
  elf = elf_begin(fd, ELF_C_READ, NULL);
  if (NULL == elf){
    fprintf(stderr, "elf_begin: Error getting elf descriptor\n");
    exit(EXIT_FAILURE);
  }

  // Ensure that it is an ELF format file
  if (elf_kind(elf) != ELF_K_ELF){
    fprintf(stderr, "This program can only read ELF format files.\n");
    exit(EXIT_FAILURE);
  }

  // Get the ELF Header
  ehdr = elf32_getehdr(elf);
  if (NULL == ehdr){
    fprintf(stderr, "Error reading ELF header\n");
    exit(EXIT_FAILURE);
  }

  // Verify if the header is AVR specific
  if (ehdr->e_machine != archNum[archNumNdx]){
    fprintf(stderr, "This ELF binary is not for %s.\n", archStrTab[archNumNdx]);
    exit(EXIT_FAILURE);
  }

  // Print the ELF Header
  if (poptions & PELFHDR)
    ElfPrintHeader(ehdr, archNum, archStrTabLen, archNumNdx, archStrTab);

  // Print Section Headers
  if (poptions & PSECHDR)
    ElfPrintSections(elf);

  // Print Symbol Table
  if (poptions & PSYMTAB)
    ElfPrintSymTab(elf);

  // Print Relocation Table
  if (poptions & PRELSEC)
    ElfPrintReloc(elf);

  // Close the ELF descriptor
  elf_end(elf);

  return 0;
}


