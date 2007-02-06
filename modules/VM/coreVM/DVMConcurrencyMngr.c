/*									tab:4
 *
 *
 * "Copyright (c) 2000-2004 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose, without fee, and without written
 * agreement is hereby granted, provided that the above copyright
 * notice, the following two paragraphs and the author appear in all
 * copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
 * DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 */
/*									tab:4
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.  By
 *  downloading, copying, installing or using the software you agree to
 *  this license.  If you do not agree to this license, do not download,
 *  install, copy or use the software.
 *
 *  Intel Open Source License 
 *
 *  Copyright (c) 2004 Intel Corporation 
 *  All rights reserved. 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 * 
 *	Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *	Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 *      Neither the name of the Intel Corporation nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INTEL OR ITS
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * 
 */
 /*
 * Authors:   Rahul Balani
 *	      Ilias Tsigkogiannis
 * History:   April 2005		Port to sos
 *	      September 29, 2005	Port to sos-1.x
 */

/**
 * @author Rahul Balani
 * @author Ilias Tsigkogiannis
 */

#include <module.h>
//#include <fntable.h>
//#include <led.h>
//#include <sos_sched.h>
#ifdef LINK_KERNEL  
//#define _MODULE_    
#endif

#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMqueue.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMBasiclib.h>

//typedefs for functions that are subscribed
typedef uint8_t (*func_u8u8_t)(func_cb_ptr cb, uint8_t);
typedef int16_t (*func_i16u8_t)(func_cb_ptr cb, uint8_t instr);

typedef struct 
{
	func_cb_ptr bytelength;
	func_cb_ptr lockNum;
	func_cb_ptr q_init;
	func_cb_ptr q_empty;
	
	func_cb_ptr q_enqueue;
	func_cb_ptr q_dequeue;
	func_cb_ptr q_remove;
	func_cb_ptr getCodeLength;
	
	func_cb_ptr get_libmask;
	func_cb_ptr get_opcode;
	func_cb_ptr reset_stack;
	func_cb_ptr sched_submit;
	uint8_t pid;
	uint8_t usedVars[DVM_CAPSULE_NUM][(DVM_LOCK_COUNT + 7) / 8];
	DvmQueue readyQueue;
	uint8_t libraries;
	uint8_t extlib_module[4];
	DvmLock locks[DVM_LOCK_COUNT];
} app_state;  

//function declarations
static inline uint8_t isRunnable(DvmContext* context, app_state *s) ;
static inline int8_t obtainLocks(DvmContext* caller, DvmContext* obtainer, app_state *s) ;
static inline int8_t releaseLocks(DvmContext* caller, DvmContext* releaser, app_state *s) ;
static inline int8_t releaseAllLocks(DvmContext* caller, DvmContext* releaser, app_state *s) ;

//functions from MLocks.
static inline void locks_reset(app_state *s) ;
static inline int8_t lock(DvmContext *, uint8_t, app_state *) ;
static inline int8_t unlock(DvmContext *, uint8_t, app_state *) ;
static inline uint8_t isLocked(uint8_t, app_state *) ;

static int8_t concurrency_handler(void *state, Message *msg) ;

#ifndef _MODULE_
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
    .mod_id           =  M_CONTEXT_SYNCH,
    .state_size       =  0,
    .num_sub_func     =  0,
    .num_prov_func    =  5,
    .module_handler   =  concurrency_handler,
    .funct = {
        {initializeContext, "vzv1", M_CONTEXT_SYNCH, INITIALIZECONTEXT},
        {yieldContext, "vzv1", M_CONTEXT_SYNCH, YIELDCONTEXT},
        {haltContext, "vzv1", M_CONTEXT_SYNCH, HALTCONTEXT},
        {resumeContext, "Czz2", M_CONTEXT_SYNCH, RESUMECONTEXT},
        {isHeldBy, "CCz2", M_CONTEXT_SYNCH, ISHELDBY},
    },
};

