/******************************************************************************

  NAME:         DoubleLinkPool
  
  DESCRIPTION:  An implementation in C of a heap memory controller using a
                doubly-linked list to manage the free list. The basis for the 
                design is the DoublyLinkedPool C++ class of Bruno Preiss
                (http://www.bpreiss.com). This version provides for acquiring 
                an area in O(n) time, and releasing an area in O(1) time.
                
  LICENSE:      This program is free software. you can redistribute it and/or
                modify it under the terms of the GNU General Public License
                as published by the Free Software Foundation; either version 2
                of the License, or any later version. This program is 
                distributed in the hope that it will be useful, but WITHOUT 
                ANY WARRANTY; without even the implied warranty of 
                MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
                GNU General Public License for more details.

  REVISION:     12-6-2005 - version for AVR
                  
  AUTHOR:       Ron Kreymborg
                Jennaron Research                
                
******************************************************************************/
//-----------------------------------------------------------------------------
// INCLUDE
//-----------------------------------------------------------------------------
#include <malloc.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_sched.h>
#include <malloc_conf.h>
#include <string.h>
#include <sos_timer.h>
#include <stdlib.h>
#include <sos_logging.h>
// SFI Mode Includes
#ifdef SOS_SFI
#include <memmap.h>          // Memmory Map AOI
#include <sfi_jumptable.h>   // sfi_get_domain_id
#include <sfi_exception.h>   // sfi_exception
#include <cross_domain_cf.h> // curr_dom_id
#endif

//-----------------------------------------------------------------------------
// DEBUG
//-----------------------------------------------------------------------------
#define LED_DEBUG
#include <led_dbg.h>
//#define SOS_DEBUG_MALLOC
#ifndef SOS_DEBUG_MALLOC
#undef DEBUG
#define DEBUG(...)
#undef DEBUG_PID
#define DEBUG_PID(...)
#define printMem(...) 
#else
static void printMem(char*);
#endif

//-----------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------
#define RESERVED            0x8000          // must set the msb of BlockSizeType
#ifndef SOS_SFI
#define BLOCKOVERHEAD (sizeof(BlockHeaderType) + 1) // The extra byte is for the guard byte
#else
#define BLOCKOVERHEAD (sizeof(BlockHeaderType)) 
#endif

//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define TO_BLOCK_PTR(p)     (Block*)((BlockHeaderType*)p - 1)
#define BLOCKS_TO_BYTES(n)  (((n & ~RESERVED) << SHIFT_VALUE) - sizeof(BlockHeaderType))
#ifndef SOS_SFI
#define BLOCK_GUARD_BYTE(p) (*((uint8_t*)((uint8_t*)((Block*)p + (p->blockhdr.blocks & ~RESERVED)))-1))
#endif

//-----------------------------------------------------------------------------
// DATA STRUCTURES
//-----------------------------------------------------------------------------
// A memory block is made up of a header and either a user data part or a pair
// of pointers to the previous and next free area. A memory area is made up
// of one or more of these blocks. Only the first block will have the header.
// The header contains the number of blocks contained within the area.
//
typedef struct _BlockHeaderType
{
  uint16_t blocks;
  uint8_t  owner;
} //PACK_STRUCT
  BlockHeaderType;


typedef struct _Block
{
  BlockHeaderType blockhdr;
  union
  {
    uint8_t userPart[BLOCK_SIZE - sizeof(BlockHeaderType)];
    struct 
    {
      struct _Block *prev;
      struct _Block *next;
    };
  } /*PACK_STRUCT */;
} //PACK_STRUCT
  Block;

//-----------------------------------------------------------------------------
// LOCAL FUNCTIONS
//-----------------------------------------------------------------------------
//static int8_t mem_handler(void *state, Message *msg);
static void InsertAfter(Block*);
static void Unlink(Block*);
static Block* MergeBlocks(Block* block);
static Block* MergeBlocksQuick(Block *block, uint16_t req_blocks);
static void SplitBlock(Block* block, uint16_t reqBlocks);
//static void verify_memory( void );

