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

#ifdef LINK_KERNEL
//#define _MODULE_
#endif

//#include <fntable.h>
//#include <sos_sched.h>
#include <VM/DVMStacks.h>
#include <VM/DVMScheduler.h>

typedef struct {
	func_cb_ptr error;
} app_state;

#ifndef _MODULE_
//static app_state stack_state;
int8_t stacks_init()
{
	return SOS_OK;
}
#else
static int8_t stacks_handler(void *state, Message *msg) ;
static int8_t resetStacks(func_cb_ptr p, DvmState *eventState);
static int8_t pushValue(func_cb_ptr p, DvmState *eventState, int16_t val, uint8_t type); 
static int8_t pushBuffer(func_cb_ptr p, DvmState *eventState, DvmDataBuffer* buffer); 
static int8_t pushOperand(func_cb_ptr p, DvmState *eventState, DvmStackVariable* var); 
static DvmStackVariable* popOperand(func_cb_ptr p, DvmState *eventState); 
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
    .mod_id         = M_STACKS,
	.code_id        = ehtons(M_STACKS),
    .state_size     = sizeof(app_state), 
	.platform_type  = HW_TYPE,                                  
	.processor_type = MCU_TYPE,
    .num_sub_func   = 1,
    .num_prov_func  = 5,
    .module_handler = stacks_handler,
    .funct          = {
		[0] = USE_FUNC_error(DVM_ENGINE_M, ERROR),
		[1] = PRVD_FUNC_pushValue(M_STACKS, PUSHVALUE),        
		[2] = PRVD_FUNC_pushOperand(M_STACKS, PUSHOPERAND),    
		[3] = PRVD_FUNC_pushBuffer(M_STACKS, PUSHBUFFER),      
		[4] = PRVD_FUNC_popOperand(M_STACKS, POPOPERAND),      
		[5] = PRVD_FUNC_resetStacks(M_STACKS, RESETSTACKS),
    },
};

static int8_t stacks_handler(void *state, Message *msg){
	return SOS_OK;
}

#ifdef LINK_KERNEL
int8_t stacks_init()
{
	    return ker_register_module(sos_get_header_address(mod_header));
}
#endif


#endif


#ifndef _MODULE_
int8_t resetStacks(DvmState *eventState) 
{
#else
static int8_t resetStacks(func_cb_ptr p, DvmState *eventState) 
{
#endif
	eventState->stack.sp = 0;
	return SOS_OK;
}

#ifndef _MODULE_
int8_t pushValue(DvmState *eventState, int16_t val, uint8_t type) 
{
	//app_state *s = &stack_state;
#else
static int8_t pushValue(func_cb_ptr p, DvmState *eventState, int16_t val, uint8_t type) 
{
	//app_state *s = (app_state *) ker_get_module_state(M_STACKS);
#endif
	DvmContext* context = &(eventState->context);

	if (eventState->stack.sp >= DVM_OPDEPTH) {
	    DEBUG("VM: Tried to push value off end of stack.\n");
		errorDL(s->error, context, DVM_ERROR_STACK_OVERFLOW);
	    return -EINVAL;
	}
	else {
	    uint8_t sIndex = eventState->stack.sp;
	    eventState->stack.stack[(int)sIndex].type = type;
	    eventState->stack.stack[(int)sIndex].value.var = val;
	    eventState->stack.sp++;
	    return SOS_OK;
	}
}

#ifndef _MODULE_
int8_t pushBuffer(DvmState *eventState, DvmDataBuffer* buffer) 
{
	//app_state *s = &stack_state;
#else
static int8_t pushBuffer(func_cb_ptr p, DvmState *eventState, DvmDataBuffer* buffer) 
{
	//app_state *s = (app_state *) ker_get_module_state(M_STACKS);
#endif
	DvmContext* context = &(eventState->context);

	if (eventState->stack.sp >= DVM_OPDEPTH) {
		DEBUG("VM: Tried to push value off end of stack.\n");
		errorDL(s->error, context, DVM_ERROR_STACK_OVERFLOW);
		return -EINVAL;
	}
	else {
		uint8_t sIndex = eventState->stack.sp;
		eventState->stack.stack[(int)sIndex].type = DVM_TYPE_BUFFER;
		eventState->stack.stack[(int)sIndex].buffer.var = buffer;
		eventState->stack.sp++;
		return SOS_OK;
	}

	return SOS_OK;
}


#ifndef _MODULE_
int8_t pushOperand(DvmState *eventState, DvmStackVariable* var) 
{
	//app_state *s = &stack_state;
#else
static int8_t pushOperand(func_cb_ptr p, DvmState *eventState, DvmStackVariable* var) 
{
	//app_state *s = (app_state *) ker_get_module_state(M_STACKS);
#endif
	DvmContext* context = &(eventState->context);

	if (eventState->stack.sp >= DVM_OPDEPTH) {
		DEBUG("VM: Tried to push value off end of stack.\n");
		errorDL(s->error, context, DVM_ERROR_STACK_OVERFLOW);
		return -EINVAL;
	}
	else {
		uint8_t sIndex = eventState->stack.sp;
		eventState->stack.stack[(int)sIndex] = *var;
		eventState->stack.sp++;
		return SOS_OK;
	}

	return SOS_OK;
}

#ifndef _MODULE_
DvmStackVariable* popOperand(DvmState *eventState) 
{
	//app_state *s = &stack_state;
#else
static DvmStackVariable* popOperand(func_cb_ptr p, DvmState *eventState) 
{
	//app_state *s = (app_state *) ker_get_module_state(M_STACKS);
#endif
	DvmStackVariable* var;
	DvmContext* context = &(eventState->context);

	if (eventState->stack.sp == 0) {
		DEBUG("VM: Tried to pop off end of stack.\n");
		eventState->stack.stack[0].type = DVM_TYPE_NONE;
		errorDL(s->error, context, DVM_ERROR_STACK_UNDERFLOW);
		return &(eventState->stack.stack[0]);
	}
	else {
		uint8_t sIndex;
		eventState->stack.sp--;
		sIndex = eventState->stack.sp;
		var = &(eventState->stack.stack[(int)sIndex]);
		return var;
	}
	return NULL;
}

