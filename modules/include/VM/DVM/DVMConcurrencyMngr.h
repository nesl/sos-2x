#ifndef _CONC_MNGR_H_INCL_
#define _CONC_MNGR_H_INCL_

#include <VM/Dvm.h>

//-----------------------------------------------------------------
// TYPEDEFS
//-----------------------------------------------------------------
typedef struct 
{
  uint8_t usedVars[DVM_CAPSULE_NUM][(DVM_LOCK_COUNT + 7) / 8];
  DvmQueue readyQueue;
  uint8_t libraries;
  uint8_t extlib_module[4];
  DvmLock locks[DVM_LOCK_COUNT];
} DVMConcurrencyMngr_state_t;  

   
void synch_reset(dvm_state_t* dvm_st);
     
void analyzeVars(dvm_state_t* dvm_st, DvmCapsuleID id);
     
void clearAnalysis(dvm_state_t* dvm_st, DvmCapsuleID id); 

void initializeContext(dvm_state_t* dvm_st, DvmContext *context);

void yieldContext(dvm_state_t* dvm_st, DvmContext* context);

uint8_t resumeContext(dvm_state_t* dvm_st, DvmContext* caller, DvmContext* context);

void haltContext(dvm_state_t* dvm_st, DvmContext* context);

// Ram - Where is this function defined ??
//uint8_t isHeldBy(uint8_t lockNum, DvmContext* context);

/**
 * \brief Message Handler
 */
int8_t concurrency_handler(void *state, Message *msg) ;


#endif
