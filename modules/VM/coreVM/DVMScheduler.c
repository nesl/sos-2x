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
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND THE UNIVERSITY OF
 * CALIFORNIA HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
 * UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 */
/*									tab:4
 *									
 *  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
 *  By downloading, copying, installing or using the software you
 *  agree to this license.  If you do not agree to this license, do
 *  not download, install, copy or use the software.
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
 * History:   April 2005		Port to sos
 *	      September 25, 2005	Port to sos-1.x
 */

/**
 * @author Rahul Balani
 */

#include <module.h>
//#include <fntable.h>
#include <led.h>

#ifdef LINK_KERNEL
//#define _MODULE_
#endif

//#include <sos_timer.h>
//#include <sos_sched.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMqueue.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMBasiclib.h>

typedef int8_t (*func_i8z_t)(func_cb_ptr cb, DvmContext* context, DvmOpcode instr);

typedef struct {
    func_cb_ptr ext_execute[4];
	func_cb_ptr reboot_ptr;
	func_cb_ptr execute_ptr;
	func_cb_ptr get_opcode;
	func_cb_ptr get_libmask;
	func_cb_ptr get_stateblock;
	func_cb_ptr q_init;
	func_cb_ptr q_empty;
	func_cb_ptr q_enqueue;
	func_cb_ptr q_dequeue;
	func_cb_ptr ctx_halt;
	func_cb_ptr synch_reset;
	func_cb_ptr clearAnalysis;
	func_cb_ptr analyzeVars;
    sos_pid_t pid;
    DvmQueue runQueue;
    DvmContext* runningContext;
    DvmErrorMsg errorMsg;
    uint8_t libraries;
    struct {
        uint8_t inErrorState  : 1;
        uint8_t errorFlipFlop : 1;
        uint8_t taskRunning   : 1;
        uint8_t halted        : 1;
    } /* __attribute__((packed)) */ flags;
} /*__attribute__((packed)) */ app_state;

static inline int8_t computeInstruction(app_state *s) ;
static int8_t executeContext(DvmContext* context, app_state *s) ;
static int8_t opdone(app_state *s) ;

static int8_t vm_scheduler(void *state, Message *msg) ;

#ifndef _MODULE_
static sos_module_t sched_module;
static app_state sched_state;

static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	mod_id : DVM_ENGINE_M,
	state_size : 0,
	num_sub_func : 4,
	num_prov_func : 1,
	num_timers : 1,
	module_handler : vm_scheduler,
	funct: {
		   {error_8, "czy2", M_EXT_LIB, EXECUTE},
		   {error_8, "czy2", M_EXT_LIB, EXECUTE},
		   {error_8, "czy2", M_EXT_LIB, EXECUTE},
		   {error_8, "czy2", M_EXT_LIB, EXECUTE},
		   {error, "czC2", DVM_ENGINE_M, ERROR},
	   },
};

int8_t VMscheduler_init()
{
	return sched_register_kernel_module(&sched_module, sos_get_header_address(mod_header), &sched_state);
}
#else
static void engineReboot(func_cb_ptr p); 
static int8_t scheduler_submit(func_cb_ptr p, DvmContext* context);
static int8_t error(func_cb_ptr p, DvmContext* context, uint8_t cause); 
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id         =   DVM_ENGINE_M,
	.code_id        =   ehtons(DVM_ENGINE_M),
	.platform_type  =   HW_TYPE,
	.processor_type =   MCU_TYPE,
	.state_size     =   sizeof(app_state),
	.num_sub_func   =    17,
	.num_prov_func  =    3,
	.num_timers     =    1,
	.module_handler =    vm_scheduler,
	.funct          = {
		{error_8, "czy2", M_EXT_LIB, EXECUTE},
		{error_8, "czy2", M_EXT_LIB, EXECUTE},
		{error_8, "czy2", M_EXT_LIB, EXECUTE},
		{error_8, "czy2", M_EXT_LIB, EXECUTE},

		USE_FUNC_rebooted(M_BASIC_LIB, REBOOTED),
		USE_FUNC_execute(M_BASIC_LIB, EXECUTE),
		USE_FUNC_getOpcode(M_HANDLER_STORE, GETOPCODE),
		USE_FUNC_getLibraryMask(M_HANDLER_STORE, GETLIBMASK),

		USE_FUNC_getStateBlock(M_HANDLER_STORE, GETSTATEBLOCK),
		USE_FUNC_queue_init(M_QUEUE, Q_INIT),
		USE_FUNC_queue_empty(M_QUEUE, Q_EMPTY),
		USE_FUNC_queue_enqueue(M_QUEUE, Q_ENQUEUE),

		USE_FUNC_queue_dequeue(M_QUEUE, Q_DEQUEUE),
		USE_FUNC_haltContext(M_CONTEXT_SYNCH, HALTCONTEXT),
		USE_FUNC_synch_reset(M_CONTEXT_SYNCH, RESET),
		USE_FUNC_clearAnalysis(M_CONTEXT_SYNCH, CLEARANALYSIS),

		USE_FUNC_analyzeVars(M_CONTEXT_SYNCH, ANALYZEVARS),
		PRVD_FUNC_engineReboot(DVM_ENGINE_M, REBOOT),
		PRVD_FUNC_scheduler_submit(DVM_ENGINE_M, SUBMIT),
		PRVD_FUNC_error(DVM_ENGINE_M, ERROR),
	},
};


