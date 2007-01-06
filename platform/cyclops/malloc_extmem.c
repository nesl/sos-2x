/**
 * \file malloc_extmem.c
 * \brief Dynamic Memory Manager for the Cyclops External Memory
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <malloc_extmem.h>
#include <ext_memmap.h>

//-----------------------------------------------------
// TYPEDEFS
//-----------------------------------------------------
typedef struct _ExtBlockHdr{
  uint8_t size;
  struct _ExtBlockHdr *prev;
  struct _ExtBlockHdr *next;
} ExtBlockHdr_t;


typedef struct _ExtBlock {
  union
  {
    ExtBlockHdr_t blkhdr;
    uint8_t data[EXT_MEM_BLK_SIZE]; // Actual External Memory
  };
} ExtBlock_t;

typedef struct _ExtBlockHdl {
  int16_t handle;   // Memory Handle
  sos_pid_t owner;  // Owner PID
  union 
  {
    struct
    {
      uint8_t size;       // Size in number of blocks
      ExtBlock_t* pblock; // Pointer to memory block
    };
    struct _ExtBlockHdl *nexthdl;
  };
} ExtBlockHdl_t;

//-----------------------------------------------------
// LOCAL STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------
static void InsertAfter(ExtBlockHdr_t* blockhdr);
static ExtBlockHdr_t* MergeBlocks(ExtBlockHdr_t* block);
static void Unlink(ExtBlockHdr_t* block);
static void SplitBlock(ExtBlockHdr_t* block, uint16_t reqBlocks);  


//-----------------------------------------------------
// LOCAL GLOBALS
//-----------------------------------------------------
static ExtBlockHdr_t FreeBlkHdr;
static ExtBlockHdr_t* mExtSentinel;
static ExtBlockHdl_t* ExtBlockHandleFreeList;
static ExtBlockHdl_t* ExtBlockHandleTable;
//static ExtBlockHdl_t ExtBlockHandleTable[EXT_MEM_MAX_SEG];



int8_t ext_mem_init()
{
  uint16_t i;
  ExtBlock_t* head;

  // Initialize the local globals
  ExtBlockHandleFreeList = (ExtBlockHdl_t*) EXT_MEM_HANDLE_START;
  ExtBlockHandleTable = (ExtBlockHdl_t*) EXT_MEM_HANDLE_START;
  //  ExtBlockHandleFreeList = &ExtBlockHandleTable[0];
  mExtSentinel = &FreeBlkHdr;

  
  mExtSentinel->size = 0;
  mExtSentinel->prev = mExtSentinel;
  mExtSentinel->next = mExtSentinel;
  head = (ExtBlock_t*)EXT_MEM_HEAP_START;
  head->blkhdr.size = EXT_MEM_NUM_BLKS;
  InsertAfter((ExtBlockHdr_t*)head);

  // Initialize the handler list
  for (i = 0; i < EXT_MEM_MAX_SEG; i++){
    ExtBlockHandleTable[i].handle = i;
    ExtBlockHandleTable[i].owner = NULL_PID;
    ExtBlockHandleTable[i].nexthdl = &ExtBlockHandleTable[i+1];
  }
  ExtBlockHandleTable[EXT_MEM_MAX_SEG-1].nexthdl = NULL;

  // Initialize the external memory map
  ext_memmap_init();  

  return SOS_OK;
}


int16_t ext_mem_get_handle(uint16_t size, sos_pid_t id, bool bCallFromModule)
{
  HAS_CRITICAL_SECTION;
  uint16_t reqBlocks;
  ExtBlockHdr_t *blockhdr;
  ExtBlockHdl_t *blockhdl;

  // Check Errors
  if (0 == size){
    return -EINVAL;
  }

  // Check Failures
  if (NULL == ExtBlockHandleFreeList){
    return -ENOMEM;
  }

  // Get segment size
  reqBlocks = ((size + EXT_MEM_BLK_SIZE - 1) >> 8);
  //  reqBlocks++; // Ram - Doing this to prevent memory corruption by the CPLD + Imager // Only for Debug

  // First-fit search for the block
  ENTER_CRITICAL_SECTION();
  for (blockhdr = mExtSentinel->next; blockhdr != mExtSentinel; blockhdr = blockhdr->next){
    blockhdr = MergeBlocks(blockhdr);
    if (blockhdr->size >= reqBlocks){
      break;
    }
  }

  // Exit with error if no memory
  if (mExtSentinel == blockhdr){
    LEAVE_CRITICAL_SECTION();
    return -ENOMEM;
  }

  // Fragment the memory
  if (blockhdr->size > reqBlocks){
    SplitBlock(blockhdr, reqBlocks);
  }

  // Get the block
  Unlink(blockhdr);

  // Get the block handle
  blockhdl = ExtBlockHandleFreeList;
  ExtBlockHandleFreeList = ExtBlockHandleFreeList->nexthdl;

  // Fill the block handle
  blockhdl->owner = id;
  blockhdl->size = reqBlocks;
  blockhdl->pblock = (ExtBlock_t*)blockhdr;

  // Fill out the permissions table
  if (bCallFromModule){
    ext_memmap_set_perms((void*) blockhdr, sizeof(ExtBlock_t) * reqBlocks, EXT_BLOCK_ALLOC_MODULE);
  }
  else{
    ext_memmap_set_perms((void*) blockhdr, sizeof(ExtBlock_t) * reqBlocks, EXT_BLOCK_ALLOC_KERNEL);
  }
  
  LEAVE_CRITICAL_SECTION();
  
  return blockhdl->handle;
}


int8_t ext_mem_free_handle(int16_t handle, bool bCallFromModule)
{
  HAS_CRITICAL_SECTION;
  //  uint8_t ext_block_num;
  //  uint8_t perms;
  ExtBlockHdl_t* blockhdl;
  ExtBlockHdr_t* blockhdr;

  // Check for errors
  if ((handle < 0) ||
      handle >= EXT_MEM_MAX_SEG){
    return -EINVAL;
  }

  ENTER_CRITICAL_SECTION();
  blockhdl = &(ExtBlockHandleTable[handle]);
  blockhdr = (ExtBlockHdr_t*)blockhdl->pblock;

  // Get the permissions for the first block
  /*  ext_block_num = EXT_MEMMAP_GET_BLOCK_NUM(blockhdr); */
  /*   EXT_MEMMAP_GET_PERMS(ext_block_num, perms); */
  /*   if ((bCallFromModule) && */
  /*       ((perms & EXT_BLOCK_TYPE_BM) == EXT_BLOCK_KERNEL)){ */
  /*     LEAVE_CRITICAL_SECTION(); // Ram - We actually need to call an exception handler here */
  /*     return -EINVAL; */
  /*   } */
  
  // Clear the permissions
  ext_memmap_set_perms((void*)blockhdr, sizeof(ExtBlock_t)*blockhdl->size, EXT_BLOCK_FREE);

  // Return Block To Free List
  blockhdr->size = blockhdl->size;
  InsertAfter(blockhdr);

  // Clear the block handle
  blockhdl->owner = NULL_PID;
  blockhdl->nexthdl = ExtBlockHandleFreeList;
  ExtBlockHandleFreeList = blockhdl;

  LEAVE_CRITICAL_SECTION();
  return SOS_OK;
}


