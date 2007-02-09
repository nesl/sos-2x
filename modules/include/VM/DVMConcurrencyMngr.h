#ifndef _CONC_MNGR_H_INCL_
#define _CONC_MNGR_H_INCL_

#include <VM/Dvm.h>
#include <VM/dvm_types.h>

//-----------------------------------------------------------------
// MESSAGE HANDLER DEFINITIONS
//-----------------------------------------------------------------
int8_t concurrency_init(dvm_state_t* dvm_st, Message *msg);
int8_t concurrency_final(dvm_state_t* dvm_st, Message *msg);
int8_t concurrency_add_lib(dvm_state_t* dvm_st, Message *msg);
int8_t concurrency_rem_lib(dvm_state_t* dvm_st, Message *msg);

//-----------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//-----------------------------------------------------------------   
void synch_reset(dvm_state_t* dvm_st);
void analyzeVars(dvm_state_t* dvm_st, DvmCapsuleID id);
void clearAnalysis(dvm_state_t* dvm_st, DvmCapsuleID id); 
void initializeContext(dvm_state_t* dvm_st, DvmContext *context);
void yieldContext(dvm_state_t* dvm_st, DvmContext* context);
uint8_t resumeContext(dvm_state_t* dvm_st, DvmContext* caller, DvmContext* context);
void haltContext(dvm_state_t* dvm_st, DvmContext* context);
// Ram - Where is this function defined ??
//uint8_t isHeldBy(uint8_t lockNum, DvmContext* context);


#endif
