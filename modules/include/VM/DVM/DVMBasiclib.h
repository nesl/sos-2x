#ifndef _BASICLIB_H_INCL_
#define _BASICLIB_H_INCL_

#include <VM/Dvm.h>

#ifndef PHOTO
#define PHOTO 0
#endif

void rebooted( func_cb_ptr p )
{ }

     
int8_t execute( func_cb_ptr p, DvmState *  eventState )
{ return -1; }

     
int16_t lockNum( func_cb_ptr p, uint8_t instr )
{ return -1; }

     
uint8_t bytelength( func_cb_ptr p, uint8_t opcode )
{ return 0; }

int8_t execute_extlib(func_cb_ptr cb, uint8_t fnid, DvmStackVariable *arg, uint8_t size, DvmStackVariable *res)
{ return -1; }
#endif
