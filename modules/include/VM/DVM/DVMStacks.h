#ifndef _STACKS_H_INCL_
#define _STACKS_H_INCL_

#include <VM/Dvm.h>

     

int8_t resetStacks( func_cb_ptr p, DvmState *  eventState )
{ return -EINVAL; }


int8_t pushValue( func_cb_ptr p, DvmState *  eventState, int16_t val, uint8_t type )
{ return -EINVAL; }

      

int8_t pushBuffer( func_cb_ptr p, DvmState *  eventState, DvmDataBuffer *  buffer )
{ return -EINVAL; }
	
      

int8_t pushOperand( func_cb_ptr p, DvmState *  eventState, DvmStackVariable *  var )
{ return -EINVAL; }

    

DvmStackVariable *  popOperand( func_cb_ptr p, DvmState *  eventState )
{ return NULL; }


#endif