void* ker_get_handle_ptr(int16_t handle)
{
  ExtBlockHdl_t* blockhdl;
  if ((handle < 0) ||
      handle >= EXT_MEM_MAX_SEG){
    return NULL;
  }
  blockhdl = &(ExtBlockHandleTable[handle]);
  if (NULL_PID == blockhdl->owner){
    return NULL;
  }
  return blockhdl->pblock;
}

//-----------------------------------------------------------------------------
// EXT MALLOC HELPER ROUTINES
//-----------------------------------------------------------------------------
static void InsertAfter(ExtBlockHdr_t* blockhdr)
{
  ExtBlockHdr_t* p = mExtSentinel->next;
  mExtSentinel->next = blockhdr;
  blockhdr->prev = mExtSentinel;
  blockhdr->next = p;
  p->prev = blockhdr;
}



static ExtBlockHdr_t* MergeBlocks(ExtBlockHdr_t* block)
{
  while (1){
    uint8_t ext_block_num;
    uint8_t perms;
    ExtBlock_t* successor = (ExtBlock_t*)((ExtBlock_t*)block + block->size);
    ext_block_num = EXT_MEMMAP_GET_BLOCK_NUM(successor);
    EXT_MEMMAP_GET_PERMS(ext_block_num, perms);
    if ((EXT_BLOCK_FREE_BM & perms) == EXT_BLOCK_ALLOC){
      return block;
    }
    Unlink((ExtBlockHdr_t*)successor);
    block->size += successor->blkhdr.size;
  }
}


static void Unlink(ExtBlockHdr_t* block)
{
  block->prev->next = block->next;
  block->next->prev = block->prev;
}

static void SplitBlock(ExtBlockHdr_t* block, uint16_t reqBlocks)  
{
  ExtBlock_t* newBlock = (ExtBlock_t*)((ExtBlock_t*)block + reqBlocks); // create a remainder area
  newBlock->blkhdr.size = block->size - reqBlocks; // set its size and mark as free
  block->size = reqBlocks;                      // set us to requested size
  InsertAfter((ExtBlockHdr_t*)newBlock);        // stitch remainder into free list
}