static sos_module_t concurrency_module;
static app_state sync_state;
int8_t concurrency_init() {
	return sched_register_kernel_module(&concurrency_module, sos_get_header_address(mod_header), &sync_state);
}
#else
static void synch_reset(func_cb_ptr p);
static void analyzeVars(func_cb_ptr p, DvmCapsuleID id); 
static void clearAnalysis(func_cb_ptr p, DvmCapsuleID id);
static void initializeContext(func_cb_ptr p, DvmContext *context);
static void yieldContext(func_cb_ptr p, DvmContext* context);
static uint8_t resumeContext(func_cb_ptr p, DvmContext* caller, DvmContext* context); 
static void haltContext(func_cb_ptr p, DvmContext* context); 
static uint8_t isHeldBy(func_cb_ptr p, uint8_t lockNum, DvmContext* context); 
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
    .mod_id           =  M_CONTEXT_SYNCH,
    .state_size       =  sizeof(app_state),
	.code_id        = ehtons(M_CONTEXT_SYNCH),
	.platform_type  = HW_TYPE,
	.processor_type = MCU_TYPE,
    .num_sub_func     =  12,
    .num_prov_func    =  8,
    .module_handler   =  concurrency_handler,
    .funct = {
        USE_FUNC_bytelength(M_BASIC_LIB, BYTELENGTH),
        USE_FUNC_lockNum(M_BASIC_LIB, LOCKNUM),
		USE_FUNC_queue_init(M_QUEUE, Q_INIT),          
		USE_FUNC_queue_empty(M_QUEUE, Q_EMPTY),        

		USE_FUNC_queue_enqueue(M_QUEUE, Q_ENQUEUE),        
		USE_FUNC_queue_dequeue(M_QUEUE, Q_DEQUEUE),        
		USE_FUNC_queue_remove(M_QUEUE, Q_REMOVE),        
		USE_FUNC_getCodeLength(M_HANDLER_STORE, GETCODELENGTH),

		USE_FUNC_getLibraryMask(M_HANDLER_STORE, GETLIBMASK),
		USE_FUNC_getOpcode(M_HANDLER_STORE, GETOPCODE),
		USE_FUNC_resetStacks(M_STACKS, RESETSTACKS),
		USE_FUNC_scheduler_submit(DVM_ENGINE_M, SUBMIT),

		PRVD_FUNC_analyzeVars(M_CONTEXT_SYNCH, ANALYZEVARS),
		PRVD_FUNC_clearAnalysis(M_CONTEXT_SYNCH, CLEARANALYSIS),
		PRVD_FUNC_initializeContext(M_CONTEXT_SYNCH, INITIALIZECONTEXT),
		PRVD_FUNC_yieldContext(M_CONTEXT_SYNCH, YIELDCONTEXT),

        PRVD_FUNC_haltContext(M_CONTEXT_SYNCH, HALTCONTEXT),
        PRVD_FUNC_resumeContext(M_CONTEXT_SYNCH, RESUMECONTEXT),
        PRVD_FUNC_isHeldBy(M_CONTEXT_SYNCH, ISHELDBY),
		PRVD_FUNC_synch_reset(M_CONTEXT_SYNCH, RESET),
    },
};

#ifdef LINK_KERNEL  
int8_t concurrency_init() 
{
	return ker_register_module(sos_get_header_address(mod_header));
}
#endif
#endif


static int8_t concurrency_handler(void *state, Message *msg)
{    
#ifndef _MODULE_    
	app_state *s = &sync_state;
#else           
	app_state *s = (app_state *) state;
#endif    
    switch (msg->type)
    {
	    case MSG_INIT:
	    {
			
		    s->pid = msg->did;		    
		    s->libraries = 0;
		    queue_initDL(s->q_init, &s->readyQueue );
			
		    DEBUG("CONTEXT SYNCH: Initialized\n");
		    return SOS_OK;
		}
		case MSG_ADD_LIBRARY:
		{
			MsgParam *param = (MsgParam *)msg->data;
			uint8_t lib_id = param->byte;
			uint8_t lib_bit = 0x1 << lib_id;
			s->libraries |= lib_bit;
			s->extlib_module[lib_id] = msg->sid;
			return SOS_OK;
		}
		case MSG_REMOVE_LIBRARY:
		{
			MsgParam *param = (MsgParam *)msg->data;
			uint8_t lib_id = param->byte;
			uint8_t lib_bit = 0x1 << lib_id;
			s->libraries ^= lib_bit;	//XOR
			s->extlib_module[lib_id] = 0;
			return SOS_OK;
		}
		case MSG_FINAL:
		{
			return SOS_OK;	
		}
		default:
			return SOS_OK;
	}
	return SOS_OK;
}

#ifndef _MODULE_    
void synch_reset() 
{
	app_state *s = &sync_state;
#else
static void synch_reset(func_cb_ptr p) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
#endif
	queue_initDL(s->q_init, &s->readyQueue);
	locks_reset(s);
}
	
static inline uint8_t isRunnable(DvmContext* context, app_state *s) 
{ 
	int8_t i;
	uint8_t* neededLocks = (context->acquireSet);
	
	DEBUG("VM: Checking whether context %i runnable: ", (int)context->which);

	for (i = 0; i < DVM_LOCK_COUNT; i++) 
	{
		DEBUG_SHORT("%i,", (int)i); 
		if ((neededLocks[i / 8]) & (1 << (i % 8))) 
		{
			if (isLocked(i, s))
			{
				DEBUG_SHORT(" - no\n");
				return FALSE;
			}
		}
	}
	DEBUG_SHORT(" - yes\n");
	return TRUE;
}

