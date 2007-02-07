#ifndef _BASICLIB_H_INCL_
#define _BASICLIB_H_INCL_

#include <VM/Dvm.h>



void rebooted();
     
int8_t execute(DvmState* eventState);
     
int16_t lockNum(uint8_t instr);
     
uint8_t bytelength(uint8_t opcode);

int8_t execute_extlib(uint8_t fnid, DvmStackVariable *arg, 
		      uint8_t size, DvmStackVariable *res);

#endif//_BASICLIB_H_INCL_
