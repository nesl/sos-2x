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
	int32_t fl_post;			//for posting a int32_t/float directly
} app_state;

static void buffer_get(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint16_t *dest);
static void buffer_set(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint32_t val); 
static void buffer_clear(DvmDataBuffer *buffer); 
static void buffer_append(DvmDataBuffer *buffer, uint8_t numBytes, uint16_t var); 
static void buffer_concatenate(DvmDataBuffer *dst, DvmDataBuffer *src); 
static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2);

#ifndef _MODULE_                                                      

static app_state dvm_buffer_state;
int8_t dvm_buffer_init()
{
	return SOS_OK;
}

#else
static int8_t dvm_buffer(void *state, Message *msg);
static int8_t buffer_execute(func_cb_ptr p, DvmState *eventState, DvmOpcode instr);
static const mod_header_t mod_header SOS_MODULE_HEADER =              
{                                                                     
	.mod_id         = M_BUFFER,                                    
	.code_id        = ehtons(M_BUFFER),                            
	.platform_type  = HW_TYPE,                                        
	.processor_type = MCU_TYPE,                                       
	.state_size     = sizeof(app_state),                                              
	.num_timers     = 0,                                              
	.num_sub_func   = 5,                                              
	.num_prov_func  = 1,                                              
	.module_handler = dvm_buffer,                                  
	.funct          = {
		[0] = USE_FUNC_pushValue(M_STACKS, PUSHVALUE),
		[1] = USE_FUNC_pushOperand(M_STACKS, PUSHOPERAND),
		[2] = USE_FUNC_pushBuffer(M_STACKS, PUSHBUFFER),
		[3] = USE_FUNC_popOperand(M_STACKS, POPOPERAND),
		[4] = USE_FUNC_getOpcode(M_HANDLER_STORE, GETOPCODE),
		[5] = PRVD_FUNC_buffer_execute(M_BUFFER, BUFFER_EXECUTE),
	},

};     
static int8_t dvm_buffer(void *state, Message *msg)
{
	return SOS_OK;
}

#ifdef LINK_KERNEL
int8_t dvm_buffer_init()
{
	return ker_register_module(sos_get_header_address(mod_header));   
}
#endif

#endif



