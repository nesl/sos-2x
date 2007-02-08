#ifndef _EVENT_H_INCL_
#define _EVENT_H_INCL_

#include <VM/Dvm.h>
#include <VM/dvm_types.h>

//--------------------------------------------------------------------
// MESSAGE HANDLER DEFINITIONS
//--------------------------------------------------------------------
int8_t event_handler_init(dvm_state_t *dvm_st, Message *msg);
int8_t event_handler_final(dvm_state_t *dvm_st, Message *msg);
int8_t event_handler_timeout(dvm_state_t *dvm_st, Message *msg);
int8_t event_handler_default(dvm_state_t *dvm_st, Message *msg);

//--------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//--------------------------------------------------------------------
int8_t initEventHandler(dvm_state_t* dvm_st, DvmState *eventState, uint8_t capsuleID);
DvmCapsuleLength getCodeLength(dvm_state_t* dvm_st, uint8_t id);
uint8_t getLibraryMask(dvm_state_t* dvm_st, uint8_t id);
DvmState *getStateBlock (dvm_state_t* dvm_st, uint8_t id);
DvmOpcode getOpcode(dvm_state_t* dvm_st, uint8_t id, uint16_t which);
// Ram - Where is this function defined ?
//void startScriptTimer(uint8_t id, uint32_t msec);



#endif//_EVENT_H_INCL_
