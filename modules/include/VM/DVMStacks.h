#ifndef _STACKS_H_INCL_
#define _STACKS_H_INCL_

#include <VM/Dvm.h>

     

int8_t resetStacks(DvmState* eventState);

int8_t pushValue(DvmState* eventState, int16_t val, uint8_t type);

int8_t pushBuffer(DvmState* eventState, DvmDataBuffer* buffer);

int8_t pushOperand(DvmState* eventState, DvmStackVariable* var);

DvmStackVariable* popOperand(DvmState* eventState);



#endif//_STACKS_H_INCL_
