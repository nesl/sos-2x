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
#include <message_queue.h>
#include <hardware.h>

#if defined (SOS_UART_CHANNEL)
#include <sos_uart.h>
#include <sos_uart_mgr.h>
#endif

#if defined (SOS_I2C_CHANNEL)                                
#include <sos_i2c.h>                                         
#include <sos_i2c_mgr.h>                                     
#endif    

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
//#define SOS_PROFILE_FRAGMENTATION
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

#ifdef SOS_DEBUG_GC
#define DEBUG_GC(arg...)  printf(arg)
#else
#define DEBUG_GC(...)
#endif

//-----------------------------------------------------------------------------
// CONSTANTS
//-----------------------------------------------------------------------------
#define MEM_GC_PERIOD       (10 * 1024L)
#define MEM_MOD_GC_STACK_SIZE    16
#define RESERVED            0x8000          // must set the msb of BlockSizeType
#define GC_MARK             0x4000
#define MEM_MASK            (RESERVED | GC_MARK)
#ifndef SOS_SFI
#define BLOCKOVERHEAD (sizeof(BlockHeaderType) + 1) // The extra byte is for the guard byte
#else
#define BLOCKOVERHEAD (sizeof(BlockHeaderType)) 
#endif

//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define TO_BLOCK_PTR(p)     (Block*)((BlockHeaderType*)p - 1)
#define BLOCKS_TO_BYTES(n)  (((n & ~MEM_MASK) << SHIFT_VALUE) - sizeof(BlockHeaderType))
#ifndef SOS_SFI
#define BLOCK_GUARD_BYTE(p) (*((uint8_t*)((uint8_t*)((Block*)p + (p->blockhdr.blocks & ~MEM_MASK)))-1))
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
} BlockHeaderType;


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
  };
} Block;

//-----------------------------------------------------------------------------
// LOCAL FUNCTIONS
//-----------------------------------------------------------------------------
static void InsertAfter(Block*);
static void Unlink(Block*);
static Block* MergeBlocks(Block* block);
static Block* MergeBlocksQuick(Block *block, uint16_t req_blocks);
static void SplitBlock(Block* block, uint16_t reqBlocks);

//-----------------------------------------------------------------------------
// LOCAL VARIABLES
//-----------------------------------------------------------------------------
#ifdef SOS_PROFILE_FRAGMENTATION
typedef struct malloc_frag_t {
	uint16_t malloc_max_efrag;  // maximum external fragmentation in blocks
	uint32_t malloc_efrag;       // total external fragmentation
	uint32_t malloc_efrag_cnt;  // number of external fragmentation counts

	uint32_t  malloc_ifrag;       // total internal fragmentation
	uint32_t malloc_alloc_cnt;  // number of allocations used for 
                                    // computing average
} PACK_STRUCT 
malloc_frag_t;

static malloc_frag_t mf;
static void malloc_record_efrag(Block *b);
static void malloc_record_ifrag(Block *b, uint16_t size);
#endif

#define NUM_HEAP_BLOCKS  ((MALLOC_HEAP_SIZE + (BLOCK_SIZE - 1))/BLOCK_SIZE)
static Block*           mPool;
static Block*           mSentinel;
static uint16_t         mNumberOfBlocks;
static Block            malloc_heap[NUM_HEAP_BLOCKS] SOS_HEAP_SECTION;


#ifdef SOS_USE_GC
static int8_t mem_handler(void *state, Message *msg);
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

#ifdef AVRORA_PLATFORM
//static uint8_t avrora_buf[100];

#endif

