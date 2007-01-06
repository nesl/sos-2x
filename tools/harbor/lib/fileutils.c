/**
 * \file fileutils.c
 * \brief Utilities for handling input binary files
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <libelf.h>


#include <fileutils.h>
#include <elfhelper.h>

//----------------------------------------------------------------------------
// DEBUG
#define DBGMODE
#ifdef DBGMODE
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif

//-------------------------------------------------------------------
int obj_file_open(char* filename, file_desc_t* fdesc)
{
  char *fileExt;
  Elf_Scn* scn;
  Elf32_Shdr *shdr;
  Elf_Data *edata;


  // Determine file type
  // .elf = ELF_FILE
  // .sos = BIN_FILE
  fileExt = strrchr(filename, '.');
  if (NULL == fileExt){
    fprintf(stderr,"Invalid file name.\n");
    return -1;
  }
  if (strcmp(fileExt,".sos") == 0)
    fdesc->type = BIN_FILE;
  else if (strcmp(fileExt,".elf") == 0)
    fdesc->type = ELF_FILE;
  else {
    fprintf(stderr, "File is not of supported type.\n");
    fprintf(stderr, "Supported types are .elf and .sos \n");
    return -1;
  }
  // Copy file name
  fdesc->name = (char*)malloc(strlen(filename) + 1);
  strcpy(fdesc->name, filename);
  
  // Open the file
  if (BIN_FILE == fdesc->type){
    // Binary File
    if ((fdesc->fd = fopen((char*)filename, "r")) == NULL){
      fprintf(stderr, "%s does not exist.\n", filename);
      return -1;
    }
  }
  else {
    // ELF File
    int fd;
    if (elf_version(EV_CURRENT) == EV_NONE){
      fprintf(stderr, "Library version is out of date.\n");
      return -1;
    }

    // Open ELF file for reading
    fd = open(filename, O_RDONLY);
    if (fd < 0){
      perror("fopen:");
      return -1;
    }
  
    // Get the ELF descriptor
    fdesc->elf = elf_begin(fd, ELF_C_READ, NULL);
    if (NULL == fdesc->elf){
      fprintf(stderr, "elf_begin: Error getting elf descriptor\n");
      return -1;
    }

    // Ensure that it is an ELF format file
    if (elf_kind(fdesc->elf) != ELF_K_ELF){
      fprintf(stderr, "This program can only read ELF format files.\n");
      return -1;
    }

    // Get progbuff
    // Assuming .text section contains the entire program
    scn = getELFSectionByName(fdesc->elf, ".text");
    if (NULL == scn){
      fprintf(stderr, "obj_file_seek: Cannot locate .text section in ELF file.\n");
      exit(EXIT_FAILURE);
    }
    if ((shdr = elf32_getshdr(scn)) != NULL){
      if (SHT_PROGBITS == shdr->sh_type){
	edata = NULL;
	while ((edata = elf_getdata(scn, edata)) != NULL){
	  if (ELF_T_BYTE == edata->d_type){
	    fdesc->progdata = edata;
	    fdesc->progbyte = (uint8_t*)edata->d_buf;
	    DEBUG("Size of binary in bytes: %d\n", (int)fdesc->progdata->d_size);
	  }
	}
      }
    }   
  }
  return 0;
}

//-------------------------------------------------------------------
int obj_file_close(file_desc_t* fdesc)
{
  if (BIN_FILE == fdesc->type)
    fclose(fdesc->fd);
  else{
    elf_end(fdesc->elf);
  }
  return 0;
}

//-------------------------------------------------------------------
int obj_file_seek(file_desc_t* fdesc, uint32_t addr, int whence)
{
  if (BIN_FILE == fdesc->type)
    return fseek(fdesc->fd, addr, whence);
  // Elf File
  uint32_t currOffset = fdesc->progbyte - (uint8_t*)fdesc->progdata->d_buf;
  switch (whence){
  case SEEK_CUR:
    if (((currOffset + addr) < 0) || ((currOffset + addr) > fdesc->progdata->d_size))
      return -1;
    fdesc->progbyte += addr;
    break;
  case SEEK_SET:
    if ((addr < 0) || (addr > fdesc->progdata->d_size))
      return -1;
    fdesc->progbyte = ((uint8_t*)fdesc->progdata->d_buf) + addr;
    break;
  case SEEK_END:
    if ((addr > 0) || (addr < (-1 * fdesc->progdata->d_size)))
      return -1;
    fdesc->progbyte = ((uint8_t*)(fdesc->progdata->d_buf)) + fdesc->progdata->d_size + addr;
    break;
  default:
    break;
  }
  return 0;
}

//-------------------------------------------------------------------
int obj_file_read(file_desc_t* fdesc, void* ptr, size_t size, size_t nmemb)
{
  if (BIN_FILE == fdesc->type)
    return fread(ptr, size, nmemb, fdesc->fd);
  // ELF File
   uint32_t currOffset;
   currOffset = fdesc->progbyte - (uint8_t*)fdesc->progdata->d_buf;
   if ((currOffset + (size * nmemb)) > fdesc->progdata->d_size){
     DEBUG("Curr Offset: %d, Size: %d, Read Size: %d\n", 
	   (int)currOffset, (int)fdesc->progdata->d_size, (int)(size * nmemb)); 
     return 0;
   }
   memcpy(ptr, fdesc->progbyte, size * nmemb);
   fdesc->progbyte += size * nmemb;
   return nmemb;
}
