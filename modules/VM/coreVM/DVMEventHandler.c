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
/*
#include <message.h>
#include <sos_timer.h>
#include <fntable.h>
#include <sos_sched.h>
#include <led.h>
*/

#ifdef LINK_KERNEL  
//#define _MODULE_
#endif

#include <VM/DVMEventHandler.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMResourceManager.h>

typedef struct {
	func_cb_ptr push_value;
	func_cb_ptr push_operand;
	func_cb_ptr push_buffer;
	func_cb_ptr pop_operand;
	func_cb_ptr reboot;
	func_cb_ptr mem_free;
	func_cb_ptr ctx_init;
	func_cb_ptr ctx_resume;
	func_cb_ptr ctx_halt;
	func_cb_ptr analyzeVars;
	sos_pid_t pid;
	DvmState* stateBlock[DVM_CAPSULE_NUM];
} app_state;

static inline void rebootContexts(app_state *s) ;

static int8_t event_handler(void *state, Message *msg) ;


#ifndef _MODULE_
static sos_module_t event_module;
static app_state event_state;

static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id        = M_HANDLER_STORE,
	.code_id       = ehtons(M_HANDLER_STORE),
	.platform_type  = HW_TYPE,    
	.processor_type = MCU_TYPE,
	.state_size    = 0,
	.num_sub_func  = 0,
	.num_prov_func = 0,
	.num_timers    = 8,
	.module_handler = event_handler,
};

int8_t eventhandler_init()
{
	return sched_register_kernel_module(&event_module, sos_get_header_address(mod_header), NULL);
}

#else
static int8_t initEventHandler(func_cb_ptr p, DvmState *eventState, uint8_t capsuleID);
static DvmCapsuleLength getCodeLength(func_cb_ptr p, uint8_t id); 
static uint8_t getLibraryMask(func_cb_ptr p, uint8_t id);
static DvmOpcode getOpcode(func_cb_ptr p, uint8_t id, uint16_t which);
static DvmState *getStateBlock (func_cb_ptr p, uint8_t id); 
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id        = M_HANDLER_STORE,
	.code_id       = ehtons(M_HANDLER_STORE),
	.platform_type  = HW_TYPE,    
	.processor_type = MCU_TYPE,
	.state_size    = 0,
	.num_sub_func  = 9,
	.num_prov_func = 5,
	.num_timers    = 8,
	.module_handler = event_handler,
	.funct          = {
		USE_FUNC_pushValue(M_STACKS, PUSHVALUE),        
		USE_FUNC_pushOperand(M_STACKS, PUSHOPERAND),    
		USE_FUNC_pushBuffer(M_STACKS, PUSHBUFFER),      
		USE_FUNC_popOperand(M_STACKS, POPOPERAND),      
		USE_FUNC_engineReboot(DVM_ENGINE_M, REBOOT),
		USE_FUNC_mem_free(M_RESOURCE_MANAGER, FREEMEM),
		USE_FUNC_initializeContext(M_CONTEXT_SYNCH, INITIALIZECONTEXT),
		USE_FUNC_resumeContext(M_CONTEXT_SYNCH, RESUMECONTEXT),
		USE_FUNC_haltContext(M_CONTEXT_SYNCH, HALTCONTEXT),
		USE_FUNC_analyzeVars(M_CONTEXT_SYNCH, ANALYZEVARS),
		PRVD_FUNC_initEventHandler(M_HANDLER_STORE, INITIALIZEHANDLER),
		PRVD_FUNC_getCodeLength(M_HANDLER_STORE, GETCODELENGTH),
		PRVD_FUNC_getOpcode(M_HANDLER_STORE, GETOPCODE),
		PRVD_FUNC_getLibraryMask(M_HANDLER_STORE, GETLIBMASK),
		PRVD_FUNC_getStateBlock(M_HANDLER_STORE, GETSTATEBLOCK),
	},
};
#ifdef LINK_KERNEL  
int8_t eventhandler_init()
{
	return ker_register_module(sos_get_header_address(mod_header));
}
#endif

#endif

static int8_t event_handler(void *state, Message *msg) {
#ifndef _MODULE_                                              
	app_state *s = &event_state;
#else                                                         
	app_state *s = (app_state *) state;                       
#endif  
	switch (msg->type) {
		case MSG_INIT: 
			{
				s->pid = msg->did;
				memset(s->stateBlock, 0, DVM_CAPSULE_NUM*sizeof(void*));
				DEBUG("HANDLER STORE: Initialized\n");
			break;
		}
		case MSG_FINAL: 
		{
			uint8_t i = 0;
			for (; i < DVM_CAPSULE_NUM; i++)
				mem_freeDL(s->mem_free, i);
			break;
		}
		case MSG_TIMER_TIMEOUT:
		{
			MsgParam *timerID = (MsgParam *)msg->data;
			DEBUG("EVENT HANDLER: TIMER %d EXPIRED\n", timerID->byte);
			if ((s->stateBlock[timerID->byte] != NULL) && (s->stateBlock[timerID->byte]->context.moduleID == TIMER_PID)
				&& (s->stateBlock[timerID->byte]->context.type == MSG_TIMER_TIMEOUT)) {
				initializeContextDL(s->ctx_init, &s->stateBlock[timerID->byte]->context);
				resumeContextDL(s->ctx_resume, &s->stateBlock[timerID->byte]->context, &s->stateBlock[timerID->byte]->context);
			}
			break;
		}
		default:
		{
			__asm __volatile("st_sos2:");
			uint8_t i;
			for (i = 0; i < DVM_CAPSULE_NUM; i++) {
				if ((s->stateBlock[i] != NULL) && (s->stateBlock[i]->context.moduleID == msg->sid)
					&& (s->stateBlock[i]->context.type == msg->type)) {
					if (s->stateBlock[i]->context.state == DVM_STATE_HALT) {
						DvmStackVariable *stackArg;
						initializeContextDL(s->ctx_init, &s->stateBlock[i]->context);
						stackArg = (DvmStackVariable *)ker_msg_take_data(s->pid, msg);
						if (stackArg != NULL)
							pushOperandDL(s->push_operand, s->stateBlock[i], stackArg);
							resumeContextDL(s->ctx_resume, &s->stateBlock[i]->context, &s->stateBlock[i]->context);
					}
				}
			}
			break;
		}
	}
    
	return SOS_OK;
}

