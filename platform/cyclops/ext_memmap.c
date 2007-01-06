/**
 * \file ext_memmap.c
 * \author Ram Kumar {ram@ee.ucla.edu}
 * \brief External Memory Map implementation for Cyclops platform
 */

#include  <ext_memmap.h>
#include  <malloc_extmem.h>

//----------------------------------------------------------
// LOCAL GLOBALS
//----------------------------------------------------------
//uint8_t* ext_memmap;
uint8_t ext_memmap[EXT_MEMMAP_TABLE_SIZE];

//----------------------------------------------------------
// INITIALIZE MEMORY MAP
//----------------------------------------------------------
void ext_memmap_init()
{
  uint8_t i;
  //ext_memmap = (uint8_t*)EXT_MEMMAP_TABLE_START;
  // Initialize all memory to be free and assigned to the kernel
  for (i = 0; i < EXT_MEMMAP_TABLE_SIZE; i++){
    ext_memmap[i] = EXT_BLOCK_FREE;
  }
  // Ram - There is an implicit assumption here about the memory layout
  ext_memmap_set_perms((void*)0, EXT_MEM_START, EXT_BLOCK_ALLOC_KERNEL);
  ext_memmap_set_perms((void*)(EXT_MEM_HEAP_END + 1), (EXT_MEM_END - EXT_MEM_HEAP_END), EXT_BLOCK_ALLOC_KERNEL);
}

//----------------------------------------------------------
// SET PERMISSIONS FOR A SET OF BLOCKS
//----------------------------------------------------------
void ext_memmap_set_perms(void* baseaddr, uint16_t length, uint8_t seg_perms)
{
  uint16_t blocksleft;
  uint16_t ext_memmap_table_index;
  uint8_t ext_memmap_byte_offset;
  uint8_t lperms_bm, lperms_set_val;
  uint8_t perms;

  if (0 == length) {
    DEBUG("Length cannot be 0\n");
    return;
  }              

  if ((length & EXT_MEMMAP_TABLE_BLOCK_MASK) != 0){
    DEBUG("Length has to be a multiple of block size\n");
    return; // Length should be a multiple of block size
  }

  blocksleft = length >> EXT_BLOCK_NUMBER_LSB;

  EXT_MEMMAP_GET_BYTE_INDEX_BYTE_OFFSETX2(baseaddr, ext_memmap_table_index, ext_memmap_byte_offset);

  perms = ext_memmap[ext_memmap_table_index];

  lperms_bm = (EXT_BLOCK_PERMS_MASK << ext_memmap_byte_offset);
  lperms_set_val = (seg_perms << ext_memmap_byte_offset);

  while (blocksleft > 0){
    perms &= ~(lperms_bm);
    perms |= lperms_set_val;
    blocksleft--;
    lperms_bm <<= EXT_MEMMAP_TABLE_ENTRY_SIZE;
    lperms_set_val <<= EXT_MEMMAP_TABLE_ENTRY_SIZE;
    if (0 == lperms_bm){
      ext_memmap[ext_memmap_table_index] = perms;
      ext_memmap_table_index++;
      perms = ext_memmap[ext_memmap_table_index];
      lperms_bm = EXT_BLOCK_PERMS_MASK;
      lperms_set_val = seg_perms;
    }
  }
  ext_memmap[ext_memmap_table_index] = perms;
}

