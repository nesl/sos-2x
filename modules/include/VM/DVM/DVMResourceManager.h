#ifndef _RES_MNGR_H_INCL_
#define _RES_MNGR_H_INCL_

#include <VM/Dvm.h>

#define NULL_CAPSULE (DVM_CAPSULE_NUM + 1)
#define DVM_STATE_SIZE				sizeof(DvmState)
#define DVM_NUM_SCRIPT_BLOCKS		2
#define DVM_DEFAULT_MEM_ALLOC_SIZE	(DVM_NUM_SCRIPT_BLOCKS*DVM_STATE_SIZE)

    

DvmState *  mem_allocate( func_cb_ptr p, uint8_t capsuleNum )
{
	return NULL;
}

    

int8_t mem_free( func_cb_ptr p, uint8_t capsuleNum )
{
	return -EINVAL;
}


#endif