static int8_t obtainLocks(DvmContext* caller, DvmContext* obtainer, app_state *s) 
{ 
	int i;
	uint8_t* neededLocks = (obtainer->acquireSet);
	DEBUG("VM: Attempting to obtain necessary locks for context %i: ", obtainer->which);
	for (i = 0; i < DVM_LOCK_COUNT; i++) 
	{
		DEBUG_SHORT("%i", (int)i);
		if ((neededLocks[i / 8]) & (1 << (i % 8))) 
		{
			DEBUG_SHORT("+"); 
			lock(obtainer, i, s);
		}
		DEBUG_SHORT(","); 
	}
	for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) 
	{
		obtainer->acquireSet[i] = 0;
	}
	DEBUG_SHORT("\n");
	return SOS_OK;		
}

static inline int8_t releaseLocks(DvmContext* caller, DvmContext* releaser, app_state *s) 
{
	int i;
	uint8_t* lockSet = (releaser->releaseSet);
	
	DEBUG("VM: Attempting to release specified locks for context %i.\n", releaser->which);
	for (i = 0; i < DVM_LOCK_COUNT; i++) 
	{
		if ((lockSet[i / 8]) & (1 << (i % 8))) 
		{
			unlock(releaser, i, s);
		}
	}
	for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) 
	{
		releaser->releaseSet[i] = 0;
	}
	return SOS_OK;		
}

static int8_t releaseAllLocks(DvmContext* caller, DvmContext* releaser, app_state *s) 
{
	int i;
	uint8_t* lockSet = (releaser->heldSet);
	
	DEBUG("VM: Attempting to release all locks for context %i.\n", releaser->which);
	for (i = 0; i < DVM_LOCK_COUNT; i++) 
	{
		if ((lockSet[i / 8]) & (1 << (i % 8))) 
		{
			unlock(releaser, i, s);
		}
	}
	for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) 
	{
		releaser->releaseSet[i] = 0;
	}
	return SOS_OK;
}

#ifndef _MODULE_    
void analyzeVars(DvmCapsuleID id) 
{
	app_state *s = &sync_state;
#else
static void analyzeVars(func_cb_ptr p, DvmCapsuleID id) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
#endif
	uint16_t i;
	uint16_t handlerLen;
	DvmOpcode instr = 0;
	uint8_t opcode_mod = 0, libMask = 0;
	
	DEBUG("VM: Analyzing capsule vars for handler %d: \n", (int)(id));
	for (i = 0; i < ((DVM_LOCK_COUNT + 7) / 8); i++) 
	{
		s->usedVars[id][i] = 0;
	}

	handlerLen = getCodeLengthDL(s->getCodeLength, id);
	
	DEBUG("CONTEXTSYNCH: Handler length for handler %d is %d.\n",id,handlerLen);
	
	libMask = getLibraryMaskDL(s->get_libmask, id);
	if ((s->libraries & libMask) != libMask) {			//library missing
		DEBUG("CONTEXT_SYNCH: Library required 0x%x. Missing for handler %d.\n",libMask,id);
		return;
	}
	
	i = 0;
	while (i < handlerLen) {
		int16_t locknum;
		instr = getOpcodeDL(s->get_opcode, id, i);
		if (!(instr & LIB_ID_BIT)) {
			opcode_mod = M_BASIC_LIB;
			locknum = lockNumDL(s->lockNum, instr);
			i += bytelengthDL(s->bytelength, instr);
		} else {
			opcode_mod = (instr & BASIC_LIB_OP_MASK) >> EXT_LIB_OP_SHIFT;
			opcode_mod = s->extlib_module[opcode_mod];
			ker_fntable_subscribe(s->pid, opcode_mod, LOCKNUM, 1);
			locknum = SOS_CALL(s->lockNum, func_i16u8_t, instr);
			ker_fntable_subscribe(s->pid, opcode_mod, BYTELENGTH, 0);
			i += SOS_CALL(s->bytelength, func_u8u8_t, instr);
		}
		if (locknum >= 0) 
			s->usedVars[id][locknum / 8] |= (1 << (locknum % 8)); 
	} 
}

#ifndef _MODULE_    
void clearAnalysis(DvmCapsuleID id) 
{
   	app_state *s = &sync_state;
#else
static void clearAnalysis(func_cb_ptr p, DvmCapsuleID id) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
#endif
	memset(s->usedVars, 0, DVM_CAPSULE_NUM * ((DVM_LOCK_COUNT + 7)/8));
}

