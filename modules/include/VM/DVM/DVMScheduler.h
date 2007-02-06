#ifndef _SCHED_H_INCL_
#define _SCHED_H_INCL_

#include <VM/Dvm.h>


   
void engineReboot( func_cb_ptr p )
{}

     

int8_t scheduler_submit( func_cb_ptr p, DvmContext *  context )
{
	return -EINVAL;
}


int8_t error( func_cb_ptr p, DvmContext *  context, uint8_t cause )
{
	return -EINVAL;
}


#endif
