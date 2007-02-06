
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
		case OP_ADD:
		case OP_SUB:
		case OP_DIV:
		case OP_MUL:                                        
		{
			DvmStackVariable* arg1 = popOperandDL(s->pop_operand, eventState);
			DvmStackVariable* arg2 = popOperandDL(s->pop_operand, eventState);
			DvmStackVariable* arg3 = NULL, *arg4 = NULL;
			int32_t fl_arg1, fl_arg2;
			int32_t res = 0;
			uint16_t res_part;

			if (arg1->type == DVM_TYPE_FLOAT) {
				fl_arg1 = convert_to_float(arg1, arg2);
				arg3 = popOperandDL(s->pop_operand, eventState);
				if (arg3->type == DVM_TYPE_FLOAT) {
					arg4 = popOperandDL(s->pop_operand, eventState);
					fl_arg2 = convert_to_float(arg3, arg4);
					if(instr == OP_ADD) {
						res = (int32_t)(fl_arg1 + fl_arg2);
					} else if(instr == OP_SUB) {
						res = (int32_t)(fl_arg1 - fl_arg2);               
					} else if(instr == OP_DIV) {
						res = (int32_t)((fl_arg1 / fl_arg2) * FLOAT_PRECISION);  
					} else if(instr == OP_MUL) {
						res = (int32_t)((fl_arg1 * fl_arg2) / FLOAT_PRECISION);
					}
				} else {
					if(instr == OP_ADD) {
						res = (int32_t)(fl_arg1 + (arg3->value.var * FLOAT_PRECISION));
					} else if(instr == OP_SUB) {
						res = (int32_t)(fl_arg1 - (arg3->value.var * FLOAT_PRECISION)); 
					} else if(instr == OP_DIV) {
						res = (int32_t)(fl_arg1 / arg3->value.var); 
					} else if(instr == OP_MUL) {
						res = (int32_t)(fl_arg1 * arg3->value.var);           
					}
				}
				res_part = res & 0xFFFF;
				pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT_DEC);
				res_part = res >> 16;
				pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT);
				context->pc += 1;
				break;
			} else if (arg2->type == DVM_TYPE_FLOAT) {
				arg3 = popOperandDL(s->pop_operand, eventState);
				fl_arg2 = convert_to_float(arg2, arg3);
				if(instr == OP_ADD) {
					res = (int32_t)((arg1->value.var * FLOAT_PRECISION) + fl_arg2) ;
				} else if(instr == OP_SUB) {
					res = (int32_t)((arg1->value.var * FLOAT_PRECISION) - fl_arg2);     
				} else if(instr == OP_DIV) {
					res = (int32_t)((arg1->value.var * FLOAT_PRECISION) / fl_arg2);
				} else if(instr == OP_MUL) {
					res = (int32_t)((arg1->value.var * FLOAT_PRECISION) * fl_arg2);               
				}
				res_part = res & 0xFFFF;
				pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT_DEC);
				res_part = res >> 16;
				pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT);
				context->pc += 1;
				break;
			}
			if(instr == OP_ADD) {
			pushValueDL(s->push_operand, eventState, arg1->value.var + arg2->value.var, DVM_TYPE_INTEGER);
			} else if(instr == OP_SUB) {
				pushValueDL(s->push_operand, eventState, arg1->value.var - arg2->value.var, DVM_TYPE_INTEGER);                                                                 
			} else if(instr == OP_DIV) {
				pushValueDL(s->push_operand, eventState, arg1->value.var/arg2->value.var, DVM_TYPE_INTEGER);
			} else if(instr == OP_MUL) {
				pushValueDL(s->push_operand, eventState, (int16_t)(arg1->value.var*arg2->value.var), DVM_TYPE_INTEGER);
			}
			context->pc += 1;
			break;
		}
		case OP_ABS:
		case OP_INCR:                                                 
		case OP_DECR:
		{                                                             
			DvmStackVariable* arg1 = popOperandDL(s->pop_operand, eventState);    
			int32_t fl_arg1 = 0;                                      
			int32_t res = 0;                                          
			uint16_t res_part;                                        

			if (arg1->type == DVM_TYPE_FLOAT) {                       
				DvmStackVariable* arg2 = popOperandDL(s->pop_operand, eventState);
				fl_arg1 = convert_to_float(arg1, arg2);               
				if( instr == OP_ABS ) {
					if (fl_arg1 < 0) {                                    
						res = 0 - fl_arg1;                                
					}                                                     
				} else if( instr == OP_INCR ) {
					res = (int32_t)(fl_arg1 + (1 * FLOAT_PRECISION));     
				} else if( instr == OP_DECR ) {
					res = (int32_t)(fl_arg1 - (1 * FLOAT_PRECISION));     
				}
				res_part = res & 0xFFFF;                              
				pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT_DEC);            
				res_part = res >> 16;                                           
				pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT);
				context->pc += 1;                                     
				break;                                                
			} else {                                                  
				if( instr == OP_ABS ) {
					if (arg1->value.var < 0) { 
						res = 0 - arg1->value.var;
					} else {
						res = arg1->value.var;
					}						
				} else if( instr == OP_INCR ) {
					res = arg1->value.var + 1;
				} else if( instr == OP_DECR ) {
					res = arg1->value.var - 1;
				}
				pushValueDL(s->push_operand, eventState, (uint16_t)(res & 0xFFFF), DVM_TYPE_INTEGER);   
			}                                                         

			context->pc += 1;                                         
			break;                                                    
		}
		case OP_MOD:                                                  
		{                                                             
			DvmStackVariable* arg1 = popOperandDL(s->pop_operand, eventState);    
			DvmStackVariable* arg2 = popOperandDL(s->pop_operand, eventState);    

			pushValueDL(s->push_operand, eventState, arg1->value.var % arg2->value.var, DVM_TYPE_INTEGER);

			context->pc += 1;                                         
			break;                                                    
		}                                                             
		case OP_JMP:
		{
			DvmOpcode line_num = getOpcodeDL(s->get_opcode, context->which, ++context->pc);
			context->pc = line_num;
			break;
		}
		case OP_JNZ:
		case OP_JZ:
		{
			DvmStackVariable* arg1 = popOperandDL(s->pop_operand, eventState);
			DvmOpcode line_num = getOpcodeDL(s->get_opcode, context->which, ++context->pc);
			int32_t fl_arg1 = 1;

			DEBUG("VM (%d): JNZ or JZ\n", context->pc);
			if (arg1->type == DVM_TYPE_FLOAT) {
				DvmStackVariable* arg2 = popOperandDL(s->pop_operand, eventState);
				fl_arg1 = convert_to_float(arg1, arg2);
			} else {
				fl_arg1 = arg1->value.var;
			}                                             

			if( instr == OP_JNZ ) {
				if (fl_arg1 != 0) {                          
					context->pc = line_num;                   
				}  else {                                          
					context->pc += 1; 
				}		
			} else if( instr == OP_JZ ) {
				if (fl_arg1 == 0) {
					context->pc = line_num;
				} else {
					context->pc += 1;
				} 
			}
			break;                                        
		}
		case OP_JG:
		case OP_JGE:                                       
		case OP_JL:                                        
		case OP_JLE:                                       
		case OP_JE:                                        
		case OP_JNE:
		{
			__asm __volatile("st_jg:");
			DvmStackVariable* arg1 = popOperandDL(s->pop_operand, eventState);
			DvmStackVariable* arg2 = popOperandDL(s->pop_operand, eventState);
			DvmOpcode line_num = getOpcodeDL(s->get_opcode, context->which, context->pc + 1);
			context->pc += 1;
			DEBUG("BASICLIB (%d): Executing JG %d.\n",context->which, line_num);
			int32_t fl_arg1, fl_arg2;

			if (arg1->type == DVM_TYPE_FLOAT) {
				DvmStackVariable* arg3 = popOperandDL(s->pop_operand, eventState);
				fl_arg1 = convert_to_float(arg1, arg2);
				if (arg3->type == DVM_TYPE_FLOAT) {
					DvmStackVariable* arg4 = popOperandDL(s->pop_operand, eventState);
					fl_arg2 = convert_to_float(arg3, arg4);
				} else {
					fl_arg2 = arg3->value.var * FLOAT_PRECISION;
				}
			} else if (arg2->type == DVM_TYPE_FLOAT) {
				DvmStackVariable* arg3 = popOperandDL(s->pop_operand, eventState);
				fl_arg1 = arg1->value.var * FLOAT_PRECISION;
				fl_arg2 = convert_to_float(arg2, arg3);
			} else {
				fl_arg1 = arg1->value.var;
				fl_arg2 = arg2->value.var;
			}

			DEBUG("BASICLIB (%d): Executing JG. Compare %d %d.\n", context->which, fl_arg1, fl_arg2);

			if(
				( (instr == OP_JG)  && (fl_arg1 > fl_arg2))   ||
				( (instr == OP_JGE) && (fl_arg1 >= fl_arg2))  ||
				( (instr == OP_JL)  && (fl_arg1 < fl_arg2))   ||
				( (instr == OP_JLE) && (fl_arg1 <= fl_arg2))  ||
				( (instr == OP_JE)  && (fl_arg1 == fl_arg2))  ||
				( (instr == OP_JNE) && (fl_arg1 != fl_arg2)) ) {
				context->pc = line_num;                    
			} else {
				context->pc += 1;                          
			}
			__asm __volatile("en_jg:");
			__asm __volatile("nop");
			break;                                         
		}
		
		
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

