/**
 * @file proc_memmap_checker.c
 * @brief Memmap checker routine for POSIX target
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#include <memmap.h>

void ker_memmap_perms_check(void* addr)
{
  uint16_t block_num;
  uint8_t block_perms;
  //uint8_t byte_offset_in_block;

  if ((uint16_t*)addr < &block_num){
    block_num = MEMMAP_GET_BLOCK_NUM(addr);
    MEMMAP_GET_PERMS(block_num, block_perms);
    if ((block_perms & BLOCK_TYPE_BM) != BLOCK_TYPE_MODULE){
      memmap_exception();
    }
    if (((block_perms & SEG_LATER_PART_MASK) != SEG_LATER_PART)&&
	(((uint8_t)((uint32_t)addr) & MEMMAP_TABLE_BLOCK_MASK) < 3)){
      memmap_exception();
    }
  }
} 