//-----------------------------------------------------------------------------
// LOCAL GLOBAL VARIABLES
//-----------------------------------------------------------------------------
static Block*           mPool;
static Block*           mSentinel;
static uint16_t         mNumberOfBlocks;
static Block            malloc_heap[(MALLOC_HEAP_SIZE + (BLOCK_SIZE - 1))/BLOCK_SIZE] SOS_HEAP_SECTION;



#if 0
static sos_module_t malloc_module;
static mod_header_t mod_header SOS_MODULE_HEADER ={
  mod_id : KER_MEM_PID,
  state_size : 0,
  num_timers : 1,
  num_sub_func : 0,
  num_prov_func : 0,
  module_handler: mem_handler,
};
#endif


//-----------------------------------------------------------------------------
// Return a pointer to an area of memory that is at least the right size.
// SFI Mode: Allocate domain ID based upon requestor pid.
//-----------------------------------------------------------------------------
void* sos_blk_mem_longterm_alloc(uint16_t size, sos_pid_t id, bool bCallFromModule)
{
#if 0
  return sos_blk_mem_alloc(size, id, false);
#else
  HAS_CRITICAL_SECTION;
  uint16_t reqBlocks;
  Block* block;
  Block* max_block = NULL;
  Block* newBlock;
#ifdef SOS_SFI
  int8_t domid;
#endif

  if (size == 0) return NULL;
  printMem("malloc_longterm begin: ");
  reqBlocks = (size + BLOCKOVERHEAD + sizeof(Block) - 1) >> SHIFT_VALUE;
  ENTER_CRITICAL_SECTION();
  // First defragment the memory
  for (block = mSentinel->next; block != mSentinel; block = block->next)
    {
      block = MergeBlocks(block);
    }
	
  // Find the block that has largest address and larger than request
  for (block = mSentinel->next; block != mSentinel; block = block->next)
    {
      //block = MergeBlocks(block);
      if( (block > max_block) &&  (block->blockhdr.blocks >= reqBlocks) ) {
	max_block = block;
      }
    }

  if( max_block == NULL ) {
    printMem("Malloc Failed!!!: ");
    LEAVE_CRITICAL_SECTION();
    return NULL;
  }

  // Now take the tail of this block
  newBlock = max_block + (max_block->blockhdr.blocks - reqBlocks);

  if( newBlock == max_block ) {
    Unlink(newBlock);
  } else {
    // otherwise we just steal the tail by reducing the size
    max_block->blockhdr.blocks -= reqBlocks;
    newBlock->blockhdr.blocks = reqBlocks;
  }

  // Mark newBlock as reserved
  //
  newBlock->blockhdr.blocks |= RESERVED;
  newBlock->blockhdr.owner = id;

#ifdef SOS_SFI
  domid = sfi_get_domain_id(id);
  memmap_set_perms((void*) newBlock, sizeof(Block), DOM_SEG_START(domid));
  memmap_set_perms((void*) ((Block*)(newBlock + 1)), sizeof(Block) * (reqBlocks - 1), DOM_SEG_LATER(domid));
#else
  BLOCK_GUARD_BYTE(newBlock) = id;
#endif

  printMem("malloc_longterm end: ");
  //DEBUG("malloc_longterm return %x\n", (unsigned int)newBlock);
  LEAVE_CRITICAL_SECTION();
  ker_log( SOS_LOG_MALLOC, id, reqBlocks );
  return newBlock->userPart;
#endif
}

