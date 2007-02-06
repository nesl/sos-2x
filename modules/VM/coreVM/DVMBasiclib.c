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
/*
 * Authors:   Rahul Balani
 * History:   April 2005		Creation
 *	      September 30, 2005	Port to sos-1.x
 */

/**
 * @author Rahul Balani
 */
#include <module.h>
#include <fntable.h>
#include <led.h>
#include <random.h>
#include <sensor.h>
#include <sos_timer.h>
#include <sos_sched.h>

#ifdef LINK_KERNEL
//#define _MODULE_
#endif
#include <VM/DVMBasiclib.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMStacks.h>
#include <VM/DVMqueue.h>
#include <VM/DVMBuffer.h>
#include <VM/DVMMathlib.h>

//
// For NOP instruction
//
#define PER_DELAY                1000L
#define COMPUTATION_DELAY        100L

typedef struct {
	uint8_t fnid;
	DvmStackVariable *argBuf;
} post_msg_type;

typedef struct 
{
	func_cb_ptr execute_syncall;
	func_cb_ptr push_value;
	func_cb_ptr push_operand;
	func_cb_ptr push_buffer;
	func_cb_ptr pop_operand;
	func_cb_ptr get_opcode;
	func_cb_ptr buffer_execute;
	func_cb_ptr q_init;
	func_cb_ptr q_empty;
	func_cb_ptr q_enqueue;
	func_cb_ptr q_dequeue;
	func_cb_ptr mathlib_execute;
	func_cb_ptr ctx_resume;
	func_cb_ptr ctx_yield;
	func_cb_ptr ctx_halt;

	sos_pid_t pid;
	uint8_t busy;				//for GET_DATA
	DvmContext *executing;		//for GET_DATA
	DvmQueue getDataWaitQueue;	//for GET_DATA
	DvmContext *nop_executing;		//for NOP
	uint16_t delay_cnt;             //for NOP
	DvmDataBuffer buffers[DVM_NUM_BUFS];
	DvmStackVariable shared_vars[DVM_NUM_SHARED_VARS];
} app_state;

typedef int8_t (*func_i8u8zu8z_t)(func_cb_ptr cb, uint8_t fnid, DvmStackVariable *arg, uint8_t size, DvmStackVariable *res);

static int8_t basic_library(void *state, Message *msg);

static int8_t tryget(DvmContext *context, uint8_t sensor_type, app_state *s);
static inline int8_t post_op(DvmState *eventState, app_state *s); 
static inline int8_t call_op(DvmState *eventState, app_state *s); 
static inline void led_op(uint16_t val);

#ifndef _MODULE_
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id			= M_BASIC_LIB,
	.code_id		= ehtons(M_BASIC_LIB),
	.platform_type  = HW_TYPE,
	.processor_type = MCU_TYPE,  
    .state_size		= 0,
	.num_timers     = 0,
    .num_sub_func	= 1,
    .num_prov_func	= 0,
    .module_handler = basic_library,
    .funct			= {
        {error_8, "cCz4", DVM_ENGINE_M, EXECUTE_SYNCALL},
    },
};

static sos_module_t basiclib_module;
static app_state basiclib_state;
int8_t basiclib_init()
{
	return sched_register_kernel_module(&basiclib_module, sos_get_header_address(mod_header), &basiclib_state);
}

#else
static int8_t execute( func_cb_ptr p, DvmState *eventState);
static void rebooted( func_cb_ptr p );
static uint8_t bytelength( func_cb_ptr p, uint8_t opcode);
static int16_t lockNum( func_cb_ptr p, uint8_t instr);