#ifdef LINK_KERNEL
int8_t VMscheduler_init()
{
	    return ker_register_module(sos_get_header_address(mod_header));
}
#endif


#endif

static int8_t vm_scheduler(void *state, Message *msg)
{
#ifndef _MODULE_
	app_state *s = &sched_state;
#else
	app_state *s = (app_state*) state;
#endif
	switch (msg->type){
		case MSG_INIT: 
		{
			s->pid = msg->did;
			s->flags.inErrorState = FALSE;
			s->flags.halted = FALSE;
			s->flags.taskRunning = FALSE;
			s->flags.errorFlipFlop = 0;
			s->libraries = 1;	//Basic library is already there.
			s->runningContext = NULL;
			ker_timer_init(s->pid, ERROR_TIMER, TIMER_REPEAT);
			DEBUG("DVM ENGINE: Dvm initializing DONE.\n");
			engineReboot(0);
			return SOS_OK;
		}
		case MSG_FINAL:
		{
			DEBUG("DVM ENGINE: VM: Stopping.\n");
			return SOS_OK;
		}
		case RUN_TASK:
		{
			return opdone(s);
		}
		case HALT:
		{
			DEBUG("DVM ENGINE: DvmEngineM halted.\n");
			s->flags.halted = TRUE;
			return SOS_OK;
		}
		case RESUME:
		{
			s->flags.halted = FALSE;
			DEBUG("DVM ENGINE: DvmEngineM resumes....\n");
			if (!s->flags.taskRunning) {
				s->flags.taskRunning = TRUE;
				DEBUG("DVM ENGINE: RUN_TASK posted...\n");
				//post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0);
				opdone(s);
			}
			return SOS_OK;
		}
		case MSG_TIMER_TIMEOUT:
		{
			MsgParam *t = (MsgParam *)msg->data;

			if (t->byte == ERROR_TIMER) {
				DEBUG("DVM ENGINE: VM: ERROR\n");
				if (!s->flags.inErrorState) {
					ker_timer_stop(s->pid, ERROR_TIMER);
					return -EINVAL;
				}
				if (s->flags.errorFlipFlop) {
					post_uart(DVM_ENGINE_M, s->pid, DVM_ERROR_MSG, sizeof(DvmErrorMsg), &(s->errorMsg), 0, UART_ADDRESS);
				} else {
					// TODO: Need to figure out what this is...
					//post_net(M_VIRUS, s->pid, DVM_ERROR_MSG, sizeof(DvmErrorMsg), &(s->errorMsg), 0, BCAST_ADDRESS);
				}
				s->flags.errorFlipFlop = !s->flags.errorFlipFlop;
				return SOS_OK;
			}
			break;
		}
		case MSG_ADD_LIBRARY:
		{
			MsgParam *param = (MsgParam *)msg->data;
			uint8_t lib_id = param->byte;
			uint8_t lib_bit = 0x1 << lib_id;
			s->libraries |= lib_bit;
			ker_fntable_subscribe(s->pid, msg->sid, EXECUTE, lib_id); 
			DEBUG("DVM_ENGINE : Adding library id %d bit %d. So libraries become %02x.\n",lib_id,lib_bit,s->libraries);
			engineReboot(0);
			return SOS_OK;
		}
		case MSG_REMOVE_LIBRARY:
		{
			MsgParam *param = (MsgParam *)msg->data;
			uint8_t lib_id = param->byte;
			uint8_t lib_bit = 0x1 << lib_id;
			s->libraries ^= lib_bit;	//XOR
			DEBUG("DVM_ENGINE : Removing library id %d bit %d. So libraries become %02x.\n",lib_id,lib_bit,s->libraries);
			engineReboot(0);
			return SOS_OK;
		}
#ifdef PC_PLATFORM
		case MSG_FROM_USER:
		{
			DEBUG("DVM ENGINE: Removing Math library\n");
			ker_deregister_module(M_MATH_LIB);
		}
#endif
		default:
		break;
	}
	return SOS_OK;
}

