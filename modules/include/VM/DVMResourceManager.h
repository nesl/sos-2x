#ifndef _RES_MNGR_H_INCL_
#define _RES_MNGR_H_INCL_

#include <VM/Dvm.h>
#include <VM/dvm_types.h>

//----------------------------------------------------------------------------
// MESSAGE HANDLER DEFINITIONS
//----------------------------------------------------------------------------
int8_t resmanager_init(dvm_state_t* dvm_st, Message *msg);
int8_t resmanager_final(dvm_state_t* dvm_st, Message *msg);
int8_t resmanager_loader_data_handler(dvm_state_t* dvm_st, Message *msg);

//----------------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//----------------------------------------------------------------------------
DvmState *mem_allocate(dvm_state_t* dvm_st, uint8_t capsuleNum);
int8_t mem_free(dvm_state_t* dvm_st, uint8_t capsuleNum);



#endif//_RES_MNGR_H_INCL_
