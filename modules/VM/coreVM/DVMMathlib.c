
//#include <sos_sched.h>
#include <module.h>

#ifdef LINK_KERNEL  
//#define _MODULE_
#endif

#include <VM/DVMBuffer.h>
#include <VM/DVMStacks.h>
#include <VM/DVMEventHandler.h>

typedef struct {
	func_cb_ptr push_value;
	func_cb_ptr push_operand;
	func_cb_ptr push_buffer;
	func_cb_ptr pop_operand;
	func_cb_ptr get_opcode;
} app_state;


static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2);
#ifndef _MODULE_                                                      

//static app_state    mathlib_state;

int8_t dvm_mathlib_init()
{               
	return SOS_OK;
}                   
#else
static int8_t dvm_mathlib(void *state, Message *msg);
static int8_t mathlib_execute(func_cb_ptr p, DvmState *eventState, DvmOpcode instr);
static const mod_header_t mod_header SOS_MODULE_HEADER =
{                                                                     
	.mod_id         = M_MATHLIB,                                    
	.code_id        = ehtons(M_MATHLIB),
	.platform_type  = HW_TYPE,                
	.processor_type = MCU_TYPE,         
	.state_size     = sizeof(app_state),
	.num_timers     = 0,   
	.num_sub_func   = 5,
	.num_prov_func  = 1,                  
	.module_handler = dvm_mathlib,                 
	.funct          = {
		[0] = USE_FUNC_pushValue(M_STACKS, PUSHVALUE),
		[1] = USE_FUNC_pushOperand(M_STACKS, PUSHOPERAND),
		[2] = USE_FUNC_pushBuffer(M_STACKS, PUSHBUFFER),
		[3] = USE_FUNC_popOperand(M_STACKS, POPOPERAND),
		[4] = USE_FUNC_getOpcode(M_HANDLER_STORE, GETOPCODE),
		[5] = {mathlib_execute, "vvv0", 0, 0},
	},
};          
static int8_t dvm_mathlib(void *state, Message *msg)
{
	return SOS_OK;
}
#ifdef LINK_KERNEL  
int8_t dvm_mathlib_init()
{
	return ker_register_module(sos_get_header_address(mod_header));
}
#endif
#endif

#ifndef _MODULE_
int8_t mathlib_execute(DvmState *eventState, DvmOpcode instr)
{
	//app_state *s = &mathlib_state;
#else
static int8_t mathlib_execute(func_cb_ptr p, DvmState *eventState, DvmOpcode instr)
{
	//app_state *s = (app_state*)ker_get_module_state(M_MATHLIB);
#endif
	DvmContext *context = &(eventState->context);

	switch( instr )
	{
		
	}
	return SOS_OK;
}

static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2)
{
	uint32_t res = (uint16_t)arg1->value.var;
	res <<= 16;
	res += (uint16_t)arg2->value.var;
	return (int32_t)res;
}