//-----------------------------------------------------------------------------
// Malloc Function
// SFI Mode: Allocate domain ID based upon requestor pid
//-----------------------------------------------------------------------------
void* sos_blk_mem_alloc(uint16_t size, sos_pid_t id, bool bCallFromModule)
{
  HAS_CRITICAL_SECTION;
  uint16_t reqBlocks;
  Block* block;
#ifdef SOS_SFI
  int8_t domid;
#endif

  // Check for errors.
  if (size == 0) return NULL;

  // Compute the number of blocks to satisfy the request.
  reqBlocks = (size + BLOCKOVERHEAD + sizeof(Block) - 1) >> SHIFT_VALUE;

  //DEBUG("sizeof(BlockHeaderType) = %d, sizeof(block) = %d\n", sizeof(BlockHeaderType), sizeof(Block));
  //DEBUG("req size = %d, reqBlocks = %d\n", size, reqBlocks);
    
  ENTER_CRITICAL_SECTION();
  //verify_memory();
  // Traverse the free list looking for the first block that will fit the
  // request. This is a "first-fit" strategy.
  //
  //printMem("malloc_start: ");
  for (block = mSentinel->next; block != mSentinel; block = block->next)
    {
      block = MergeBlocksQuick(block, reqBlocks);
      // Is this free area (which could have just expanded somewhat)
      // large enough to satisfy the request.
      //
      if (block->blockhdr.blocks >= reqBlocks)
        {
	  break;
        }
    }

  // If we are pointing at the sentinel then all blocks are allocated.
  //
  if (block == mSentinel)
    {
      printMem("Malloc Failed!!!: ");
      LEAVE_CRITICAL_SECTION();
      return NULL;
    }

  // If this free area is larger than required, it is split in two. The
  // size of the first area is set to that required and the second area
  // to the blocks remaining. The second area is then inserted into 
  // the free list.
  //
  if (block->blockhdr.blocks > reqBlocks)
    {
      SplitBlock(block, reqBlocks);
    }

  // Unlink the now correctly sized area from the free list and mark it 
  // as reserved.
  //
  Unlink(block);
  block->blockhdr.blocks |= RESERVED;
  block->blockhdr.owner = id;

#ifdef SOS_SFI
  domid = sfi_get_domain_id(id);
  memmap_set_perms((void*) block, sizeof(Block), DOM_SEG_START(domid));
  memmap_set_perms((void*) ((Block*)(block + 1)), sizeof(Block) * (reqBlocks - 1), DOM_SEG_LATER(domid));
#else                           
  BLOCK_GUARD_BYTE(block) = id; 
#endif

  //printMem("malloc_end: ");
  LEAVE_CRITICAL_SECTION();

  ker_log( SOS_LOG_MALLOC, id, reqBlocks );
  return block->userPart;
}

//-----------------------------------------------------------------------------
// Return this memory block to the free list.
// SFI Mode: 1. If call comes from un-trusted domain, then free only if current domain is owner
//           2. Block being freed should be start of segment
//-----------------------------------------------------------------------------
void sos_blk_mem_free(void* pntr, bool bCallFromModule)
{
	uint16_t freed_blocks;
	sos_pid_t owner;
#ifdef SOS_SFI
  uint16_t block_num;
  uint8_t perms;
#endif
  HAS_CRITICAL_SECTION;
  // Check for errors.
  //
  Block* top;
  Block* baseArea;   // convert to a block address

  if( pntr == NULL ) {
    return;
  }
  top = mPool + mNumberOfBlocks;
  baseArea = TO_BLOCK_PTR(pntr);   // convert to a block address

  if ( (baseArea < malloc_heap) || (baseArea >= (malloc_heap + mNumberOfBlocks)) ) {
    DEBUG("sos_blk_mem_free: try to free invalid memory\n");
    DEBUG("possible owner %d %d\n", baseArea->blockhdr.owner, BLOCK_GUARD_BYTE(baseArea));
    return;
  }
  
#ifdef SOS_SFI
  ENTER_CRITICAL_SECTION();
  // Get the permission of the first block
  block_num = MEMMAP_GET_BLK_NUM(baseArea);
  MEMMAP_GET_PERMS(block_num, perms);
  // Check - Not a start of segment
  if ((perms & MEMMAP_SEG_MASK) == MEMMAP_SEG_LATER) {
    LEAVE_CRITICAL_SECTION();
    sfi_exception(MALLOC_EXCEPTION);
    return; 
  }
  // Check - Untrusted domain trying to free memory that it does not own or that is free.
#ifdef SFI_DOMS_8
  if ((bCallFromModule) && ((perms & MEMMAP_DOM_MASK) != curr_dom_id))
#endif
#ifdef SFI_DOMS_2
  if ((bCallFromModule) && ((perms & MEMMAP_DOM_MASK) == KER_DOM_ID))
#endif
    {
      LEAVE_CRITICAL_SECTION();
      sfi_exception(MALLOC_EXCEPTION);
      return; 
    }
#else
  // Check for memory corruption
  if (baseArea->blockhdr.owner != BLOCK_GUARD_BYTE(baseArea)){
    DEBUG("sos_blk_mem_free: detect memory corruption\n");
    DEBUG("possible owner %d %d\n", baseArea->blockhdr.owner, BLOCK_GUARD_BYTE(baseArea));
    return;
  }
#endif
  
  // Very simple - we insert the area to be freed at the start
  // of the free list. This runs in constant time. Since the free
  // list is not kept sorted, there is less of a tendency for small
  // areas to accumulate at the head of the free list.
  //
  ENTER_CRITICAL_SECTION();
  owner = baseArea->blockhdr.owner;
  baseArea->blockhdr.blocks &= ~RESERVED;
  baseArea->blockhdr.owner = NULL_PID;
  freed_blocks = baseArea->blockhdr.blocks;
  
#ifdef SOS_SFI
  MEMMAP_SET_PERMS(block_num, BLOCK_FREE);
  memmap_change_perms((void*)((Block*)(baseArea + 1)), 
		      MEMMAP_SEG_MASK|MEMMAP_DOM_MASK, 
		      DOM_SEG_LATER(perms), 
		      BLOCK_FREE);
#endif
  InsertAfter(baseArea);
  //printMem("free_end: ");
  LEAVE_CRITICAL_SECTION();
  ker_log( SOS_LOG_FREE, owner, freed_blocks );
  return;
}

