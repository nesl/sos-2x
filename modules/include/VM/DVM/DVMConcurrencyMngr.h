#ifndef _CONC_MNGR_H_INCL_
#define _CONC_MNGR_H_INCL_

#include <VM/Dvm.h>

   
void synch_reset( func_cb_ptr p )
{}
     
void analyzeVars( func_cb_ptr p, DvmCapsuleID id )
{}

     
void clearAnalysis( func_cb_ptr p, DvmCapsuleID id )
{}


void initializeContext( func_cb_ptr p, DvmContext *  context )
{}

void yieldContext( func_cb_ptr p, DvmContext *  context )
{}

uint8_t resumeContext( func_cb_ptr p, DvmContext *  caller, DvmContext *  context )
{
	return 0;
}

void haltContext( func_cb_ptr p, DvmContext *  context )
{}


uint8_t isHeldBy( func_cb_ptr p, uint8_t lockNum, DvmContext *  context )
{ return 0; }

#endif
