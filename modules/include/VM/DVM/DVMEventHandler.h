#ifndef _EVENT_H_INCL_
#define _EVENT_H_INCL_

#include <VM/Dvm.h>

      
int8_t initEventHandler( func_cb_ptr p, DvmState *  eventState, uint8_t capsuleID )
{
	return -EINVAL;
}
    

DvmCapsuleLength getCodeLength( func_cb_ptr p, uint8_t id )
{
	return 0;
}

DvmOpcode getOpcode( func_cb_ptr p, uint8_t id, uint16_t which )
{
	return 0;
}
    

uint8_t getLibraryMask( func_cb_ptr p, uint8_t id )
{
	return 0;
}
	
     
DvmState *  getStateBlock( func_cb_ptr p, uint8_t id )
{
	return 0;
}

void startScriptTimer( func_cb_ptr p, uint8_t id, uint32_t msec )
{
	return;
}

#endif
