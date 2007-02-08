/**
 * \file dvm_types.h
 * \brief DVM data type
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef __DVM_TYPES_H__
#define __DVM_TYPES_H__

// Ram - For the rest of the data types look at Dvm.h !! :-)

/**
 * \brief Encapsulate the complete state of the DVM module
 */
typedef struct _str_dvm_state {
  DVMScheduler_state_t sched_st; // This should be the first field (because of fn. ptr. ptr.)
  DVMResourceManager_state_t resmgr_st;
  DVMEventHandler_state_t evhdlr_st;
  DVMConcurrencyMngr_state_t conmgr_st;
} dvm_state_t;


#endif//__DVM_TYPES_H__
