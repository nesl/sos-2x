#ifndef _RES_MNGR_H_INCL_
#define _RES_MNGR_H_INCL_

#include <VM/Dvm.h>

#define NULL_CAPSULE (DVM_CAPSULE_NUM + 1)
#define DVM_STATE_SIZE				sizeof(DvmState)
#define DVM_NUM_SCRIPT_BLOCKS		2
#define DVM_DEFAULT_MEM_ALLOC_SIZE	(DVM_NUM_SCRIPT_BLOCKS*DVM_STATE_SIZE)


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef struct {
  DvmState *scripts[DVM_CAPSULE_NUM];
  uint8_t script_block_owners[DVM_NUM_SCRIPT_BLOCKS];
  DvmState *script_block_ptr;
} DVMResourceManager_state_t;


DvmState* mem_allocate(uint8_t capsuleNum);

int8_t mem_free(uint8_t capsuleNum);

/**
 * \brief Resource Manager Handler Function
 */
int8_t resmanager_handler(void *state, Message *msg);


#endif//_RES_MNGR_H_INCL_
