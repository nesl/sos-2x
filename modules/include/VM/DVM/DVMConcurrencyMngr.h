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

   
void synch_reset();
     
void analyzeVars(DvmCapsuleID id);
     
void clearAnalysis(DvmCapsuleID id);

void initializeContext(DvmContext* context);

void yieldContext(DvmContext* context);

uint8_t resumeContext(DvmContext* caller, DvmContext* context);

void haltContext(DvmContext* context);

uint8_t isHeldBy(uint8_t lockNum, DvmContext* context);

/**
 * \brief Message Handler
 */
int8_t concurrency_handler(void *state, Message *msg) ;


#endif
