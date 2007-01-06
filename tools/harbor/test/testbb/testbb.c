/**
 * \file testbb.c
 * \brief Test of Basic Block Formation
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <basicblock.h>

void printusage();
int testbb(char* binFileName, uint32_t startaddr);


int main(int argc, char** argv)
{
  int ch;
  uint32_t startaddr;
  
  while ((ch = getopt(argc, argv, "s:")) != -1){
    switch (ch){
    case 's': startaddr = (uint32_t)strtol(optarg, NULL, 0); break;
    case '?':
      printusage();
      return 0;
    }
  }
  argv += optind;
  return testbb(argv[0], startaddr);
}



int testbb(char* binFileName, uint32_t startaddr)
{
  FILE *binFile;
  binFile = fopen((char*)binFileName, "r");

  if (NULL == binFile){
    fprintf(stderr, "%s does not exist\n", binFileName);
    return -1;
  }
  create_cfg(binFile, startaddr);
  return 0;
}

 void printusage()
 {
    printf("Usage testbb -s <startaddr> <Filename> \n");
    printf("Input file should be a SOS binary.\n");   
 }
