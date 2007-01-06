/**
 * \file ext_memmap.h
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \brief External Memory Map Implementation for Cyclops platform
 */

#ifndef _EXT_MEMMAP_H_
#define _EXT_MEMMAP_H_

#include <malloc_extmem.h>

#define EXT_MEMMAP_TABLE_SIZE  64     // Number of bytes
extern uint8_t ext_memmap[EXT_MEMMAP_TABLE_SIZE];

//-----------------------------------------------------------------------
// PERMISSION CONSTANTS
//-----------------------------------------------------------------------
enum 
  {
    EXT_BLOCK_FREE                  = 0x00,
    EXT_BLOCK_ALLOC                 = 0x01,
    EXT_BLOCK_KERNEL                = 0x00,
    EXT_BLOCK_MODULE                = 0x02,
    EXT_BLOCK_TYPE_BM               = 0x02,
    EXT_BLOCK_FREE_BM               = 0x01,
    EXT_BLOCK_ALLOC_MODULE          = 0x03,
    EXT_BLOCK_ALLOC_KERNEL          = 0x01,
  };


enum
  {
    EXT_MEMMAP_TABLE_BLOCK_SIZE     =    256, // Bytes
    EXT_MEMMAP_TABLE_ENTRY_SIZE     =      2, // Bits
    EXT_BLOCK_PERMS_PER_BYTE        =      4, // Permission entries per byte
    EXT_BLOCK_NUMBER_LSB            =      8, // log2(EXT_MEMMAP_TABLE_BLOCK_SIZE)
    EXT_MEMMAP_TABLE_BLOCK_MASK     = 0x00FF, // ~(0xFf << EXT_BLOCK_NUMBER_LSB)
    EXT_MEMMAP_TABLE_INDEX_LSB      =     10, // EXT_MEMMAP_TABLE_ENTRY_SIZE + EXT_BLOCK_NUMBER_LSB
    EXT_MEMMAP_BYTE_BLK_OFFSET_MASK = 0x03FF, // ~(0xFF << MEMMAP_TABLE_INDEX_LSB)
    EXT_BLOCK_PERMS_MASK            =   0x03, // ~(0xFF << MEMMAP_TABLE_ENTRY_SIZE)
  };

#define EXT_MEMMAP_GET_BLOCK_NUM(addr)    (uint8_t)((uint16_t)addr >> EXT_BLOCK_NUMBER_LSB)


#define EXT_MEMMAP_GET_PERMS(block_num, perms)				\
  {									\
    uint8_t __ext_memmap_table_index;					\
    uint8_t __ext_memmap_byte_offset;					\
    __ext_memmap_table_index = (block_num >> 2);			\
    __ext_memmap_byte_offset = ((block_num & 0x03) << 1);		\
    perms = ext_memmap[__ext_memmap_table_index];			\
    perms >>= __ext_memmap_byte_offset;					\
    perms &= 0x03;							\
  }

#define EXT_MEMMAP_GET_BYTE_INDEX_BYTE_OFFSETX2(addr, memmap_table_index, memmap_byte_offset) { \
    uint8_t __temp;							\
    __temp = (uint8_t)((uint16_t)addr >> 8);				\
    memmap_byte_offset = ((__temp & 0x03) << 1);			\
    memmap_table_index = (__temp >> 2);					\
  }

void ext_memmap_init();
void ext_memmap_set_perms(void* baseaddr, uint16_t length, uint8_t seg_perms);



#endif//_EXT_MEMMAP_H_
