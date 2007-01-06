/** -*-C-*- **/
/**
 * @file memmap.c
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @brief Memory Map Implementation for SOS Kernel (Currently POSIX and AVR targets only)
 */

#include <memmap.h>

//-----------------------------------------------------------------------------
// DEBUG MACROS
//-----------------------------------------------------------------------------
//#define SOS_DEBUG_MEMMAP
#ifndef SOS_DEBUG_MEMMAP
#undef DEBUG
#define DEBUG(...)
#undef DEBUG_PID
#define DEBUG_PID(...)
#define printMemMap(...) 
#else
#define DEBUG(arg...) printf(arg)
static void printMemMap(char*);
#endif

//----------------------------------------------------------
// GLOBAL VARIABLE
//----------------------------------------------------------
uint8_t memmap[MEMMAP_TABLE_SIZE] SOS_MEMMAP_SECTION;

//----------------------------------------------------------
// INITIALIZE MEMORY MAP
//----------------------------------------------------------
void memmap_init()
{
  uint16_t i;
  // Initialize all memory to be owned by the kernel
  for (i = 0; i < MEMMAP_TABLE_SIZE; i++){
    memmap[i] = BLOCK_FREE_VEC;
  }
  printMemMap("Memmap Init:");
}

//----------------------------------------------------------
// SET PERMISSIONS FOR A SET OF BLOCKS
//----------------------------------------------------------
void memmap_set_perms(void* baseaddr, uint16_t length, uint8_t seg_perms)
{
  blk_num_t blknum, blocksleft;
  uint16_t memmap_table_index;
  uint8_t memmap_byte_offset;
  uint8_t lperms_bm, lperms_set_val;
  uint8_t perms;

  if (0 == length) {
    DEBUG("Length cannot be 0\n");
    return;
  }              

  if ((length & MEMMAP_BLK_OFFSET) != 0){
    DEBUG("Length has to be a multiple of block size\n");
    return; // Length should be a multiple of block size
  }

  blocksleft = length >> MEMMAP_BLK_NUM_LSB;
  blknum = MEMMAP_GET_BLK_NUM(baseaddr);
  memmap_table_index = MEMMAP_GET_TABLE_NDX(blknum);
  memmap_byte_offset = MEMMAP_GET_BYTE_OFFSET(blknum);

  
  perms = memmap[memmap_table_index];

  lperms_bm = (MEMMAP_REC_MASK << memmap_byte_offset);
  lperms_set_val = (seg_perms << memmap_byte_offset);

  while (blocksleft > 0){
    perms &= ~(lperms_bm);
    perms |= lperms_set_val;
    blocksleft--;
    lperms_bm <<= MEMMAP_REC_BITS;
    lperms_set_val <<= MEMMAP_REC_BITS;
    if (0 == lperms_bm){
      memmap[memmap_table_index] = perms;
      memmap_table_index++;
      perms = memmap[memmap_table_index];
      lperms_bm = MEMMAP_REC_MASK;
      lperms_set_val = seg_perms;
    }
  }
  memmap[memmap_table_index] = perms;
  printMemMap("Memmap Set Perms:");
}

//----------------------------------------------------------
// CHANGE PERMISSIONS FOR CURRENT SEGMENT
//----------------------------------------------------------
uint16_t memmap_change_perms(void* baseaddr, uint8_t perm_mask, uint8_t perm_check_val, uint8_t perm_set_val)
{
  blk_num_t blknum, nblocks;
  uint16_t memmap_table_index;
  uint8_t memmap_byte_offset;
  uint8_t lperm_mask, lperm_check_val, lperm_set_val, perm_bm;
  uint8_t perms;

  nblocks = 0;
  blknum = MEMMAP_GET_BLK_NUM(baseaddr);
  memmap_table_index = MEMMAP_GET_TABLE_NDX(blknum);
  memmap_byte_offset = MEMMAP_GET_BYTE_OFFSET(blknum);

  perms = memmap[memmap_table_index];

  lperm_mask = (perm_mask << memmap_byte_offset);
  lperm_check_val = (perm_check_val << memmap_byte_offset);
  lperm_set_val = (perm_set_val << memmap_byte_offset);
  perm_bm = (MEMMAP_REC_MASK << memmap_byte_offset);

  while ((perms & lperm_mask) == lperm_check_val){
    perms &= ~perm_bm;
    perms |= lperm_set_val;
    nblocks++;

    lperm_mask <<= MEMMAP_REC_BITS;
    lperm_check_val <<= MEMMAP_REC_BITS;
    lperm_set_val <<= MEMMAP_REC_BITS;
    perm_bm <<= MEMMAP_REC_BITS;

    if (0 == perm_bm){
      memmap[memmap_table_index] = perms;
      memmap_table_index++;
      perms = memmap[memmap_table_index];
      lperm_mask = perm_mask;
      lperm_check_val = perm_check_val;
      lperm_set_val = perm_set_val;
      perm_bm = MEMMAP_REC_MASK;
    }
  }
  memmap[memmap_table_index] = perms;
  printMemMap("memmap_change_perms:");
  return nblocks;
}


#ifdef SOS_DEBUG_MEMMAP
static void printMemMap(char* s)
{
  uint16_t i;
  uint8_t p;
  printf("%s\n", s);
  for (i = 0; i < (MEMMAP_TABLE_SIZE * MEMMAP_REC_PER_BYTE); i++){
    MEMMAP_GET_PERMS(i, p);
    printf("%3d: ",i);

    if ((p & MEMMAP_SEG_MASK) == MEMMAP_SEG_START)
      printf("S");
    else
      printf(" ");
    printf("%d ", (p & MEMMAP_DOM_MASK));
    
    if (((i+1)%16) == 0) printf("\n");
  }
  printf("\n");
  return;
}
#endif


//----------------------------------------------------------
// OBTAIN THE NUMBER OF BLOCKS IN CURRENT SEGMENT
//----------------------------------------------------------
/*
uint16_t memmap_get_seg_size(void* baseaddr)
{
  uint16_t nblocks;
  uint16_t memmap_table_index;
  uint8_t  memmap_byte_offset;
  uint8_t perms;
  uint8_t lperms_bm;

  MEMMAP_GET_BYTE_INDEX_BYTE_OFFSETX2(baseaddr, memmap_table_index, memmap_byte_offset);
  perms = memmap[memmap_table_index];
  lperms_bm = (SEG_LATER_PART_MASK << memmap_byte_offset);
  nblocks = 0;
  
  while ((lperms_bm & perms) != 0){
    nblocks++;
    lperms_bm <<= MEMMAP_REC_BITS;
    if (0 == lperms_bm){
      memmap_table_index++;
      perms = memmap[memmap_table_index];
    }
  }
  return nblocks;
}
*/