static int8_t opdone(app_state *s) 
{
	DEBUG("DVM ENGINE: opdone starts running...\n");
	if (s->flags.halted == TRUE) {
		DEBUG("DVM ENGINE: Halted, don't run.\n");
		s->flags.taskRunning = FALSE;
		return SOS_OK;
	}

	if ((s->runningContext != NULL) && 
		(s->runningContext->state == DVM_STATE_RUN) && 
		(s->runningContext->num_executed >= DVM_CPU_SLICE)) { 
		DEBUG("DVM ENGINE: Slice for context %i expired, re-enqueue.\n", (int)s->runningContext->which);
		s->runningContext->state = DVM_STATE_READY;
		queue_enqueueDL(s->q_enqueue, s->runningContext, &(s->runQueue), s->runningContext);
		s->runningContext = NULL;
	}

	if (s->runningContext == NULL && !s->flags.inErrorState && !queue_emptyDL(s->q_empty, &s->runQueue) ) 
	{
		DEBUG("DVM ENGINE: Dequeue a new context.\n");
		s->runningContext = queue_dequeueDL(s->q_dequeue, NULL, &s->runQueue );
		s->runningContext->num_executed = 0;
		s->runningContext->state = DVM_STATE_RUN;
	}
	if (s->runningContext != NULL) {
		if (s->runningContext->state != DVM_STATE_RUN) {
			//Context was halted by library
			//Release the CPU
			s->runningContext = NULL;
			if (post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0) != SOS_OK) {
				s->flags.taskRunning = FALSE;
			}
		} else {
			computeInstruction(s);
		}
	} else {
		DEBUG("DVM ENGINE: Running_context was NULL\n");
		s->flags.taskRunning = FALSE;
	}
	DEBUG("DVM ENGINE: opdone call over\n");
	return SOS_OK;
}	