//-----------------------------------------------------------------------------
// Return a pointer to an area of memory that is at least the right size.
// SFI Mode: Allocate domain ID based upon requestor pid.
//-----------------------------------------------------------------------------
void* sos_blk_mem_longterm_alloc(uint16_t size, sos_pid_t id, bool bCallFromModule)
{
	HAS_CRITICAL_SECTION;
	uint16_t reqBlocks;
	Block* block;
	Block* max_block = NULL;
	Block* newBlock;
#ifdef SOS_SFI
	int8_t domid;
#endif

	if (size == 0) { return NULL; }

	printMem("malloc_longterm begin: ");
	reqBlocks = (size + BLOCKOVERHEAD + sizeof(Block) - 1) >> SHIFT_VALUE;
	ENTER_CRITICAL_SECTION();
	// First defragment the memory
	for (block = mSentinel->next; block != mSentinel; block = block->next) {
		block = MergeBlocks(block);
	}

	// Find the block that has largest address and larger than request
	for (block = mSentinel->next; block != mSentinel; block = block->next) {
		if( (block > max_block) &&  (block->blockhdr.blocks >= reqBlocks) ) {
			max_block = block;
		}
	}

#ifdef SOS_PROFILE_FRAGMENTATION
	// Record external fragmentation
	for (block = max_block->next; block != mSentinel; block = block->next) {
		malloc_record_efrag( block );
	}	
#endif

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

#ifdef SOS_PROFILE_FRAGMENTATION
	// Record internal fragmentation
	malloc_record_ifrag(newBlock, size);
#endif

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
	LEAVE_CRITICAL_SECTION();
	ker_log( SOS_LOG_MALLOC, id, reqBlocks );
	return newBlock->userPart;
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
	printMem("malloc_start: ");
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
#ifdef SOS_PROFILE_FRAGMENTATION
		//
		// otherwise it is an external fragmentation
		malloc_record_efrag( block );
#endif
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

#ifdef SOS_PROFILE_FRAGMENTATION
	// Record internal fragmentation
	malloc_record_ifrag(block, size);
#endif
  
  block->blockhdr.blocks |= RESERVED;
  block->blockhdr.owner = id;

#ifdef SOS_SFI
  domid = sfi_get_domain_id(id);
  memmap_set_perms((void*) block, sizeof(Block), DOM_SEG_START(domid));
  memmap_set_perms((void*) ((Block*)(block + 1)), sizeof(Block) * (reqBlocks - 1), DOM_SEG_LATER(domid));
#else                           
  BLOCK_GUARD_BYTE(block) = id; 
#endif

  printMem("malloc_end: ");
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
  baseArea->blockhdr.blocks &= ~MEM_MASK;
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
  printMem("free_end: ");
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
  if (NULL_PID == id || NULL == ptr) return SOS_OK;           

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
  if ((blockptr < malloc_heap) || (blockptr >= (malloc_heap + mNumberOfBlocks)) ) {
		
	  return -EINVAL;
  }
  if (blockptr->blockhdr.owner != BLOCK_GUARD_BYTE(blockptr))  {
    DEBUG("sos_blk_mem_change_own: detect memory corruption %x\n", (int)blockptr);
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
#ifdef SOS_USE_GC
  sched_register_kernel_module(&malloc_module, sos_get_header_address(mod_header), NULL);
#endif
}

int8_t mem_remove_all(sos_pid_t id)
{
  HAS_CRITICAL_SECTION;
  Block* block = (Block*)malloc_heap;

  ENTER_CRITICAL_SECTION();
  for (block = (Block*)malloc_heap; 
       block != mSentinel; 
       block += block->blockhdr.blocks & ~MEM_MASK) 
    {
      if ( (block->blockhdr.owner == id) && (block->blockhdr.blocks & RESERVED) ){
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
  if ( (pntr == NULL ) || (newSize == 0) ) {
	  return pntr;
  }

  printMem("realloc start: ");
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
	  block->blockhdr.blocks |= RESERVED;         // convert it back
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
  printMem("realloc end: ");
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

#ifdef SOS_PROFILE_FRAGMENTATION
	mf.malloc_max_efrag = 0;
	mf.malloc_efrag = 0;
	mf.malloc_efrag_cnt = 0;
	mf.malloc_ifrag = 0;
	mf.malloc_alloc_cnt = 0;
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
	/*
  Block* p = mSentinel->next;
  mSentinel->next = block;
  block->prev = mSentinel;
  block->next = p;
  p->prev = block;
  */

  Block *p = mSentinel->next;

  while( p < block && p != mSentinel) {
	  p = p->next;
  }
  p->prev->next = block;
  block->prev = p->prev;
  p->prev = block;
  block->next = p;
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
#endif

#ifdef SOS_USE_GC
static int8_t mem_handler(void *state, Message *msg)
{
  switch(msg->type){
  case MSG_TIMER_TIMEOUT:
    {
      //mem_defrag();
	  //led_yellow_toggle();
	  malloc_gc_kernel();
	  //led_yellow_toggle();
      break;
    }
  case MSG_INIT:
    {
      ker_timer_init(KER_MEM_PID, 0, TIMER_REPEAT);
      ker_timer_start(KER_MEM_PID, 0, MEM_GC_PERIOD);
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
#endif /* #ifdef SOS_USE_GC */

#if 0
static void verify_memory( void )
{
  Block* block;
  block = (Block*)malloc_heap;
  Block* next_block;
  while(block != mSentinel) {
    next_block = block + (block->blockhdr.blocks & ~MEM_MASK);
    if( block->blockhdr.blocks & RESERVED ) {
      if( block->blockhdr.owner != BLOCK_GUARD_BYTE(block) ) {
	ker_led(LED_RED_TOGGLE);
	return;
      }
    }
    if( next_block != mSentinel) {
      if( (next_block->blockhdr.blocks & ~MEM_MASK) > ((MALLOC_HEAP_SIZE + (BLOCK_SIZE - 1))/BLOCK_SIZE) ) {
	ker_led(LED_GREEN_TOGGLE);
	ker_led(LED_RED_TOGGLE);
	return;
      }	
    }
    block = next_block;
  }

}
#endif

int8_t ker_gc_mark( sos_pid_t pid, void *pntr )
{
	Block* baseArea;   // convert to a block address
	Block* itr;
	
	baseArea = TO_BLOCK_PTR(pntr);   // convert to a block address
	
	if ( (baseArea < malloc_heap) || (baseArea >= (malloc_heap + mNumberOfBlocks)) ) {
		// Not a valid block
		return -EINVAL;
	}
	
	//
	// Traverse the memory list to make sure that this is a valid memory block
	//
	itr = (Block*)malloc_heap;
	while(itr != mSentinel && itr >= malloc_heap && itr < &(malloc_heap[NUM_HEAP_BLOCKS])) {
		if( itr == baseArea ) {
			if( itr->blockhdr.owner == pid ) {
				DEBUG_GC("Mark memory: %d\n", (int) itr->userPart);
				itr->blockhdr.blocks |= GC_MARK;
				return SOS_OK;
			}
			return -EINVAL;
		}
		itr += itr->blockhdr.blocks & ~MEM_MASK;
	}
	return -EINVAL;
}

//
// GC a module
//
void malloc_gc(sos_pid_t pid)
{
#ifdef SOS_DEBUG_GC
	int i;
#endif
	Block* block = (Block*)malloc_heap;
	//
	// Traverse the memory
	// Look for matching pid
	// If the memory is reserved and is not marked, free it
	//
	for (block = (Block*)malloc_heap; 
       block != mSentinel; 
       block += block->blockhdr.blocks & ~MEM_MASK) 
    {
		if ( (block->blockhdr.owner == pid) &&
		((block->blockhdr.blocks & RESERVED) != 0) ) { 
			if( ((block->blockhdr.blocks & GC_MARK) == 0) ){
				DEBUG_GC("Found memory leak: %d\n", (int) block->userPart);
				ker_free(block->userPart);
			} else {
				block->blockhdr.blocks &= ~GC_MARK;
			}		
		}
	}
	
#ifdef SOS_DEBUG_GC
	for (block = (Block*)malloc_heap, i= 0; 
       block != mSentinel; 
       block += block->blockhdr.blocks & ~MEM_MASK) 
    {
		DEBUG_GC("block %d : addr: %x size: %d alloc: %d owner: %d check %d\n", i++, 
	  (unsigned int) block, 
	  (unsigned int) (block->blockhdr.blocks & ~MEM_MASK), 
	  (unsigned int) (block->blockhdr.blocks & RESERVED)? 1 : 0, 
	  (unsigned int) block->blockhdr.owner,
	  (unsigned int) BLOCK_GUARD_BYTE(block));
    }
#endif

}

//
// GC entire kernel
//
void malloc_gc_kernel( void )
{
#ifdef SOS_USE_GC
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	shm_gc();
	LEAVE_CRITICAL_SECTION();

	ENTER_CRITICAL_SECTION();
	timer_gc();
	LEAVE_CRITICAL_SECTION();

	ENTER_CRITICAL_SECTION();
	sched_gc();
	LEAVE_CRITICAL_SECTION();
#ifdef SOS_RADIO_CHANNEL
	ENTER_CRITICAL_SECTION();
	radio_gc();
	LEAVE_CRITICAL_SECTION();
#endif

#ifdef SOS_UART_CHANNEL
	ENTER_CRITICAL_SECTION();
	uart_gc();
	LEAVE_CRITICAL_SECTION();
#endif

	ENTER_CRITICAL_SECTION();
	mq_gc();
	LEAVE_CRITICAL_SECTION();
#endif
}

uint8_t malloc_gc_module( sos_pid_t pid )
{
#ifdef SOS_USE_GC
	sos_module_t *mcb;
	Block* block;
	uint8_t mod_memmap_cnt = 0;
	Block** mod_memmap;
	uint8_t mod_stack_sp = 0;
	Block** mod_gc_stack;
	
	Block*  mod_memmap_buf[MEM_MOD_GC_STACK_SIZE];
	Block*  mod_gc_stack_buf[MEM_MOD_GC_STACK_SIZE];
	uint8_t num_leaks = 0;
	HAS_CRITICAL_SECTION;
	//
	// Get module control block
	//
	mcb = ker_get_module( pid );
	
	if( mcb == NULL || mcb->handler_state == NULL) {
		return 0;
	}
	
	if( (mcb->flag & SOS_KER_STATIC_MODULE) != 0 ) {
		// Don't check for static module (kernel module)
		return 0;
		//Block* baseArea; 
		//baseArea = TO_BLOCK_PTR(mcb->handler_state);
		//baseArea->blockhdr.blocks |= GC_MARK;
	}
	
	ENTER_CRITICAL_SECTION();
	//
	// get number of blocks we need to check against
	//
	DEBUG_GC("in malloc_gc_module\n");
	DEBUG_GC("get number of blocks\n");
	for (block = (Block*)malloc_heap; 
       block != mSentinel; 
       block += block->blockhdr.blocks & ~MEM_MASK) 
    {
		if ( (block->blockhdr.owner == pid) &&
		((block->blockhdr.blocks & RESERVED) != 0) ) { 
			mod_memmap_cnt++;
		}
	}
	
	DEBUG_GC("allocate memory: mod_memmap_cnt = %d\n", mod_memmap_cnt);
	//
	// Allocate memory
	//
	if( mod_memmap_cnt < MEM_MOD_GC_STACK_SIZE ) {
		mod_memmap = mod_memmap_buf;
		mod_gc_stack = mod_gc_stack_buf;
	} else {
		mod_memmap = ker_malloc( sizeof(Block*) * mod_memmap_cnt, KER_MEM_PID );
		if( mod_memmap == NULL ) {
			LEAVE_CRITICAL_SECTION();
			DEBUG_GC("no memory\n");
			return 0;
		}
		
		mod_gc_stack = ker_malloc( sizeof(Block*) * mod_memmap_cnt, KER_MEM_PID );
		if( mod_gc_stack == NULL ) {
			ker_free( mod_memmap );
			LEAVE_CRITICAL_SECTION();
			DEBUG_GC("no memory\n");
			return 0;
		}
	}
	
	//
	// Get all blocks in place
	//
	DEBUG_GC("get all blocks in place\n");
	for (block = (Block*)malloc_heap, mod_memmap_cnt = 0; 
       block != mSentinel; 
       block += block->blockhdr.blocks & ~MEM_MASK) 
    {
		if ( (block->blockhdr.owner == pid) &&
		((block->blockhdr.blocks & RESERVED) != 0) ) {
			mod_memmap[mod_memmap_cnt] = block;
			mod_memmap_cnt++;
		}
	}
	
	//
	// Use module state as the root
	//
	mod_gc_stack[0] = TO_BLOCK_PTR(mcb->handler_state);
	//
	// Mark this item checked
	//
	(mod_gc_stack[0])->blockhdr.blocks |= GC_MARK;
	mod_stack_sp = 1;
	
	//
	// Run until all items in the stack is checked
	//
	DEBUG_GC("Mark memory\n");
	while( mod_stack_sp != 0 ) {
		uint16_t mem_size; // memory size to check 
		uint16_t i;
		uint8_t *userPart;
		
		mod_stack_sp--;
		block = mod_gc_stack[ mod_stack_sp ];
		
		mem_size = BLOCKS_TO_BYTES( block->blockhdr.blocks );
		userPart = block->userPart;
		
		for( i = 0; i < mem_size; i++ ) {
			void *pntr;
			uint8_t j;
			//
			// treated as double pointers
			//
			pntr = *((uint8_t**)(userPart + i));
			
			//
			// Check against the memmap
			//
			for( j = 0; j < mod_memmap_cnt; j++ ) {
				if( pntr == (mod_memmap[j])->userPart ) {
					if( (((mod_memmap[j])->blockhdr.blocks) & GC_MARK) == 0 ) {
						// found a match, added to sp
						DEBUG_GC("Found a match addr: %d index: %d, value: %d\n", (int)userPart, (int) i, (int)pntr);
						(mod_memmap[j])->blockhdr.blocks |= GC_MARK;
						mod_gc_stack[ mod_stack_sp ] = mod_memmap[j];
						mod_stack_sp++;
					}
				}
			}
		}
	}
	
	DEBUG_GC("do module GC\n");
	//
	// Now do GC
	//
	{
		uint8_t k;
		
		for( k = 0; k < mod_memmap_cnt; k++ ) {
			if( ((mod_memmap[k])->blockhdr.blocks & GC_MARK) == 0 ) {
				// found leak...
				DEBUG_GC("Found memory leak: %d\n", (int) (mod_memmap[k])->userPart);
				num_leaks++;
				ker_free( (mod_memmap[k])->userPart );
			} else {
				(mod_memmap[k])->blockhdr.blocks &= ~GC_MARK;
			}
		} 
	}
	
	DEBUG_GC("memory cleanup\n");
	//
	// Clean up
	//
	if( mod_memmap_cnt < MEM_MOD_GC_STACK_SIZE ) {
		ker_free( mod_memmap );
		ker_free( mod_gc_stack );
	}
	LEAVE_CRITICAL_SECTION();
	return num_leaks;
#else
	return 0;
#endif	
}

#ifdef SOS_DEBUG_MALLOC
#ifndef SOS_SFI
static void printMem(char* s)
{
  Block* block;
  int i = 0;

  DEBUG("%s\n", s);
  for (block = mSentinel->next; block != mSentinel && block < &(malloc_heap[NUM_HEAP_BLOCKS]) && block >= malloc_heap; 
  block = block->next)
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
  while(block != mSentinel && block >= malloc_heap && block < &(malloc_heap[NUM_HEAP_BLOCKS])) {
    DEBUG("block %d : addr: %x size: %d alloc: %d owner: %d check %d\n", i++, 
	  (unsigned int) block, 
	  (unsigned int) (block->blockhdr.blocks & ~MEM_MASK), 
	  (unsigned int) (block->blockhdr.blocks & RESERVED)? 1 : 0, 
	  (unsigned int) block->blockhdr.owner,
	  (unsigned int) BLOCK_GUARD_BYTE(block));
	if( (block->blockhdr.blocks & ~MEM_MASK) == 0 ) {
		DEBUG("blocks is zero\n");
		exit(1);
	}
    block += block->blockhdr.blocks & ~MEM_MASK;
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
    DEBUG("block %d : Addr: %x size: %d alloc: %d owner: %d\n", i++, (uint32_t)block, block->blockhdr.blocks & ~MEM_MASK, (block->blockhdr.blocks & RESERVED)? 1: 0, block->blockhdr.owner);
    block += block->blockhdr.blocks & ~MEM_MASK;
  }
}
#endif // SOS_SFI
#endif

#ifdef SOS_PROFILE_FRAGMENTATION
static void malloc_record_efrag(Block *b)
{
	mf.malloc_efrag_cnt++;
	if( b->blockhdr.blocks > mf.malloc_max_efrag ) {            
		mf.malloc_max_efrag = b->blockhdr.blocks;        
	}
	mf.malloc_efrag += b->blockhdr.blocks;
#ifdef SOS_SIM
	printf("max efrag = %d bytes\n", mf.malloc_max_efrag << SHIFT_VALUE);
	printf("average efrag = %f\n", (float)mf.malloc_efrag * (1<<SHIFT_VALUE) / (float)mf.malloc_efrag_cnt );
#endif
	
}

static void malloc_record_ifrag(Block *b, uint16_t size)
{
	mf.malloc_alloc_cnt++;	
	mf.malloc_ifrag += ((b->blockhdr.blocks << SHIFT_VALUE) - (size + BLOCKOVERHEAD));    
#ifdef SOS_SIM
	printf("internal frag = %d\n",  
		(int)((b->blockhdr.blocks << SHIFT_VALUE) - (size + BLOCKOVERHEAD)));    
	printf("average internel frag = %f\n", (float)mf.malloc_ifrag / mf.malloc_alloc_cnt);
#endif
}
#endif

void* ker_sys_malloc(uint16_t size)
{    
  sos_pid_t my_id = ker_get_current_pid();    
  void *ret = sos_blk_mem_alloc(size, my_id, true);    
  if( ret != NULL ) {        
    return ret;    
  }    
  if( size == 0 ) {
	return NULL;
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
  if( newSize == 0 ) {
	return NULL;
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


