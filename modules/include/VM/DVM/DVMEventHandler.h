#ifndef _EVENT_H_INCL_
#define _EVENT_H_INCL_

#include <VM/Dvm.h>


//--------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------
typedef struct {
  DvmState* stateBlock[DVM_CAPSULE_NUM];
} DVMEventHandler_state_t;

      
int8_t initEventHandler(dvm_state_t* dvm_st, DvmState *eventState, uint8_t capsuleID);

DvmCapsuleLength getCodeLength(dvm_state_t* dvm_st, uint8_t id);

uint8_t getLibraryMask(dvm_state_t* dvm_st, uint8_t id);

DvmState *getStateBlock (dvm_state_t* dvm_st, uint8_t id);

DvmOpcode getOpcode(dvm_state_t* dvm_st, uint8_t id, uint16_t which);

// Ram - Where is this function defined ?
//void startScriptTimer(uint8_t id, uint32_t msec);

/**
 * \brief event_handler message handler !!
 */
int8_t event_handler(void *state, Message *msg) ;

#endif//_EVENT_H_INCL_