#ifndef _MODULE_
void engineReboot(void) 
{
	app_state *s = &sched_state;
#else
static void engineReboot(func_cb_ptr p) 
{
	app_state *s = (app_state*)ker_get_module_state(DVM_ENGINE_M);
#endif
	DvmCapsuleID id;

	DEBUG("DVM ENGINE: VM: Dvm rebooting.\n");
	s->runningContext = NULL;

	queue_initDL(s->q_init,  &s->runQueue );

	synch_resetDL(s->synch_reset);

	for (id = 0; id < DVM_CAPSULE_NUM; id++) {
		clearAnalysisDL(s->clearAnalysis, id);
	}
	DEBUG("DVM ENGINE: VM: Analyzing lock sets.\n");
	for (id = 0; id < DVM_CAPSULE_NUM; id++) {
		analyzeVarsDL(s->analyzeVars,id);
	}
	s->flags.inErrorState = FALSE;
	s->flags.halted = FALSE;

	DEBUG("DVM ENGINE: VM: Signaling reboot to libraries.\n");    
	rebootedDL(s->reboot_ptr);

	ker_led(LED_RED_OFF);
	ker_led(LED_GREEN_OFF);
	ker_led(LED_YELLOW_OFF);
}

#ifndef _MODULE_
int8_t scheduler_submit(DvmContext* context) 
{
	app_state *s = &sched_state;
#else
static int8_t scheduler_submit(func_cb_ptr p, DvmContext* context) 
{
	app_state *s = (app_state*)ker_get_module_state(DVM_ENGINE_M);
#endif
    DEBUG("DVM_ENGINE: VM: Context %i submitted to run.\n", (int)context->which);
    context->state = DVM_STATE_READY;
    return executeContext(context, s);
}

#ifndef _MODULE_
int8_t error(DvmContext* context, uint8_t cause) 
{
	app_state *s = &sched_state;
#else
static int8_t error(func_cb_ptr p, DvmContext* context, uint8_t cause) 
{
	app_state *s = (app_state*)ker_get_module_state(DVM_ENGINE_M);
#endif
    
    s->flags.inErrorState = TRUE;
    DEBUG("DVM_ENGINE: VM: Entering ERROR state. Context: %i, cause %i\n", (int)context->which, (int)cause);
    ker_timer_start(s->pid, ERROR_TIMER, 1000);
    if (context != NULL) {
      s->errorMsg.context = context->which;
      s->errorMsg.reason = cause;
      s->errorMsg.capsule = context->which;
      s->errorMsg.instruction = context->pc - 1;
      s->errorMsg.me = ker_id();
      context->state = DVM_STATE_HALT;
    }
    else {
      s->errorMsg.context = DVM_CAPSULE_INVALID;
      s->errorMsg.reason = cause;
      s->errorMsg.capsule = DVM_CAPSULE_INVALID;
      s->errorMsg.instruction = 255;
    }
    return SOS_OK;
}

static inline int8_t computeInstruction(app_state *s) {
	int8_t r;
	DvmContext *context = s->runningContext;
	DvmOpcode instr = getOpcodeDL(s->get_opcode, context->which, context->pc);
	if (context->state != DVM_STATE_RUN) {
		return -EINVAL;
	}
	
	DEBUG("DVM_ENGINE: Inside compute_instruction \n");
	if (context->pc == 0) {
		uint8_t libMask = getLibraryMaskDL(s->get_libmask, context->which);
		if ((s->libraries & libMask) != libMask) {
			DEBUG("DVM_ENGINE: Library missing for context %d. Halting execution.\n", context->which);
			haltContextDL(s->ctx_halt, context);
			s->runningContext = NULL;
			post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0);
			return -EINVAL;
		}
	}

	while ((context->state == DVM_STATE_RUN) && (context->num_executed < DVM_CPU_SLICE)) {
		if (instr & LIB_ID_BIT) {	//Extension Library
			uint8_t library_mod = (instr ^ LIB_ID_BIT) >> EXT_LIB_OP_SHIFT;
			__asm __volatile("st_call1:");
			r = SOS_CALL(s->ext_execute[library_mod], func_i8z_t, s->runningContext, instr);
			__asm __volatile("en_call1:");
			context->num_executed++;
			//if (r < 0) signal error, halt context, break
		} else {			//Basic Library
			DEBUG("DVM_ENGINE: Library being called is %d\n", M_BASIC_LIB);
			r = executeDL(s->execute_ptr, getStateBlockDL(s->get_stateblock, context->which));
			//if (r < 0) signal error, halt context, break
		}
		instr = getOpcodeDL(s->get_opcode, context->which, context->pc);
	}
	if (context->num_executed >= DVM_CPU_SLICE)
		post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0);
	else if (context->state != DVM_STATE_RUN)
		return opdone(s);
	return SOS_OK;
}
  
static int8_t executeContext(DvmContext* context, app_state *s) {
    if (context->state != DVM_STATE_READY) {
      DEBUG("DVM_ENGINE: Failed to submit context %i: not in READY state.\n", (int)context->which);
      return -EINVAL;
    }
    
    queue_enqueueDL(s->q_enqueue, context, &s->runQueue, context);

    if (!s->flags.taskRunning) {
        s->flags.taskRunning = TRUE;
        DEBUG("DVM_ENGINE: Executing context.. Posting run task.\n");
        post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0); 
    }
    return SOS_OK;
}