//-----------------------------------------------------------------------------
// Change ownership of memory
// SFI Mode: 1. If call from un-trusted domain. change permissions only if current domain is block owner
//           2. If call from trusted domain, everything is fair !!
//-----------------------------------------------------------------------------
int8_t sos_blk_mem_change_own(void* ptr, sos_pid_t id, bool bCallFromModule) 
{
#ifdef SOS_SFI
  HAS_CRITICAL_SECTION;
  uint8_t perms;
  uint16_t block_num;
  int8_t domid;
#endif

  Block* blockptr = TO_BLOCK_PTR(ptr); // Convert to a block address         
  sos_pid_t old_owner;
  // Check for errors                                          
  if (NULL_PID == id || NULL == ptr) return -EINVAL;           

#ifdef SOS_SFI
  ENTER_CRITICAL_SECTION();

  //Get the permission for the first block
  block_num = MEMMAP_GET_BLK_NUM(blockptr);
  MEMMAP_GET_PERMS(block_num, perms);

  // Check - Not a start of segment
  if ((perms & MEMMAP_SEG_MASK) == MEMMAP_SEG_LATER) {
    LEAVE_CRITICAL_SECTION();
    sfi_exception(MALLOC_EXCEPTION);
  }

#ifdef SFI_DOMS_8 
  if ((bCallFromModule && ((perms & MEMMAP_DOM_MASK) == curr_dom_id))|| (!bCallFromModule))
#endif
#ifdef SFI_DOMS_2
  if ((bCallFromModule && ((perms & MEMMAP_DOM_MASK) != KER_DOM_ID)) || (!bCallFromModule))
#endif
    {
      // Call has come from trusted domain OR
      // Call has come from block owner
      
      // Get domain id of new owner
      domid = sfi_get_domain_id(id);
      if (domid < 0){
	LEAVE_CRITICAL_SECTION();
	sfi_exception(SFI_DOMAINID_EXCEPTION);
      }
      
      // Change Permissions Only if it is required
      if (domid != (perms & MEMMAP_DOM_MASK)){
	MEMMAP_SET_PERMS(block_num, DOM_SEG_START(domid));
	memmap_change_perms((void*)((Block*)(blockptr + 1)),
			    MEMMAP_SEG_MASK | MEMMAP_DOM_MASK,
			    DOM_SEG_LATER(perms),
			    DOM_SEG_LATER(domid));
      }
      LEAVE_CRITICAL_SECTION();
    }
  else{
    // Non-owner trying to change ownership of the block
    DEBUG("Current domain is not owner of this block.\n");
    LEAVE_CRITICAL_SECTION();
    sfi_exception(MALLOC_EXCEPTION);
    return -EINVAL;
  }

#else
  // Check for memory corruption                               
  if (blockptr->blockhdr.owner != BLOCK_GUARD_BYTE(blockptr))  {
    DEBUG("sos_blk_mem_change_own: detect memory corruption\n");
    DEBUG("possible owner %d %d\n", blockptr->blockhdr.owner, BLOCK_GUARD_BYTE(blockptr));
    return -EINVAL;
  }
  BLOCK_GUARD_BYTE(blockptr) = id; 
#endif
  old_owner =  blockptr->blockhdr.owner;
  // Set the new block ID                                      
  blockptr->blockhdr.owner = id;        
  //ker_log( SOS_LOG_CHANGE_OWN, id, old_owner);  
  return SOS_OK;
}