#ifndef _MODULE_
int8_t initEventHandler(DvmState *eventState, uint8_t capsuleID) 
{
	app_state *s = &event_state;
#else
static int8_t initEventHandler(func_cb_ptr p, DvmState *eventState, uint8_t capsuleID) 
{
	app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
#endif

	if (capsuleID >= DVM_CAPSULE_NUM) {return -EINVAL;}
	s->stateBlock[capsuleID] = eventState;

	analyzeVarsDL(s->analyzeVars, capsuleID);
	initializeContextDL(s->ctx_init, &s->stateBlock[capsuleID]->context);

	{
#ifdef PC_PLATFORM
		int i;
		DEBUG("EventHandler: Installing capsule %d:\n\t", (int)capsuleID);
		for (i = 0; i < s->stateBlock[capsuleID]->context.dataSize; i++) {
			DEBUG_SHORT("[%hhx]", getOpcode(capsuleID, i));
		}
		DEBUG_SHORT("\n");
#endif
	}

	engineRebootDL(s->reboot);
	rebootContexts(s);
	
	if (s->stateBlock[capsuleID]->context.moduleID == TIMER_PID) {
		ker_timer_init(s->pid, capsuleID, TIMER_REPEAT);
		DEBUG("VM (%d): TIMER INIT\n", capsuleID);
	}

	if (s->stateBlock[DVM_CAPSULE_REBOOT] != NULL) {
		initializeContextDL(s->ctx_init, &(s->stateBlock[DVM_CAPSULE_REBOOT]->context));
		resumeContextDL(s->ctx_resume, &s->stateBlock[DVM_CAPSULE_REBOOT]->context, &s->stateBlock[DVM_CAPSULE_REBOOT]->context);
	}

	return SOS_OK;
}


#ifndef _MODULE_
DvmCapsuleLength getCodeLength(uint8_t id) 
{
	app_state *s = &event_state;
#else
static DvmCapsuleLength getCodeLength(func_cb_ptr p, uint8_t id) 
{
	app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
#endif
	if (s->stateBlock[id] != NULL)
		return s->stateBlock[id]->context.dataSize;
	else
		return 0;
}

#ifndef _MODULE_
uint8_t getLibraryMask(uint8_t id) 
{
	app_state *s = &event_state;
#else
static uint8_t getLibraryMask(func_cb_ptr p, uint8_t id) 
{
	app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
#endif
	if (s->stateBlock[id] != NULL)
		return s->stateBlock[id]->context.libraryMask;
	else
		return 0;
}
	
#ifndef _MODULE_
DvmOpcode getOpcode(uint8_t id, uint16_t which) 
{
	app_state *s = &event_state;
#else
static DvmOpcode getOpcode(func_cb_ptr p, uint8_t id, uint16_t which) 
{
	app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
#endif
	if (s->stateBlock[id] != NULL) {
		DvmState *ds = s->stateBlock[id];
		DvmOpcode op;
		// get the opcode at index "which" from codemem and return it.
		// handler for codemem is in s->stateBlock[id]->script
		if(ker_codemem_read(ds->cm, M_HANDLER_STORE,
					&(op), sizeof(op), offsetof(DvmScript, data) + which) == SOS_OK) {
			DEBUG("Event Handler: getOpcode. correct place.\n");
			return op;
		}
		return OP_HALT;
	} else {
		return OP_HALT;
	}
}

#ifndef _MODULE_
DvmState *getStateBlock (uint8_t id) 
{
	app_state *s = &event_state;
#else
static DvmState *getStateBlock (func_cb_ptr p, uint8_t id) 
{
	app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
#endif
	if (s->stateBlock[id] != NULL) {
		return s->stateBlock[id];
	}
	return NULL;
}
	
static inline void rebootContexts(app_state *s)
{
	int i, j;
	for (i = 0; i < DVM_CAPSULE_NUM; i++) {
		if (s->stateBlock[i] != NULL) {
			for (j = 0; j < DVM_NUM_LOCAL_VARS; j++) {
				s->stateBlock[i]->vars[j].type = DVM_TYPE_INTEGER;
				s->stateBlock[i]->vars[j].value.var = 0;
			}
			if (s->stateBlock[i]->context.state != DVM_STATE_HALT)
				haltContextDL(s->ctx_halt, &s->stateBlock[i]->context);
		}
	}
}

