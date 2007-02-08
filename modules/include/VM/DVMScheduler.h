#ifndef _SCHED_H_INCL_
#define _SCHED_H_INCL_

#include <VM/Dvm.h>

//--------------------------------------------------------------
// DVM SCHEDULER STATE
//--------------------------------------------------------------
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

//--------------------------------------------------------------
// MESSAGE HANDLER DECLARATIONS
//--------------------------------------------------------------
int8_t dvmsched_init(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_final(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_run_task(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_halt(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_resume(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_timeout(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_add_lib(dvm_state_t* dvm_st, Message *msg);
int8_t dvmsched_rem_lib(dvm_state_t* dvm_st, Message *msg);
//--------------------------------------------------------------
// EXTERNAL FUNCTION DECLARATIONS
//--------------------------------------------------------------
void engineReboot(dvm_state_t* dvm_st);
int8_t scheduler_submit(dvm_state_t* dvm_st, DvmContext* context); 
int8_t error(dvm_state_t* dvm_st, DvmContext* context, uint8_t cause);


#endif//_SCHED_H_INCL_