#ifndef _MODULE_    
void initializeContext(DvmContext *context) 
{
   	app_state *s = &sync_state;
#else
static void initializeContext(func_cb_ptr p, DvmContext *context) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
#endif
	int i;
	for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) 
	{
		context->heldSet[i] = 0;
		context->releaseSet[i] = 0;
	}
	memcpy(context->acquireSet, s->usedVars[context->which], (DVM_LOCK_COUNT + 7) / 8);
	context->pc = 0;
	resetStacksDL(s->reset_stack, (DvmState *)context);
	if (context->queue) 
	{
	    queue_removeDL(s->q_remove, context, context->queue, context);
	}
	context->state = DVM_STATE_HALT;
	context->num_executed = 0;
}

#ifndef _MODULE_
void yieldContext(DvmContext* context) 
{
   	app_state *s = &sync_state;
#else
static void yieldContext(func_cb_ptr p, DvmContext* context) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
#endif
	DvmContext* start = NULL;
	DvmContext* current = NULL;
	
	DEBUG("VM (%i): Yielding.\n", (int)context->which);
	releaseLocks(context, context, s);
	
	if( !queue_emptyDL(s->q_empty, &s->readyQueue) )
	{
		do 
		{
			current = queue_dequeueDL(s->q_dequeue, context, &s->readyQueue);
#ifndef _MODULE_
			if (!resumeContext(context, current)) 
#else
			if (!resumeContext(0, context, current)) 
#endif
			{
				DEBUG("VM (%i): Context %i not runnable.\n", (int)context->which, (int)current->which);
				if (start == NULL) 
				{
					start = current;
				}
				else if (start == current) 
				{
					DEBUG("VM (%i): Looped on ready queue. End checks.\n", (int)context->which);
					break;
				}
			}
		}
		while ( !queue_emptyDL(s->q_empty,  &s->readyQueue ) );
	}
	else 
	{
		DEBUG("VM (%i): Ready queue empty.\n", (int)context->which);
	}
}

#ifndef _MODULE_
uint8_t resumeContext(DvmContext* caller, DvmContext* context) 
{
   	app_state *s = &sync_state;
#else
static uint8_t resumeContext(func_cb_ptr p, DvmContext* caller, DvmContext* context) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
#endif
	context->state = DVM_STATE_WAITING;
	if (isRunnable(context, s)) 
	{
		obtainLocks(caller, context, s);
		
		if(scheduler_submitDL(s->sched_submit, context) == SOS_OK)
		{
			DEBUG("VM (%i): Resumption of %i successful.\n", (int)caller->which, (int)context->which);
			context->num_executed = 0;
			return TRUE;
		}
		else 
		{
			DEBUG("VM (%i): Resumption of %i FAILED.\n", (int)caller->which, (int)context->which);
			return -EINVAL;
		}
	}
	else 
	{
		DEBUG("VM (%i): Resumption of %i unsuccessful, putting on the queue.\n", (int)caller->which, (int)context->which);
		
		queue_enqueueDL(s->q_enqueue, caller, &s->readyQueue, context);
		return FALSE;
	}	
}

#ifndef _MODULE_
void haltContext(DvmContext* context) 
{
	app_state *s = &sync_state;
#else
static void haltContext(func_cb_ptr p, DvmContext* context) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
#endif
	releaseAllLocks(context, context, s);
#ifndef _MODULE_
	yieldContext(context);
#else
	yieldContext(0, context);
#endif
	if (context->queue && context->state != DVM_STATE_HALT) 
	{
		queue_removeDL(s->q_remove, context, context->queue, context);
	}
	if (context->state != DVM_STATE_HALT) 
	{
		context->state = DVM_STATE_HALT;
	}
}

//MLocks functions
static inline void locks_reset(app_state *s) 
{
	uint16_t i;
	for( i = 0; i < DVM_LOCK_COUNT; i++) 
	{
		s->locks[i].holder = NULL;
	}
}

static inline int8_t lock(DvmContext* context, uint8_t lockNum, app_state *s) 
{
	s->locks[lockNum].holder = context;
	context->heldSet[lockNum / 8] |= (1 << (lockNum % 8));
	DEBUG("VM: Context %i locking lock %i\n", (int)context->which, (int)lockNum);
	return SOS_OK;
}

static inline int8_t unlock(DvmContext* context, uint8_t lockNum, app_state *s) 
{
	context->heldSet[lockNum / 8] &= ~(1 << (lockNum % 8));
	s->locks[lockNum].holder = 0;
	DEBUG("VM: Context %i unlocking lock %i\n", (int)context->which, (int)lockNum);
	return SOS_OK;
}
 
static inline uint8_t isLocked(uint8_t lockNum, app_state *s) 
{
	return (s->locks[lockNum].holder != 0);	
}

#ifndef _MODULE_    
uint8_t isHeldBy(uint8_t lockNum, DvmContext* context) 
{
	app_state *s = &sync_state;
#else
static uint8_t isHeldBy(func_cb_ptr p, uint8_t lockNum, DvmContext* context) 
{
	app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
#endif
	return (s->locks[lockNum].holder == context);
}

