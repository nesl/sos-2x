#ifndef _SCHED_H_INCL_
#define _SCHED_H_INCL_

#include <VM/Dvm.h>
#include <VM/dvm_types.h>

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
int8_t error(DvmContext* context, uint8_t cause);


#endif//_SCHED_H_INCL_