void mem_start() 
{
  // sched_register_kernel_module(&malloc_module, sos_get_header_address(mod_header), NULL);
}

int8_t mem_remove_all(sos_pid_t id)
{
  HAS_CRITICAL_SECTION;
  Block* block = (Block*)malloc_heap;

  ENTER_CRITICAL_SECTION();
  for (block = (Block*)malloc_heap; 
       block != mSentinel; 
       block += block->blockhdr.blocks & ~RESERVED) 
    {
      if ((block->blockhdr.blocks & RESERVED) && 
	  (block->blockhdr.owner == id)){
	ker_free(block->userPart);
      }		
    }
  //printMem("remove_all_end: ");
  LEAVE_CRITICAL_SECTION();
  return SOS_OK;
}


//-----------------------------------------------------------------------------
// Re-allocate the buffer to a new area the requested size. If possible the
// existing area is simply expanded. Otherwise a new area is allocated and
// the current contents copied.
// SFI Mode: 1. If call from untrusted domain, only owner is allowed to realloc
//-----------------------------------------------------------------------------
void* sos_blk_mem_realloc(void* pntr, uint16_t newSize, bool bCallFromModule)
{
  HAS_CRITICAL_SECTION;
  sos_pid_t id;
#ifdef SOS_SFI
  uint16_t block_num;
  uint8_t perms;  
  uint16_t oldSize;
  int8_t domid;
#endif
  // Check for errors.
  //
  if ( (pntr == NULL ) || (newSize == 0) ) return pntr;

#ifdef SOS_SFI
  ENTER_CRITICAL_SECTION();
  // Get the permission of the first block
  block_num = MEMMAP_GET_BLK_NUM(pntr);
  MEMMAP_GET_PERMS(block_num, perms);
#ifdef SFI_DOMS_8
  if ((bCallFromModule) && ((perms & MEMMAP_DOM_MASK) != curr_dom_id))
#endif
#ifdef SFI_DOMS_2
  if ((bCallFromModule) && ((perms & MEMMAP_DOM_MASK) == KER_DOM_ID))
#endif
    {
      LEAVE_CRITICAL_SECTION();
      sfi_exception(MALLOC_EXCEPTION);
      // Error - Untrusted domain trying to realloc memory that it does not own or that is free.
    }
  domid = perms & MEMMAP_DOM_MASK;
  LEAVE_CRITICAL_SECTION();
#endif


  Block* block = TO_BLOCK_PTR(pntr);   // convert user to block address
  uint16_t reqBlocks = (newSize + BLOCKOVERHEAD + sizeof(Block) - 1) >> SHIFT_VALUE;

  ENTER_CRITICAL_SECTION();
  id = block->blockhdr.owner;
  block->blockhdr.blocks &= ~RESERVED;         // expose the size
#ifdef SOS_SFI
  oldSize = BLOCKS_TO_BYTES(block->blockhdr.blocks);
#endif

  // The fastest option is to merge this block with any free blocks
  // that are contiguous with this block.
  //
  block = MergeBlocks(block);

  if (block->blockhdr.blocks > reqBlocks)
    {
      // The merge produced a larger block than required, so split it
      // into two blocks. This also takes care of the case where the
      // new size is less than the old.
      //
      SplitBlock(block, reqBlocks);
#ifdef SOS_SFI
      if (reqBlocks < oldSize){
	memmap_set_perms((Block*)(block + reqBlocks), 
			 (oldSize - reqBlocks)*(sizeof(Block)), 
			 MEMMAP_SEG_START|BLOCK_FREE);
      }
      else{
	memmap_set_perms((Block*)(block + oldSize),
			   (reqBlocks - oldSize)*(sizeof(Block)), 
			   DOM_SEG_LATER(domid));
      }
#endif
      
    }
  else if (block->blockhdr.blocks < reqBlocks)
    {
      // Could not expand this block. Must attempt to allocate
      // a new one the correct size and copy the current contents.
      //
#ifndef SOS_SFI
      uint16_t oldSize = BLOCKS_TO_BYTES(block->blockhdr.blocks);
#endif
      block = (Block*)ker_malloc(newSize, id);
      if (NULL != block)
        {
	  // A new block large enough has been allocated. Copy the
	  // existing data and then discard the old block.
	  //
	  block = TO_BLOCK_PTR(block);
	  memcpy(block->userPart, (Block*)(TO_BLOCK_PTR(pntr))->userPart, oldSize);
	  sos_blk_mem_free(pntr, bCallFromModule);
        }
      else
        {
	  // Cannot re-allocate this block. Note the old pointer
	  // is still valid.
	  //
	  LEAVE_CRITICAL_SECTION();
	  return NULL;        // no valid options
        }
    }
#ifdef SOS_SFI
  else if (block->blockhdr.blocks == reqBlocks)
    {
      memmap_set_perms((Block*)(block + oldSize), 
		       (reqBlocks - oldSize)*(sizeof(Block)), 
		       DOM_SEG_LATER(domid));
    }
#endif

  block->blockhdr.blocks |= RESERVED;
  block->blockhdr.owner = id;
#ifndef SOS_SFI                                
  BLOCK_GUARD_BYTE(block) = id; 
#endif
  LEAVE_CRITICAL_SECTION();
  return block->userPart;
}
//-----------------------------------------------------------------------------
// Compute the number of blocks that will fit in the memory area defined.
// Allocate the pool of blocks. Note this includes the sentinel area that is 
// attached to the end and is always only one block. The first entry in the 
// free list pool is set to include all available blocks. The sentinel is 
// initialised to point back to the start of the pool.
//
void mem_init(void)
{
  Block* head;
  char* heapStart = (char*)malloc_heap; //&__heap_start;
  char* heapEnd = (char*)(((char*)malloc_heap) + MALLOC_HEAP_SIZE);//&__heap_end;

  DEBUG("malloc init\n");
  mPool = (Block*)heapStart;
  mNumberOfBlocks = (uint16_t)(((heapEnd - (char*)mPool) >> SHIFT_VALUE) - 1L);
  mSentinel = mPool + mNumberOfBlocks;

  mSentinel->blockhdr.blocks = RESERVED;           // now cannot be used
  mSentinel->prev = mSentinel;
  mSentinel->next = mSentinel;

  // Entire pool is initially a single unallocated area.
  //
  head = &mPool[0];
  head->blockhdr.blocks = mNumberOfBlocks;         // initially all of free memeory
  InsertAfter(head);                      // link the sentinel

#ifdef SOS_SFI
  memmap_init(); // Initialize all the memory to be owned by the kernel
  memmap_set_perms((void*) mPool, mNumberOfBlocks * sizeof(Block), MEMMAP_SEG_START|BLOCK_FREE); // Init heap to unallocated
#endif

}