#ifndef _MODULE_
int8_t buffer_execute(DvmState *eventState, DvmOpcode instr)
{
	app_state *s = &dvm_buffer_state;
#else
static int8_t buffer_execute(func_cb_ptr p, DvmState *eventState, DvmOpcode instr)
{
	app_state *s = (app_state*)ker_get_module_state(M_BUFFER);
#endif
	DvmContext *context = &(eventState->context);

	switch( instr )
	{
			case OP_GETLOCAL + 0: case OP_GETLOCAL + 1: case OP_GETLOCAL + 2:
			case OP_GETLOCAL + 3: case OP_GETLOCAL + 4:
			{
				__asm __volatile("st_getl:");
				uint8_t arg = instr - OP_GETLOCAL;
				DEBUG("BASIC_LIB : OPGETLOCAL (%d):: pushing value %d.\n", (int)arg,(int)eventState->vars[arg].value.var);
				if (eventState->vars[arg].type == DVM_TYPE_FLOAT) {
					pushOperandDL(s->push_operand, eventState, &eventState->vars[arg+1]);
				}
				pushOperandDL(s->push_operand, eventState, &eventState->vars[arg]);
				context->pc += 1;
				__asm __volatile("en_getl:");
				__asm __volatile("nop");
				break;
			}
			case OP_SETLOCAL + 0: case OP_SETLOCAL + 1: case OP_SETLOCAL + 2:
			case OP_SETLOCAL + 3: case OP_SETLOCAL + 4:
			{
				__asm __volatile("st_setl:");
				uint8_t arg = instr - OP_SETLOCAL;
				DvmStackVariable* var = popOperandDL(s->pop_operand, eventState), *var1 = NULL;
				DEBUG("BASIC_LIB: OPSETLOCAL (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
				eventState->vars[arg] = *var;
				if (var->type == DVM_TYPE_FLOAT) {
					var1 = popOperandDL(s->pop_operand, eventState);
					eventState->vars[arg+1] = *var1;
					DEBUG("BASIC_LIB: OPSETLOCAL (%d):: Setting LSB value to %d.\n",(int)arg,(int)var1->value.var);
				}
				context->pc += 1;
				__asm __volatile("en_setl:");
				__asm __volatile("nop");
				__asm __volatile("nop");
				break;
			}
			case OP_GETLOCALF + 0: case OP_GETLOCALF + 1: case OP_GETLOCALF + 2:
			case OP_GETLOCALF + 3: case OP_GETLOCALF + 4:
			{	// Only used for type-casting integer to float
				__asm __volatile("st_getf:");
				uint8_t arg = instr - OP_GETLOCALF;
				int32_t res = 0;
				uint16_t res_part = 0;
				DEBUG("BASIC_LIB : OPGETLOCALF (%d):: pushing value %d.\n", (int)arg,(int)eventState->vars[arg].value.var);
				
				if (eventState->vars[arg].type == DVM_TYPE_INTEGER) {
					res = (int32_t)(eventState->vars[arg].value.var * FLOAT_PRECISION);
					res_part = res & 0xFFFF;
					pushValueDL(s->push_value, eventState, res_part, DVM_TYPE_FLOAT_DEC);
					res_part = res >> 16;
					pushValueDL(s->push_value, eventState, res_part, DVM_TYPE_FLOAT);
				} else if (eventState->vars[arg].type == DVM_TYPE_FLOAT) {
					pushOperandDL(s->push_operand, eventState, &eventState->vars[arg+1]);
					pushOperandDL(s->push_operand, eventState, &eventState->vars[arg]);
				}
				context->pc += 1;
				__asm __volatile("en_getf:");
				__asm __volatile("nop");
				break;
			}
			case OP_SETLOCALF + 0: case OP_SETLOCALF + 1:
			case OP_SETLOCALF + 2: case OP_SETLOCALF + 3:
			{	// Only used for type-casting integer to float
				uint8_t arg = instr - OP_SETLOCALF;
				DvmStackVariable* var = popOperandDL(s->pop_operand, eventState);
				int32_t res = 0;
				uint16_t res_part;
				DEBUG("BASIC_LIB: OPSETLOCALF (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
				if (var->type == DVM_TYPE_INTEGER) {
					res = (int32_t)(var->value.var * FLOAT_PRECISION);
					res_part = res & 0xFFFF;
					eventState->vars[arg+1].type = DVM_TYPE_FLOAT_DEC;
					eventState->vars[arg+1].value.var = res_part;
					res_part = res >> 16;
					eventState->vars[arg].type = DVM_TYPE_FLOAT;
					eventState->vars[arg].value.var = res_part;
				} else if (var->type == DVM_TYPE_FLOAT) {
					DvmStackVariable* var1 = popOperandDL(s->pop_operand, eventState);
					eventState->vars[arg] = *var;
					eventState->vars[arg+1] = *var1;
				}
				context->pc += 1;
				break;
			}


		case OP_PUSH:
			{
				__asm __volatile("st_push:");
				context->pc += 1;
				DvmOpcode arg = getOpcodeDL(s->get_opcode, context->which, context->pc);
				pushValueDL(s->push_value, eventState, arg, DVM_TYPE_INTEGER);
				context->pc += 1;
				DEBUG("VM (%i): Executing PUSH with arg %hi\n", (int)context->which, arg);
				__asm __volatile("en_push:");
				__asm __volatile("nop");
				break;
			}
			case OP_PUSHF:
			{
				uint16_t arg;
				context->pc += 1;
				DvmOpcode arg1 = getOpcodeDL(s->get_opcode, context->which, context->pc), arg2, arg3, arg4;
				context->pc += 1;
				arg2 = getOpcodeDL(s->get_opcode, context->which, context->pc);
				context->pc += 1;
				arg3 = getOpcodeDL(s->get_opcode, context->which, context->pc);
				context->pc += 1;
				arg4 = getOpcodeDL(s->get_opcode, context->which, context->pc);
				arg = arg3;
				arg = (arg << 8) + arg4;
				pushValueDL(s->push_value, eventState, arg, DVM_TYPE_FLOAT_DEC);
				DEBUG("VM (%i): Executing PUSHF with decimal %d\n", (int)context->which, arg);
				arg = arg1;
				arg = (arg << 8) + arg2;
				pushValueDL(s->push_value, eventState, arg, DVM_TYPE_FLOAT);
				DEBUG("VM (%i): Executing PUSHF with arg %d \n", (int)context->which, arg);
				context->pc += 1;
				break;
			}
			case OP_POP:
			{
				popOperandDL(s->pop_operand, eventState);
				context->pc += 1;
				break;
			}
		
		
		case OP_BAPPEND + 0:
		case OP_BAPPEND + 1:
		case OP_BAPPEND + 2:
		case OP_BAPPEND + 3:
		case OP_BAPPEND + 4:
		case OP_BAPPEND + 5:               
		case OP_BAPPEND + 6:    //Always leaves the buffer on the stack
		{
			__asm __volatile("st_bapp1:");
			uint8_t opt = instr - OP_BAPPEND + 1;    
			uint8_t i = 0;    
			DvmStackVariable* bufarg;

			DEBUG("VM(%d) BAPPEND %d\n", context->pc, opt);
			bufarg = popOperandDL(s->pop_operand, eventState);     
			__asm __volatile("en_bapp1:");    
			__asm __volatile("nop");    
			for (; i < opt; i++) {   
				__asm __volatile("st_bapp2:");   
				DvmStackVariable* apparg = popOperandDL(s->pop_operand, eventState);  
				if (apparg->type == DVM_TYPE_BUFFER) {    
					buffer_concatenate(bufarg->buffer.var, apparg->buffer.var); 
				} else if (apparg->type == DVM_TYPE_FLOAT) {  
					DvmStackVariable* apparg_dec = popOperandDL(s->pop_operand, eventState);
					buffer_append(bufarg->buffer.var, 2, apparg_dec->value.var);
					buffer_append(bufarg->buffer.var, 2, apparg->value.var);
					DEBUG("BASICLIB: OPBAPPEND. %d %d \n", apparg->value.var, apparg_dec->value.var); 
				} else {
					buffer_append(bufarg->buffer.var, 2, apparg->value.var);   
					DEBUG("BASICLIB: OPBAPPEND. %d \n", apparg->value.var);
				}
				__asm __volatile("en_bapp2:");
				__asm __volatile("nop");
			}
			DEBUG("BASICLIB: After BAPPEND. buffer size is %d.\n",bufarg->buffer.var->size);
			__asm __volatile("st_bapp3:");                
			pushOperandDL(s->push_operand, eventState, bufarg);        
			context->pc += 1;                             
			__asm __volatile("en_bapp3:");                
			__asm __volatile("nop");                      
			break;              
		}
		case OP_BCLEAR:                                   
        {                                                 
			__asm __volatile("st_bclr:");                 
			DvmStackVariable* bufarg;
			DEBUG("VM(%d) BCLEAR\n", context->pc);
		   	bufarg = popOperandDL(s->pop_operand, eventState);    
			buffer_clear(bufarg->buffer.var);             
			pushOperandDL(s->push_operand, eventState, bufarg);        
			context->pc += 1;                             
			__asm __volatile("en_bclr:");                 
			__asm __volatile("nop");                      
			break;                                        
		} 
		case OP_BREADF:
		{                                                 
			__asm __volatile("st_brd:");                  
			DvmStackVariable* bufarg = popOperandDL(s->pop_operand, eventState);
			DvmStackVariable* indexarg = popOperandDL(s->pop_operand, eventState);  
			uint16_t res_part, res_part2;

			buffer_get(bufarg->buffer.var, 2, indexarg->value.var, &res_part);
			DEBUG("BASICLIB: OPBREADF. read decimal part %d\n", res_part);
			buffer_get(bufarg->buffer.var, 2, indexarg->value.var+2, &res_part2);                                                         
			DEBUG("BASICLIB: OPBREADF. read MSB %d\n", res_part2);
			pushValueDL(s->push_operand, eventState, res_part, DVM_TYPE_FLOAT_DEC);  
			pushValueDL(s->push_operand, eventState, res_part2, DVM_TYPE_FLOAT);     

			context->pc += 1;                             
			__asm __volatile("en_brd:");                  
			__asm __volatile("nop");                      
			break;                                        
		}
		case OP_BSET:                                     
		{                                                 
			__asm __volatile("st_bset:");                 
			DvmStackVariable* bufarg = popOperandDL(s->pop_operand, eventState);    
			DvmStackVariable* valuearg = popOperandDL(s->pop_operand, eventState);  
			DvmStackVariable* indexarg = NULL;            

			if (valuearg->type == DVM_TYPE_FLOAT) {       
				DvmStackVariable* valuearg1 = popOperandDL(s->pop_operand, eventState);
				indexarg = popOperandDL(s->pop_operand, eventState);  
				buffer_set(bufarg->buffer.var, 2, indexarg->value.var, valuearg1->value.var);                                             
				DEBUG("BASICLIB: OPBSET. set float LSB in buffer %d\n", valuearg1->value.var);                                            
				buffer_set(bufarg->buffer.var, 2, indexarg->value.var+2, valuearg->value.var);                                            
				DEBUG("BASICLIB: OPBSET. set float MSB in buffer %d\n", valuearg->value.var);
			} else {                                      
				indexarg = popOperandDL(s->pop_operand, eventState);  
				buffer_set(bufarg->buffer.var, 2, indexarg->value.var, valuearg->value.var);                                              
				DEBUG("BASICLIB: OPSET. set integer in buffer %d\n", valuearg->value.var);                                                
			}                                             

			context->pc += 1;                             
			__asm __volatile("en_bset:");                 
			__asm __volatile("nop");                      
			break;                                        
		}
		
		case OP_POSTNET:
			{
				DvmStackVariable* addarg = popOperandDL(s->pop_operand, eventState);
				DvmStackVariable* modarg = popOperandDL(s->pop_operand, eventState);
				DvmStackVariable* typearg = popOperandDL(s->pop_operand, eventState);
				DvmStackVariable* bufarg = popOperandDL(s->pop_operand, eventState);

				if (bufarg->type == DVM_TYPE_BUFFER) {
					post_net(modarg->value.var, M_BASIC_LIB, typearg->value.var, bufarg->buffer.var->size, bufarg->buffer.var->entries, 0, addarg->value.var);
				} else if (bufarg->type == DVM_TYPE_FLOAT) {
					DvmStackVariable* bufarg_dec = popOperandDL(s->pop_operand, eventState);
					s->fl_post = convert_to_float(bufarg, bufarg_dec);
					post_net(modarg->value.var, M_BASIC_LIB, typearg->value.var, sizeof(int32_t), &s->fl_post, 0, addarg->value.var);
				} else {
					post_net(modarg->value.var, M_BASIC_LIB, typearg->value.var, sizeof(int16_t), &bufarg->value.var, 0, addarg->value.var);
				}

				context->pc += 1;
				break;
			}	
			case OP_BCAST:
			{
				__asm __volatile("st_bcst:");
				DvmStackVariable* modarg = popOperandDL(s->pop_operand, eventState);
				DvmStackVariable* typearg = popOperandDL(s->pop_operand, eventState);
				DvmStackVariable* bufarg = popOperandDL(s->pop_operand, eventState);

				if (bufarg->type == DVM_TYPE_BUFFER) {
				DEBUG("\n\nBASICLIB (%d): BCAST BUFFER\n\n", context->which);
					post_net(modarg->value.var, M_BASIC_LIB, typearg->value.var, bufarg->buffer.var->size, bufarg->buffer.var->entries, 0, BCAST_ADDRESS);
					//DEBUG("\n\nBASICLIB (%d): BCAST \n\n", context->which);
				} else if (bufarg->type == DVM_TYPE_FLOAT) {
				DEBUG("\n\nBASICLIB (%d): BCAST FLOAT\n\n", context->which);
					DvmStackVariable* bufarg_dec = popOperandDL(s->pop_operand, eventState);
					s->fl_post = convert_to_float(bufarg, bufarg_dec);
					post_net(modarg->value.var, M_BASIC_LIB, typearg->value.var, sizeof(int32_t), &s->fl_post, 0, BCAST_ADDRESS);
				} else {
				DEBUG("\n\nBASICLIB (%d): BCAST VALUE\n\n", context->which);
					post_net(modarg->value.var, M_BASIC_LIB, typearg->value.var, sizeof(int16_t), &bufarg->value.var, 0, BCAST_ADDRESS);
				}
				//DEBUG("\n\nBASICLIB (%d): BCAST \n\n", context->which);

				context->pc += 1;
				__asm __volatile("en_bcst:");
				__asm __volatile("nop");
				break;
			}
		
	}
	return SOS_OK;
}

static void buffer_get(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint16_t *dest) 
{
	uint16_t res = 0;
	DEBUG("BUFFER_GET: offset %d numBytes %d size %d.\n", bufferOffset, numBytes, buffer->size);
	DEBUG("BUFFER entries %d %d \n", buffer->entries[0], buffer->entries[1]);
	if ((bufferOffset + numBytes) <= buffer->size) {
		if (numBytes == 2) {
			res = buffer->entries[bufferOffset+1];
		}
		*dest = (res << 8) + buffer->entries[bufferOffset];
		DEBUG("BUFFER: Correct place.\n");
	}
}

static void buffer_set(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint32_t val) 
{
	uint8_t i = 0;
	if ((bufferOffset + numBytes) <= DVM_BUF_LEN) {
		buffer->size = bufferOffset + numBytes;
		for (; i < numBytes; i++) {
			buffer->entries[bufferOffset + i] = val & 0xFF;
			val >>= 8;
		}
	}
}

static void buffer_clear(DvmDataBuffer *buffer) 
{
	buffer->size = 0;
}

static void buffer_append(DvmDataBuffer *buffer, uint8_t numBytes, uint16_t var) 
{
	if ((buffer->size + numBytes) <= DVM_BUF_LEN) {	// Append in Little Endian format
		buffer->entries[(int)buffer->size] = var & 0xFF;
		DEBUG("Inside BAPPEND: added 0x%x \n", buffer->entries[(int)buffer->size]);
		buffer->size++;
		if (numBytes == 2) {
			buffer->entries[(int)buffer->size] = var >> 8;
			DEBUG("Inside BAPPEND: added 0x%x \n", buffer->entries[(int)buffer->size]);
			buffer->size++;
		}
	}
}

static void buffer_concatenate(DvmDataBuffer *dst, DvmDataBuffer *src) 
{
	uint8_t i, start, end, num = 2;
	uint16_t var;

	start = dst->size;
	end = start + src->size;
	end = (end > DVM_BUF_LEN)? DVM_BUF_LEN : end;
	for (i=start; i<end; i+=num) {
		num = ((end - i) >= 2)? 2: (end - i);
		buffer_get(src, num, i - start, &var);
		buffer_append(dst, num, var);
	}
}

static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2) 
{
	uint32_t res = (uint16_t)arg1->value.var;
	res <<= 16;
	res += (uint16_t)arg2->value.var;
	return (int32_t)res;
}

