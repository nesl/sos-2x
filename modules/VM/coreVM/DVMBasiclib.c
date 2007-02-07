/**
 * \file DVMBasiclib.c
 * \author Rahul Balani
 * \author Ram Kumar - Port to sos-2.x
 */

#include <VM/DVMBasiclib.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMStacks.h>
#include <VM/DVMqueue.h>
#include <VM/DVMBuffer.h>
#include <VM/DVMMathlib.h>
//--------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------
//
// For NOP instruction
//
#define PER_DELAY                1000L
#define COMPUTATION_DELAY        100L
//--------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------
typedef struct {
  uint8_t fnid;
  DvmStackVariable *argBuf;
} post_msg_type;

typedef struct 
{
  sos_pid_t pid;
  uint8_t busy;			  // for GET_DATA
  DvmContext *executing;	  // for GET_DATA
  DvmQueue getDataWaitQueue;	  // for GET_DATA
  DvmContext *nop_executing;	  // for NOP
  uint16_t delay_cnt;             // for NOP
  DvmDataBuffer buffers[DVM_NUM_BUFS];
  DvmStackVariable shared_vars[DVM_NUM_SHARED_VARS];
} app_state;

//--------------------------------------------------------------------
// STATIC FUNCTION DEFINITIONS
//--------------------------------------------------------------------
static int8_t basic_library(void *state, Message *msg);
static int8_t tryget(DvmContext *context, uint8_t sensor_type, app_state *s);
static inline int8_t post_op(DvmState *eventState, app_state *s); 
static inline int8_t call_op(DvmState *eventState, app_state *s); 
static inline void led_op(uint16_t val);
//--------------------------------------------------------------------
static int8_t basic_library(void *state, Message *msg)
{    
  app_state *s = (app_state *) state;

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
//--------------------------------------------------------------------
int8_t execute(DvmState *eventState) 
{
  app_state *s = (app_state*)ker_get_module_state(M_BASIC_LIB);
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
	/*
	  case OP_JMP: case OP_JNZ: case OP_JZ: case OP_JG:
	  case OP_JGE: case OP_JL: case OP_JLE: case OP_JE:
	  case OP_JNE:
	  case OP_ADD: case OP_SUB: case OP_DIV: case OP_MUL: 
	  case OP_ABS: case OP_MOD: case OP_INCR: case OP_DECR: 
	  {
	  mathlib_executeDL(s->mathlib_execute, eventState, instr);
	  break;
	  }
	*/
	// Math Lib
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
	/*
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
	*/
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

				
      default:
	{
	  context->pc += 1;
	  break;
	}
      }	//instruction selection ends
  }	//while loop ends

  return SOS_OK;

}
//--------------------------------------------------------------------
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
//--------------------------------------------------------------------
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
//--------------------------------------------------------------------
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
//--------------------------------------------------------------------
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
//--------------------------------------------------------------------
void rebooted() 
{
  app_state *s = (app_state*)ker_get_module_state(M_BASIC_LIB);

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
//--------------------------------------------------------------------
uint8_t bytelength(uint8_t opcode)
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
//--------------------------------------------------------------------
int16_t lockNum(uint8_t instr)
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
//--------------------------------------------------------------------
// LOCAL FUNCTIONS
//--------------------------------------------------------------------------------------------
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
  return;
}
//--------------------------------------------------------------------------------------------
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
  return;
}
//--------------------------------------------------------------------------------------------
static void buffer_clear(DvmDataBuffer *buffer) 
{
  buffer->size = 0;
  return;
}
//--------------------------------------------------------------------------------------------
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
  return;
}
//--------------------------------------------------------------------------------------------
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
  return;
}
//--------------------------------------------------------------------------------------------  
static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2) 
{
  uint32_t res = (uint16_t)arg1->value.var;
  res <<= 16;
  res += (uint16_t)arg2->value.var;
  return (int32_t)res;
}
//--------------------------------------------------------------------------------------------  