static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id			= M_BASIC_LIB,
	.code_id		= ehtons(M_BASIC_LIB),
	.platform_type  = HW_TYPE,
	.processor_type = MCU_TYPE,  
    .state_size		= 0,
	.num_timers     = 0,
    .num_sub_func	= 15,
    .num_prov_func	= 4,
    .module_handler = basic_library,
    .funct			= {
		[0] = {error_8, "cCz4", DVM_ENGINE_M, EXECUTE_SYNCALL},
		[1] = USE_FUNC_pushValue(M_STACKS, PUSHVALUE),
		[2] = USE_FUNC_pushOperand(M_STACKS, PUSHOPERAND),
		[3] = USE_FUNC_pushBuffer(M_STACKS, PUSHBUFFER),
		[4] = USE_FUNC_popOperand(M_STACKS, POPOPERAND),
		[5] = USE_FUNC_getOpcode(M_HANDLER_STORE, GETOPCODE),
		[6] = USE_FUNC_buffer_execute(M_BUFFER, BUFFER_EXECUTE),
		[7] =      USE_FUNC_queue_init(M_QUEUE, Q_INIT),          
		[8] =     USE_FUNC_queue_empty(M_QUEUE, Q_EMPTY),        
		[9] =     USE_FUNC_queue_enqueue(M_QUEUE, Q_ENQUEUE),    
		[10] =     USE_FUNC_queue_dequeue(M_QUEUE, Q_DEQUEUE),    
        [11] = USE_FUNC_mathlib_execute(M_MATHLIB, MATHLIB_EXECUTE),
		[12] = USE_FUNC_resumeContext(M_CONTEXT_SYNCH, RESUMECONTEXT),
		[13] = USE_FUNC_yieldContext(M_CONTEXT_SYNCH, YIELDCONTEXT),
		[14] = USE_FUNC_haltContext(M_CONTEXT_SYNCH, HALTCONTEXT),
		[15] = PRVD_FUNC_rebooted(M_BASIC_LIB, REBOOTED),
		[16] = PRVD_FUNC_execute( M_BASIC_LIB, EXECUTE),
		[17] = PRVD_FUNC_lockNum(M_BASIC_LIB, LOCKNUM),
		[18] = PRVD_FUNC_bytelength(M_BASIC_LIB, BYTELENGTH),
    },
};

#ifdef LINK_KERNEL
int8_t basiclib_init()
{
	return ker_register_module(sos_get_header_address(mod_header));
}
#endif

#endif

static int8_t basic_library(void *state, Message *msg)
{    
#ifndef _MODULE_
	app_state *s = &basiclib_state;
#else
	app_state *s = (app_state *) state;
#endif
	switch (msg->type)
	{
		case MSG_INIT:
		{	
			//basiclib_state.pid = msg->did;			
			s->pid = msg->did;			
			//basiclib_state.busy = 0;
			s->busy = 0;
			s->nop_executing = NULL;
			queue_initDL(s->q_init, &(s->getDataWaitQueue));
			return SOS_OK;
		}
		case MSG_DATA_READY:
		{
			__asm __volatile("en_rcv:");
			__asm __volatile("nop");
			__asm __volatile("nop");
			__asm __volatile("st_data:");
			MsgParam* param = (MsgParam*) (msg->data);
			uint16_t photo_data = param->word;
			s->busy = 0;
			if (s->executing != NULL) {
				DEBUG("BASICLIB (%d): Sensor returns - Context resumed.\n",s->executing->which);
				pushValueDL(s->push_value, (DvmState *)(s->executing), photo_data, DVM_TYPE_INTEGER); 
				resumeContextDL(s->ctx_resume, s->executing, s->executing);
				s->executing = NULL;
			}

			if (!queue_emptyDL(s->q_empty, &(s->getDataWaitQueue))) {
				DvmContext *current = queue_dequeueDL(s->q_dequeue, NULL, &(s->getDataWaitQueue));
				DEBUG("BASICLIB (%d): Sensor returns - Some context waiting to get photo data.\n",current->which);
				tryget(current, PHOTO, s);
			}
			__asm __volatile("en_data:");
			__asm __volatile("nop");
			return SOS_OK;
		}			
		case MOD_MSG_START:
		{
			uint16_t i;

			for( i = 0; i < PER_DELAY; i++ ) {
				asm volatile("nop");
			}
			s->delay_cnt++;
			if( s->delay_cnt >= COMPUTATION_DELAY ) {
				// Delay done
				if( s->nop_executing != NULL ) {
					resumeContextDL(s->ctx_resume, s->nop_executing, s->nop_executing);
					s->nop_executing = NULL;
				}
			} else {
				post_short( M_BASIC_LIB, M_BASIC_LIB, MOD_MSG_START, 0, 0, 0 );
			}
			return SOS_OK;
		}
		default:
			return SOS_OK;
	}
	return SOS_OK;
}        

