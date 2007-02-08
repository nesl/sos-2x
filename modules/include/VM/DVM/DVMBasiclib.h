#ifndef _BASICLIB_H_INCL_
#define _BASICLIB_H_INCL_

#include <VM/Dvm.h>


typedef struct 
{
  uint8_t busy;			  // for GET_DATA
  DvmContext *executing;	  // for GET_DATA
  DvmQueue getDataWaitQueue;	  // for GET_DATA
  DvmContext *nop_executing;	  // for NOP
  uint16_t delay_cnt;             // for NOP
  DvmDataBuffer buffers[DVM_NUM_BUFS];
  DvmStackVariable shared_vars[DVM_NUM_SHARED_VARS];
} DVMBasiclib_state_t;



void rebooted();     

int8_t execute(DvmState* eventState);

int16_t lockNum(uint8_t instr);

uint8_t bytelength(uint8_t opcode);


int8_t execute_extlib(uint8_t fnid, DvmStackVariable *arg, 
		      uint8_t size, DvmStackVariable *res);

/**
 * \brief Basic Library Handler
 */
int8_t basic_library(void *state, Message *msg);




#endif//_BASICLIB_H_INCL_
