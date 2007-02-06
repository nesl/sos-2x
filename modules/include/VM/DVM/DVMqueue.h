#ifndef _DVM_QUEUE_H_INCL_
#define _DVM_QUEUE_H_INCL_

#include <VM/Dvm.h>
#include <sos_list.h>

     

int8_t queue_init( func_cb_ptr p, DvmQueue *  queue )
{
	return -EINVAL;
}

    

uint8_t queue_empty( func_cb_ptr p, DvmQueue *  queue )
{
	return 0;
}
	
        

int8_t queue_enqueue( func_cb_ptr p, DvmContext *  context, DvmQueue *  queue, DvmContext *  element )
{
	return -EINVAL;
}

      

DvmContext *  queue_dequeue( func_cb_ptr p, DvmContext *  context, DvmQueue *  queue )
{
	return NULL;
}

        

int8_t queue_remove( func_cb_ptr p, DvmContext *  context, DvmQueue *  queue, DvmContext *  element )
{
	return -EINVAL;
}


#endif
