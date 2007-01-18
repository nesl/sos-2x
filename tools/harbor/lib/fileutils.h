/**
 * \file fileutils.h
 * \brief Utilities for handling File IO operations
 * \author Ram Kumar {ram@ee.ucla.edu}
 */


#ifndef _FILEUTILS_H_
#define _FILEUTILS_H_

#include <stdio.h>
#include <libelf.h>
#include <inttypes.h>

/**
 * \brief Input file types supported
 */
typedef enum _enum_file_type {
  ELF_FILE = 0, //!< ELF File
  BIN_FILE = 1, //!< Binary File
} file_type_t;

/**
 * \brief Object File Type Descriptor
 */
typedef struct _file_desc_str {
  char* name;       //!< File Name
  file_type_t type; //!< File Type
  union {
    FILE* fd;               //!< File Descriptor for raw binary file
    struct {
      Elf* elf;             //!< ELF Descriptor for ELF file
      Elf_Data* progdata;   //!< ELF_Data structure for the binary data
      uint8_t* progbyte;    //!< Pointer to current program byte
    };
  };
} file_desc_t;


/**
 * \brief Opens a binary object file for reading
 */
int obj_file_open(char* filename, file_desc_t* fdesc);


/**
 * \brief Closes a binary object file
 */
int obj_file_close(file_desc_t* fdesc);


/**
 * \brief Seek an address within an object file
 */
int obj_file_seek(file_desc_t* fdesc, uint32_t addr, int whence);


/**
 * \brief Read from an object file
 */
int obj_file_read(file_desc_t* fdesc, void* ptr, size_t size, size_t nmemb);

#endif//_FILEUTILS_H_
