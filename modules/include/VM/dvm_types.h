/**
 * \file dvm_types.h
 * \brief DVM data type
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef __DVM_TYPES_H__
#define __DVM_TYPES_H__

#include <VM/Dvm.h>
#include <VM/DvmConstants.h>
// Ram - For the rest of the data types look at Dvm.h !! :-)


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
#define NULL_CAPSULE                    (DVM_CAPSULE_NUM + 1)
#define DVM_STATE_SIZE			sizeof(DvmState)
#define DVM_NUM_SCRIPT_BLOCKS		2
#define DVM_DEFAULT_MEM_ALLOC_SIZE	(DVM_NUM_SCRIPT_BLOCKS*DVM_STATE_SIZE)

typedef struct {
  DvmState *scripts[DVM_CAPSULE_NUM];
  uint8_t script_block_owners[DVM_NUM_SCRIPT_BLOCKS];
  DvmState *script_block_ptr;
} DVMResourceManager_state_t;

typedef struct 
{
  uint8_t usedVars[DVM_CAPSULE_NUM][(DVM_LOCK_COUNT + 7) / 8];
  DvmQueue readyQueue;
  uint8_t libraries;
  uint8_t extlib_module[4];
  DvmLock locks[DVM_LOCK_COUNT];
} DVMConcurrencyMngr_state_t;  

typedef struct {
  func_cb_ptr ext_execute[4];
  DvmQueue runQueue;
  DvmContext* runningContext;
  DvmErrorMsg errorMsg;
  uint8_t libraries;
  struct {
  uint8_t inErrorState  : 1;
  uint8_t errorFlipFlop : 1;
  uint8_t taskRunning   : 1;
  uint8_t halted        : 1;
  } flags;
} DVMScheduler_state_t;

typedef struct {
  DvmState* stateBlock[DVM_CAPSULE_NUM];
} DVMEventHandler_state_t;

typedef struct 
{
  uint8_t busy;			  // for GET_DATA
  DvmContext *executing;	  // for GET_DATA
  DvmQueue getDataWaitQueue;	  // for GET_DATA
  DvmContext *nop_executing;	  // for NOP
  uint16_t delay_cnt;             // for NOP
  DvmDataBuffer buffers[DVM_NUM_BUFS];
  DvmStackVariable shared_vars[DVM_NUM_SHARED_VARS];
  int32_t fl_post;			//for posting a int32_t/float directly
} DVMBasiclib_state_t;


typedef struct _str_dvm_state {
  func_cb_ptr execute_syncall;
  DVMScheduler_state_t sched_st; // This should be the second field (because of fn. ptr. ptr.)
  DVMResourceManager_state_t resmgr_st;
  DVMEventHandler_state_t evhdlr_st;
  DVMConcurrencyMngr_state_t conmgr_st;
  DVMBasiclib_state_t basiclib_st;
} dvm_state_t;


#endif//__DVM_TYPES_H__