#ifndef _MODULE_
int8_t execute(DvmState *eventState) 
{
	app_state *s = &basiclib_state;
#else
static int8_t execute( func_cb_ptr p, DvmState *eventState)
{
	app_state *s = (app_state*)ker_get_module_state(M_BASIC_LIB);
#endif
	DvmContext *context = &(eventState->context);
	
	while ((context->state == DVM_STATE_RUN) && (context->num_executed < DVM_CPU_SLICE)) { 
		DvmOpcode instr = getOpcodeDL(s->get_opcode, context->which, context->pc);
		DEBUG("VM (%d): PC is %d. instr %d\n",context->which, context->pc, instr);
		if(instr & LIB_ID_BIT)
		{	//Extension Library opcode encountered
			return SOS_OK;
		}
		context->num_executed++;
		switch(instr)
		{
			case OP_START:
			{
				__asm __volatile("st_sos:");
				context->pc += 1;
				break;
			}
			case OP_STOP:
			{
				__asm __volatile("en_sos:");
				context->pc += 1;
				break;
			}
			case OP_HALT:
			{
				DEBUG("VM (%d): HALT executed.\n", (int)context->which);
				haltContextDL(s->ctx_halt, context);
				context->state = DVM_STATE_HALT;
				context->pc = 0;
				break;
			}
			case OP_LED:
			{
				DvmStackVariable* arg = popOperandDL(s->pop_operand, eventState);
				led_op(arg->value.var);
				context->pc += 1;
				break;
			}

			case OP_GETVAR + 0: case OP_GETVAR + 1: case OP_GETVAR + 2:
			case OP_GETVAR + 3: case OP_GETVAR + 4: case OP_GETVAR + 5:
			case OP_GETVAR + 6: case OP_GETVAR + 7:
			{
				__asm __volatile("st_getv:");
				uint8_t arg = instr - OP_GETVAR;
				DEBUG("BASIC_LIB : OPGETVAR (%d):: pushing value %d.\n", (int)arg,(int)s->shared_vars[arg].value.var);
				pushOperandDL(s->push_operand, eventState, &s->shared_vars[arg]);
				context->pc += 1;
				__asm __volatile("en_getv:");
				__asm __volatile("nop");
				break;
			}
			case OP_SETVAR + 0: case OP_SETVAR + 1: case OP_SETVAR + 2:
			case OP_SETVAR + 3: case OP_SETVAR + 4: case OP_SETVAR + 5:
			case OP_SETVAR + 6: case OP_SETVAR + 7:
			{
				__asm __volatile("st_setv:");
				uint8_t arg = instr - OP_SETVAR;
				DvmStackVariable* var = popOperandDL(s->pop_operand, eventState);
				DEBUG("BASIC_LIB: OPSETVAR (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
				s->shared_vars[arg] = *var;
				context->pc += 1;
				__asm __volatile("en_setv:");
				__asm __volatile("nop");
				break;
			}
			case OP_GETVARF + 0: case OP_GETVARF + 1: case OP_GETVARF + 2:
			case OP_GETVARF + 3: case OP_GETVARF + 4: case OP_GETVARF + 5:
			case OP_GETVARF + 6: case OP_GETVARF + 7:
			{	// Use for type casting an integer shared var to float
				uint8_t arg = instr - OP_GETVARF;
				int32_t res = 0;
				uint16_t res_part = 0;
				DEBUG("BASIC_LIB : OPGETVARF (%d):: pushing value %d.\n", (int)arg,(int)s->shared_vars[arg].value.var);
				res = (int32_t)(s->shared_vars[arg].value.var * FLOAT_PRECISION);
				res_part = res & 0xFFFF;
				pushValueDL(s->push_value, eventState, res_part, DVM_TYPE_FLOAT_DEC);
				res_part = res >> 16;
				pushValueDL(s->push_value, eventState, res_part, DVM_TYPE_FLOAT);
				context->pc += 1;
				break;
			}
			case OP_SETVARF + 0: case OP_SETVARF + 1: case OP_SETVARF + 2:
			case OP_SETVARF + 3: case OP_SETVARF + 4: case OP_SETVARF + 5: 
			case OP_SETVARF + 6:
			{	// Type-casting an integer to float and saving it in shared var
				uint8_t arg = instr - OP_SETVARF;
				DvmStackVariable* var = popOperandDL(s->pop_operand, eventState);
				int32_t res = 0;
				uint16_t res_part;
				DEBUG("BASIC_LIB: OPSETVARF (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
				res = (int32_t)(var->value.var * FLOAT_PRECISION);
				res_part = res & 0xFFFF;
				s->shared_vars[arg+1].type = DVM_TYPE_FLOAT_DEC;
				s->shared_vars[arg+1].value.var = res_part;
				res_part = res >> 16;
				s->shared_vars[arg].type = DVM_TYPE_FLOAT;
				s->shared_vars[arg].value.var = res_part;
				context->pc += 1;
				break;
			}
			case OP_SETTIMER + 0: case OP_SETTIMER + 1: case OP_SETTIMER + 2: 
			case OP_SETTIMER + 3: case OP_SETTIMER + 4: case OP_SETTIMER + 5: 
			case OP_SETTIMER + 6: case OP_SETTIMER + 7:
			{
				uint32_t msec;
				uint8_t timerID = instr - OP_SETTIMER;
				DvmStackVariable* arg = popOperandDL(s->pop_operand, eventState);
				DEBUG("BASIC_LIB: Setting timer 0 period to %d.\n", arg->value.var);
				//msec = 102 * arg->value.var + (4 * arg->value.var) / 10; 
				msec = arg->value.var; 
				ker_timer_stop(M_HANDLER_STORE, timerID);
				if (msec > 0) {
					ker_timer_start(M_HANDLER_STORE, timerID, msec);
					DEBUG("BasicLib: TIMER STARTED FOR ID %d at %d \n", timerID, msec);
				}
				context->pc += 1;
				break;
			}
			case OP_RAND:
			{
				DvmStackVariable* arg = popOperandDL(s->pop_operand, eventState);
				uint16_t rnd;
				rnd = ker_rand() % arg->value.var;
				pushValueDL(s->push_value, eventState, rnd, DVM_TYPE_INTEGER);
				context->pc += 1;
				break;
			}
			case OP_JMP: case OP_JNZ: case OP_JZ: case OP_JG:
			case OP_JGE: case OP_JL: case OP_JLE: case OP_JE:
			case OP_JNE:
		   	case OP_ADD: case OP_SUB: case OP_DIV: case OP_MUL: 
			case OP_ABS: case OP_MOD: case OP_INCR: case OP_DECR: 
			{
				mathlib_executeDL(s->mathlib_execute, eventState, instr);
				break;
			}
			case OP_GET_DATA + 0: case OP_GET_DATA + 1: case OP_GET_DATA + 2:
			case OP_GET_DATA + 3: case OP_GET_DATA + 4: case OP_GET_DATA + 5:
			case OP_GET_DATA + 6:
			{
				__asm __volatile("st_get:");
				uint8_t sensor_type = instr - OP_GET_DATA;
				int8_t res = SOS_OK;
				context->pc += 1;
				if (s->busy) {
					context->state = DVM_STATE_WAITING;
					queue_enqueueDL(s->q_enqueue, context, &(s->getDataWaitQueue), context);
					DEBUG("BASICLIB (%d): Calling get sensor data - Sensor busy\n",context->which);
					res = SOS_OK;
				} else {
					DEBUG("BASICLIB (%d): Calling get photo data\n",context->which);
					res = tryget(context, sensor_type, s);
				}
				__asm __volatile("en_get:");
				__asm __volatile("nop");
				__asm __volatile("nop");
				__asm __volatile("st_rcv:");
				__asm __volatile("nop");
				return res;
			}
			case OP_NOP:
			{
				context->pc += 1;
				s->delay_cnt = 0;
				s->nop_executing = context;
				context->state = DVM_STATE_BLOCKED;
				post_short( M_BASIC_LIB, M_BASIC_LIB, MOD_MSG_START, 0, 0, 0 );
				yieldContextDL(s->ctx_yield, context);
				return SOS_OK;
			}
			case OP_POST:	//Asynchronous call
			{
				if (post_op(eventState, s) < 0)
					return SOS_OK;
				break;
			}
			case OP_CALL:	//synchronous call
			{
				call_op(eventState, s);
				break;
			}
			case OP_BPUSH + 0: case OP_BPUSH + 1:
		   	case OP_BPUSH + 2: case OP_BPUSH + 3:
			{
				__asm __volatile("st_bp:");
				uint8_t buf_idx = instr - OP_BPUSH;
				DEBUG("VM (%d) BPUSH %d\n", context->pc, buf_idx);
				pushBufferDL(s->push_buffer, eventState, &(s->buffers[buf_idx]));
				__asm __volatile("en_bp:");
				context->pc += 1;
				break;
			}
			case OP_BAPPEND + 0:	case OP_BAPPEND + 1:	
			case OP_BAPPEND + 2:	case OP_BAPPEND + 3:	
			case OP_BAPPEND + 4:	case OP_BAPPEND + 5:	
			case OP_BAPPEND + 6:	//Always leaves the buffer on the stack
			case OP_BCLEAR:
			case OP_BREADF:
			case OP_BSET:
			case OP_GETLOCAL + 0: case OP_GETLOCAL + 1: case OP_GETLOCAL + 2:
			case OP_GETLOCAL + 3: case OP_GETLOCAL + 4:
			case OP_SETLOCAL + 0: case OP_SETLOCAL + 1: case OP_SETLOCAL + 2:
			case OP_SETLOCAL + 3: case OP_SETLOCAL + 4:
			case OP_GETLOCALF + 0: case OP_GETLOCALF + 1: case OP_GETLOCALF + 2:
			case OP_GETLOCALF + 3: case OP_GETLOCALF + 4:
			case OP_SETLOCALF + 0: case OP_SETLOCALF + 1:
			case OP_SETLOCALF + 2: case OP_SETLOCALF + 3:
			case OP_PUSH:
			case OP_PUSHF:
			case OP_POP:
			case OP_POSTNET:
			case OP_BCAST:
			{
				buffer_executeDL(s->buffer_execute, eventState, instr);
				break;
			}
				
			default:
			{
				context->pc += 1;
				break;
			}
		}	//instruction selection ends
	}	//while loop ends

	return SOS_OK;

}

static inline void led_op(uint16_t val)
{
	uint8_t op = (val >> 3) & 3;
	uint8_t led	= val & 7;
	DEBUG("BASICLIB: Executing LED with %d and %d\n", op, led);
	switch (op) 
	{
		case 0:			/* set */
		{
			if (led & 1) 
				ker_led(LED_RED_ON);
			else 
				ker_led(LED_RED_OFF);
			if (led & 2)
				ker_led(LED_GREEN_ON);
			else
				ker_led(LED_GREEN_OFF);
			if (led & 4)
				ker_led(LED_YELLOW_ON);
			else
				ker_led(LED_YELLOW_OFF);
			break;
		}
		case 1:			/* OFF 0 bits */
		{
			if (!(led & 1)) ker_led(LED_RED_OFF);
			if (!(led & 2)) ker_led(LED_GREEN_OFF);
			if (!(led & 4)) ker_led(LED_YELLOW_ON);
			break;
		}
		case 2:			/* on 1 bits */
		{
			if (led & 1) ker_led(LED_RED_ON);
			if (led & 2) ker_led(LED_GREEN_ON);
			if (led & 4) ker_led(LED_YELLOW_ON);
			break;
		}
		case 3:			/* TOGGLE 1 bits */
		{
			if (led & 1) ker_led(LED_RED_TOGGLE);
			if (led & 2) ker_led(LED_GREEN_TOGGLE);
			if (led & 4) ker_led(LED_YELLOW_TOGGLE);
			break;
		}
		default:
		{
			DEBUG("VM: LED command had unknown operations.\n");
			break;
		}
	}

}

static inline int8_t call_op(DvmState *eventState, app_state *s) {
	// Argument can be either an integer or a buffer
	// Result will depend on the function being called, and the 
	// script compiler will take care of pushing the appropriate variable
	DvmContext *context = &(eventState->context);
	uint8_t size = 0;
	DvmStackVariable *callArgs = popOperandDL(s->pop_operand, eventState);
	DvmStackVariable *retValue = popOperandDL(s->pop_operand, eventState);

	if (retValue->type != DVM_TYPE_BUFFER) {DEBUG("\n\n\nPROBLEM IN RET BUFFER\n\n\n");}

	context->pc += 1;
	uint8_t mod_id = getOpcodeDL(s->get_opcode, context->which, context->pc);
	context->pc += 1;
	uint8_t fnid = getOpcodeDL(s->get_opcode, context->which, context->pc);

	if (ker_fntable_subscribe(s->pid, mod_id, EXECUTE_SYNCALL, 0) != SOS_OK) {
		DEBUG("\n\n\n\nSUBSCRIPTION PROBLEMS\n\n\n\n\n");
	}

	if (callArgs->type == DVM_TYPE_BUFFER) {
		size = callArgs->buffer.var->size;
		DEBUG("BASICLIB: OPCALL arg size = %d mod = %d fnid = %d.\n", size, mod_id, fnid);
	} else if (callArgs->type == DVM_TYPE_INTEGER) {
		size = 2;
	} 
	SOS_CALL(s->execute_syncall, func_i8u8zu8z_t, fnid, callArgs, size, retValue);
	pushOperandDL(s->push_operand, eventState, retValue);

	context->pc += 1;
	return SOS_OK;
}

static inline int8_t post_op(DvmState *eventState, app_state *s) {
	// Post the stack variable + fnid.
	// The "post stub" should extract and COPY the required information, and post back to itself
	post_msg_type *pmsg = (post_msg_type *)ker_malloc(sizeof(uint8_t)+sizeof(DvmStackVariable*), s->pid);
	DvmContext *context = &(eventState->context);

	DvmStackVariable *callArgs = popOperandDL(s->pop_operand, eventState);
	context->pc += 1;
	uint8_t mod_id = getOpcodeDL(s->get_opcode, context->which, context->pc);
	context->pc += 1;
	uint8_t fnid = getOpcodeDL(s->get_opcode, context->which, context->pc);

	pmsg->fnid = fnid;
	pmsg->argBuf = callArgs;

	if (post_long(mod_id, s->pid, POST_EXECUTE, sizeof(post_msg_type), pmsg, SOS_MSG_RELEASE) != SOS_OK) {
		//unable to post message for some reason 
		//So, restore the stack condition and PC
		//And, yield the context and put it in ready queue
		pushOperandDL(s->push_operand, eventState, callArgs);
		context->pc -= 2;
		yieldContextDL(s->ctx_yield, context);
		resumeContextDL(s->ctx_resume, context, context);
		context->state = DVM_STATE_RUN;
		//Context should be put back in ready queue
		//so that the post operation can be retried
		return -EINVAL;
	}
	//context->state = DVM_STATE_BLOCKED;
	//queue_enqueueDL(s->q_enqueue, context, &s->blockedQueue, context);
	context->pc += 1;
	DEBUG("BASICLIB (%d): Posting to tree routing module.\n",context->which);
	return SOS_OK;
}

static int8_t tryget(DvmContext *context, uint8_t sensor_type, app_state *s) {

	if (ker_sensor_get_data(s->pid, sensor_type) == SOS_OK) {
		s->busy = 1;
		s->executing = context;
		context->state = DVM_STATE_BLOCKED;
		yieldContextDL(s->ctx_yield, context);
	} else {
		context->state = DVM_STATE_WAITING;
		queue_enqueueDL(s->q_enqueue, context, &(s->getDataWaitQueue), context);
	}
	return SOS_OK;
}

#ifndef _MODULE_
void rebooted() 
{
	app_state *s = &basiclib_state;
#else
static void rebooted(func_cb_ptr p) 
{
	app_state *s = (app_state*)ker_get_module_state(M_BASIC_LIB);
#endif
	int i;

	queue_initDL(s->q_init, &(s->getDataWaitQueue));

	for (i = 0; i < DVM_NUM_BUFS; i++) {
		s->buffers[i].size = 0;
	}	

	for (i = 0; i < DVM_NUM_SHARED_VARS; i++) {
		s->shared_vars[i].type = DVM_TYPE_INTEGER;
		s->shared_vars[i].value.var = 0;
	}

}

#ifndef _MODULE_
uint8_t bytelength(uint8_t opcode)
#else
static uint8_t bytelength(func_cb_ptr p, uint8_t opcode)
#endif
{
	switch(opcode)
	{
		case OP_PUSHF:
		{
			return 5;
		}
		case OP_PUSH: case OP_JMP: case OP_JNZ: case OP_JZ:
		case OP_JG: case OP_JGE: case OP_JL: case OP_JLE:
		case OP_JE: case OP_JNE:
		{
			return 2;
		}
		case OP_CALL: case OP_POST:
		{
			return 3;
		}
		default:
			return 1;
	}	
}

#ifndef _MODULE_
int16_t lockNum(uint8_t instr)
#else
static int16_t lockNum(func_cb_ptr p, uint8_t instr)
#endif
{
	switch(instr) {
		case OP_BPUSH + 0: return 0;
		case OP_BPUSH + 1: return 1;
		case OP_BPUSH + 2: return 2;
		case OP_BPUSH + 3: return 3;
		case OP_SETVARF + 0: 
		case OP_GETVARF + 0: 
		case OP_SETVAR + 0: 
		case OP_GETVAR + 0: return 4;
		case OP_SETVARF + 1: 
		case OP_GETVARF + 1: 
		case OP_SETVAR + 1: 
		case OP_GETVAR + 1: return 5;
		case OP_SETVARF + 2: 
		case OP_GETVARF + 2: 
		case OP_SETVAR + 2: 
		case OP_GETVAR + 2: return 6;
		case OP_SETVARF + 3: 
		case OP_GETVARF + 3: 
		case OP_SETVAR + 3: 
		case OP_GETVAR + 3: return 7;
		case OP_SETVARF + 4: 
		case OP_GETVARF + 4: 
		case OP_SETVAR + 4: 
		case OP_GETVAR + 4: return 8;
		case OP_SETVARF + 5: 
		case OP_GETVARF + 5: 
		case OP_SETVAR + 5: 
		case OP_GETVAR + 5: return 9;
		case OP_SETVARF + 6: 
		case OP_GETVARF + 6: 
		case OP_SETVAR + 6: 
		case OP_GETVAR + 6: return 10;
		case OP_SETVAR + 7: 
		case OP_GETVAR + 7: return 11;
		default: return -1;
	}
}

