/** -*-C-*- **/
/**
 * @file memmap_checker_proc.c
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @brief Run-time checker for the AVR architecture
 */
#include <hardware.h>
#include <memmap.h>
#include <memmap_checker_const.h>

void ker_memmap_perms_check(void* x){
  HAS_CRITICAL_SECTION;
  uint16_t memmap_index;
  uint8_t perms_lut_offset;
  uint8_t perms_bm;
  uint8_t block_perms;
  uint16_t inputaddr;
  uint8_t* writeaddr;
  ENTER_CRITICAL_SECTION();
  if ((uint16_t*)x < &memmap_index){
    inputaddr = (uint16_t)x;
    perms_lut_offset = (uint8_t)(inputaddr & 0x001f);
    memmap_index = (inputaddr >> 5);
    block_perms = memmap[memmap_index];
    perms_bm = pgm_read_byte(&MEMMAP_PERMS_BM_LUT[perms_lut_offset]);
    if ((perms_bm & block_perms) == 0){
      LEAVE_CRITICAL_SECTION();
      memmap_exception(); 
    }
    perms_bm = pgm_read_byte(&BLK_HDR_CHECK_BM_LUT[perms_lut_offset]);
    if (((!block_perms) & perms_bm) != 0){
      LEAVE_CRITICAL_SECTION();
      memmap_exception(); 
    }
  }
  writeaddr = (uint8_t*)x;
  *writeaddr = safe_store_addr[0];
  LEAVE_CRITICAL_SECTION();
  return;
}                          