//-----------------------------------------------------------------------------
// As each area is examined for a fit, we also examine the following area. 
// If it is free then it must also be on the Free list. Being a doubly-linked 
// list, we can combine these two areas in constant time. If an area is 
// combined, the procedure then looks again at the following area, thus 
// repeatedly combining areas until a reserved area is found. In terminal 
// cases this will be the sentinel block.
//
static Block* MergeBlocks(Block* block)
{
  while (TRUE)
    {
      Block* successor = block + block->blockhdr.blocks;   // point to next area
      /*
	DEBUG("block = %x, blocks = %d, successor = %x, alloc = %d\n",
	(unsigned int)block,
	block->blockhdr.blocks,
	(unsigned int) successor,
	successor->blockhdr.blocks);
      */
      if (successor->blockhdr.blocks & RESERVED)           // done if reserved
        {
	  return block;
        }
      Unlink(successor);
      block->blockhdr.blocks += successor->blockhdr.blocks;         // add in its blocks
    }
}

static Block* MergeBlocksQuick(Block *block, uint16_t req_blocks)
{
  while (TRUE)
    {
      Block* successor = block + block->blockhdr.blocks;   // point to next area
      if (successor->blockhdr.blocks & RESERVED)           // done if reserved
	{
	  return block;
	}
      Unlink(successor);
      block->blockhdr.blocks += successor->blockhdr.blocks;         // add in its blocks
      if( block->blockhdr.blocks >= req_blocks ) {
	return block;
      }
    }
}


