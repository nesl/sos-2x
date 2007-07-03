#include <error_type.h>
#include <sos_types.h>

//----------------------------------------------------------------------------
// DEBUG
#ifdef DBGMODE
#include <stdio.h>
#define DEBUG(arg...) printf(arg)
#else
#define DEBUG(arg...)
#endif


/**
 * @brief error stub for function subscriber
 */
void error_v(func_cb_ptr p) 
{
	DEBUG("error_v is called\n");
}

int8_t error_8(func_cb_ptr p) 
{
	DEBUG("error_8 is called\n");
	return -1;
}	

int16_t error_16(func_cb_ptr p)
{
	DEBUG("error_16 is called\n");
	return -1;
}

int32_t error_32(func_cb_ptr p)
{
	DEBUG("error_32 is called\n");
	return -1;
}	

void* error_ptr(func_cb_ptr p)
{
	DEBUG("error_ptr is called\n");
	return NULL;
}
