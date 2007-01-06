/**
 * @file memmap_conf.h
 * @brief POSIX Processor architecture configuration for memory map
 * @author Ram Kumar {ram@ee.ucla.edu}
 */
// $Id: memmap_conf.h,v 1.3 2006/06/03 18:35:05 ram Exp $

#ifndef _MEMMAP_CONF_H_
#define _MEMMAP_CONF_H_
#ifdef SOS_SFI

//-----------------------------------------------------------------------
// PROCESSOR SPECIFIC CONSTANTS
//-----------------------------------------------------------------------
/**
 * POSIX architecture has a 16-bit address range
 * The total heap size on POSIX is 3072 bytes
 * The word size is 1 byte.
 * Heap Offset Address is represented as bit vector <A11:A0>
 * We use a block size of 16 bytes.
 * Block number is given by the bit vector <A11:A4>
 * Permission entry is 2 bits wide for every block.
 * There are 4 permission entries per byte in the memmap table.
 * Memmap Table Entry Index = <A11:A6>
 * Memmap Table Entry Offset = (<A5:A4> * 2)
 */

enum
  {
    MEMMAP_TABLE_BLOCK_SIZE     = 16,  // Bytes
    MEMMAP_TABLE_ENTRY_SIZE     = 2,   // Bits
    BLOCK_PERMS_PER_BYTE        = 4,   // Permission entries per byte
    BLOCK_NUMBER_LSB            = 4,   // log2(MEMMAP_TABLE_BLOCK_SIZE)
    MEMMAP_TABLE_SIZE           = 48,  //((MALLOC_HEAP_SIZE/MEMMAP_TABLE_BLOCK_SIZE))/BLOCK_PERMS_PER_BYTE
    MEMMAP_TABLE_BLOCK_MASK     = 0x0F, //~(0xFF << BLOCK_NUMBER_LSB)                                      
    MEMMAP_TABLE_INDEX_LSB      = 6, //MEMMAP_TABLE_ENTRY_SIZE + BLOCK_NUMBER_LSB
    MEMMAP_BYTE_BLK_OFFSET_MASK = 0x3F, //~(0xFF << MEMMAP_TABLE_INDEX_LSB) 
    BLOCK_PERMS_MASK            = 0x03, //~(0xFF << MEMMAP_TABLE_ENTRY_SIZE)
  };

extern uint8_t malloc_heap[MALLOC_HEAP_SIZE] SOS_HEAP_SECTION;

#define SOS_MEMMAP_SECTION

//-----------------------------------------------------------------------
// MEMMAP ACCESS MACROS
//-----------------------------------------------------------------------
#define MEMMAP_GET_BYTE_INDEX(addr) (uint16_t) (((uint16_t)addr - (uint16_t)malloc_heap) >> MEMMAP_TABLE_INDEX_LSB)

#define MEMMAP_GET_BYTE_AND_BLK_OFFSET(addr) (uint8_t)(((uint16_t)addr - (uint16_t)malloc_heap) & MEMMAP_BYTE_BLK_OFFSET_MASK)

#define MEMMAP_GET_BLOCK_NUM(addr)  (uint16_t)(((uint32_t)addr - (uint32_t)malloc_heap) >> BLOCK_NUMBER_LSB);



// Assume address to be in the heap
#define MEMMAP_GET_BYTE_INDEX_BYTE_OFFSETX2(addr, memmap_table_index, memmap_byte_offset) \
  {									\
    uint16_t __temp;							\
    __temp = (uint16_t)(((uint32_t)addr - (uint32_t)malloc_heap)>>3);	\
    memmap_byte_offset = __temp & 0x06;					\
    memmap_table_index = (__temp >> 3);					\
  }


#endif//SOS_SFI
#endif//_MEMMAP_CONF_H_