//-----------------------------------------------------------------------------
//
static void SplitBlock(Block* block, uint16_t reqBlocks)  
{
  Block* newBlock = block + reqBlocks;            // create a remainder area
  newBlock->blockhdr.blocks = block->blockhdr.blocks - reqBlocks;   // set its size and mark as free
  block->blockhdr.blocks = reqBlocks;                      // set us to requested size
  InsertAfter(newBlock);                          // stitch remainder into free list
}
    
//-----------------------------------------------------------------------------
//
static void InsertAfter(Block* block)
{
  Block* p = mSentinel->next;
  mSentinel->next = block;
  block->prev = mSentinel;
  block->next = p;
  p->prev = block;
}

//-----------------------------------------------------------------------------
//
static void Unlink(Block* block)
{
  block->prev->next = block->next;
  block->next->prev = block->prev;
}

#if 0
static inline void mem_defrag()
{
  HAS_CRITICAL_SECTION;
  Block* block;
  ENTER_CRITICAL_SECTION();
  printMem("before defrag\n");
  for (block = mSentinel->next; block != mSentinel; block = block->next)
    {
      block = MergeBlocks(block);
    }
  printMem("after defrag\n");
  LEAVE_CRITICAL_SECTION();
}

static int8_t mem_handler(void *state, Message *msg)
{
  switch(msg->type){
  case MSG_TIMER_TIMEOUT:
    {
      mem_defrag();
      break;
    }
  case MSG_INIT:
    {
      ker_timer_init(KER_MEM_PID, 0, TIMER_REPEAT);
      ker_timer_start(KER_MEM_PID, 0, 10 * 1024L);
      break;
    }
  case MSG_DEBUG:
    {
      break;
    }
  default:
    return -EINVAL;
  }
  return SOS_OK;
}
#endif /* #if 0 */

#if 0
static void verify_memory( void )
{
  Block* block;
  block = (Block*)malloc_heap;
  Block* next_block;
  while(block != mSentinel) {
    next_block = block + (block->blockhdr.blocks & ~RESERVED);
    if( block->blockhdr.blocks & RESERVED ) {
      if( block->blockhdr.owner != BLOCK_GUARD_BYTE(block) ) {
	ker_led(LED_RED_TOGGLE);
	return;
      }
    }
    if( next_block != mSentinel) {
      if( (next_block->blockhdr.blocks & ~RESERVED) > ((MALLOC_HEAP_SIZE + (BLOCK_SIZE - 1))/BLOCK_SIZE) ) {
	ker_led(LED_GREEN_TOGGLE);
	ker_led(LED_RED_TOGGLE);
	return;
      }	
    }
    block = next_block;
  }

}
#endif

