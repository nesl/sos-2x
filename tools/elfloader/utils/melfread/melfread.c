/**
 * \file melfread.c
 * \brief Mini-ELF Reader
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <minielflib.h>
#include <minielf.h>
#include <libelf.h>
#include <dispmelf.h>

static int MelfReadFile(char* fname);

int main(int argc, char** argv)
{
  MelfReadFile(argv[1]);
  return 0;
}

static int MelfReadFile(char* fname)
{
  int fd;
  Melf* melf;
  
  printf("Filename: %s\n", fname);

  // Open MELF file for reading
  if ((fd = open(fname, O_RDONLY)) < 0){
    perror("fopen: ");
    exit(EXIT_FAILURE);
  }

  // Get the MELF descriptor
  if ((melf = melf_begin(fd)) == NULL){
    fprintf(stderr, "Error getting MELF descriptor\n");
    exit(EXIT_FAILURE);
  }

  // Print MELF Header
  PrintMelfHdr(melf);

  // Print MELF Section Table
  PrintMelfSecTable(melf);

  // Print MELF Symbol Table
  PrintMelfSymTable(melf);

  // Print RELA table
  PrintMelfRelaTable(melf);

  return 0;
}

