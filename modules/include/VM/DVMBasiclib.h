#ifndef _BASICLIB_H_INCL_
#define _BASICLIB_H_INCL_

#include <VM/Dvm.h>
#include <VM/dvm_types.h>


//------------------------------------------------------------------
// MESSAGE HANDLER DEFINITIONS
//------------------------------------------------------------------
int8_t basic_library_init(dvm_state_t* dvm_st, Message *msg);
int8_t basic_library_final(dvm_state_t* dvm_st, Message* msg);
int8_t basic_library_data_ready(dvm_state_t* dvm_st, Message *msg);

//------------------------------------------------------------------
// EXTERNAL FUNCTION DEFINITIONS
//------------------------------------------------------------------
int8_t execute(dvm_state_t* dvm_st, DvmState* eventState);
void rebooted(dvm_state_t* dvm_st); 
int16_t lockNum(uint8_t instr);
uint8_t bytelength(uint8_t opcode);
//int8_t execute_extlib(uint8_t fnid, DvmStackVariable *arg, uint8_t size, DvmStackVariable *res);






#endif//_BASICLIB_H_INCL_