#ifdef SOS_DEBUG_MALLOC
#ifndef SOS_SFI
static void printMem(char* s)
{
  Block* block;
  int i = 0;

  DEBUG("%s\n", s);
  for (block = mSentinel->next; block != mSentinel; block = block->next)
    {
      /*
	if(block->blockhdr.owner != BLOCK_GUARD_BYTE(block)) {
	DEBUG("detect memory corruption in PrintMem\n");
	DEBUG("possible owner %d %d\n", block->blockhdr.owner, BLOCK_GUARD_BYTE(block));
	} else {
      */
      DEBUG("block %d : block: %x, prev : %x, next : %x, blocks : %d\n", i,
	    (unsigned int) block, 
	    (unsigned int) block->prev, 
	    (unsigned int) block->next, 
	    (unsigned int) block->blockhdr.blocks);	
      //}
      i++;
    }
  DEBUG("Memory Map:\n");
  block = (Block*)malloc_heap;
  i = 0;
  while(block != mSentinel) {
    DEBUG("block %d : addr: %x size: %d alloc: %d owner: %d check %d\n", i++, 
	  (unsigned int) block, 
	  (unsigned int) (block->blockhdr.blocks & ~RESERVED), 
	  (unsigned int) (block->blockhdr.blocks & RESERVED), 
	  (unsigned int) block->blockhdr.owner,
	  (unsigned int) BLOCK_GUARD_BYTE(block));
    block += block->blockhdr.blocks & ~RESERVED;
  }

}
#else
static void printMem(char* s)
{
  Block* block;
  int i = 0;

  DEBUG("%s\n", s);
  for (block = mSentinel->next; block != mSentinel; block = block->next){
    DEBUG("Block %d : Addr: %x, Prev : %x, Next : %x, Blocks : %d\n", i++, (uint32_t)block, (uint32_t)block->prev, (uint32_t)block->next, block->blockhdr.blocks);	
  }
  DEBUG("Memory Map:\n");
  block = (Block*)malloc_heap;
  i = 0;
  while(block != mSentinel) {
    DEBUG("block %d : Addr: %x size: %d alloc: %d owner: %d\n", i++, (uint32_t)block, block->blockhdr.blocks & ~RESERVED, block->blockhdr.blocks & RESERVED, block->blockhdr.owner);
    block += block->blockhdr.blocks & ~RESERVED;
  }
}
#endif // SOS_SFI
#endif

/**
 * Use by SYS API to notify module's panic
 */
int8_t ker_mod_panic(sos_pid_t pid)
{	
  return ker_panic();
}

/**
 * Used by the kernel to notify kernel component panic
 */
int8_t ker_panic(void)
{
  uint16_t val;
  LED_DBG(LED_RED_ON);
  LED_DBG(LED_GREEN_ON);
  LED_DBG(LED_YELLOW_ON);
  val = 0xffff;
  while (1){
#ifndef DISABLE_WDT
    watchdog_reset();
#endif
    if (val == 0){
      LED_DBG(LED_RED_TOGGLE);
      LED_DBG(LED_GREEN_TOGGLE);
      LED_DBG(LED_YELLOW_TOGGLE);
#ifdef SOS_SIM
      DEBUG("Malloc_Exception");
#endif
    }
    val--;
  }
  return -EINVAL;	
}

void* ker_sys_malloc(uint16_t size)
{    
  sos_pid_t my_id = ker_get_current_pid();    
  void *ret = sos_blk_mem_alloc(size, my_id, true);    
  if( ret != NULL ) {        
    return ret;    
  }    
  ker_mod_panic(my_id);    
  return NULL;
}

void* ker_sys_realloc(void* pntr, uint16_t newSize)
{
  void *ret = sos_blk_mem_realloc(pntr, newSize, true);
  if( ret != NULL ) {
    return ret;
  }
  ker_mod_panic(ker_get_current_pid());
  return NULL;
}

void ker_sys_free(void *pntr) 
{
  sos_blk_mem_free(pntr, true);
}	

int8_t ker_sys_change_own( void* ptr )
{
  sos_pid_t my_id = ker_get_current_pid();    
  if( SOS_OK != sos_blk_mem_change_own( ptr, my_id, true ) ) {
	ker_mod_panic(my_id);
  }
  return SOS_OK;
}


