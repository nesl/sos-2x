/**
 * \file DVMBasiclib.c
 * \author Rahul Balani
 * \author Ram Kumar - Port to sos-2.x
 */
//#define OUTLIER_SCRIPT_DBG
#include <VM/DVMBasiclib.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMStacks.h>
#include <VM/DVMqueue.h>
#include <VM/DVMBuffer.h>
#include <VM/DVMMathlib.h>
#include <led.h>
#include <sys_module.h>

#ifdef OUTLIER_SCRIPT_DBG
#include <demos/outlier_avg/outlier_avg.h>
#endif


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
typedef int8_t (*execute_syncall_func_t)(func_cb_ptr cb, uint8_t fnid, DvmStackVariable *arg, 
				  uint8_t size, DvmStackVariable *res);

typedef struct {
  uint8_t fnid;
  DvmStackVariable *argBuf;
} post_msg_type;

//--------------------------------------------------------------------
// STATIC FUNCTION DEFINITIONS
//--------------------------------------------------------------------
static int8_t tryget(dvm_state_t* dvm_st, DvmContext *context, uint8_t sensor_type, DVMBasiclib_state_t *s);
static inline int8_t post_op(dvm_state_t* dvm_st, DvmState *eventState, DVMBasiclib_state_t *s); 
static inline int8_t call_op(dvm_state_t* dvm_st, DvmState *eventState, DVMBasiclib_state_t *s); 
static inline void led_op(uint16_t val);
static void buffer_get(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint16_t *dest);
static void buffer_set(DvmDataBuffer *buffer, uint8_t numBytes, uint8_t bufferOffset, uint32_t val);
static void buffer_clear(DvmDataBuffer *buffer);
static void buffer_append(DvmDataBuffer *buffer, uint8_t numBytes, uint16_t var);
static void buffer_concatenate(DvmDataBuffer *dst, DvmDataBuffer *src);
static int32_t convert_to_float(DvmStackVariable *arg1, DvmStackVariable *arg2);
//--------------------------------------------------------------------
// MESSAGE HANDLER
//--------------------------------------------------------------------   
//    case MSG_INIT:
int8_t basic_library_init(dvm_state_t* dvm_st, Message *msg)
{	
  DVMBasiclib_state_t *s =  (&dvm_st->basiclib_st);
  s->busy = 0;
  s->nop_executing = NULL;
  queue_init( &(s->getDataWaitQueue));
  DEBUG("[BASIC LIB] basic_library_init: Success\n");
  return SOS_OK;
}
//--------------------------------------------------------------------   
//    case MSG_FINAL:
int8_t basic_library_final(dvm_state_t* dvm_st, Message* msg)
{
  return SOS_OK;
}
//--------------------------------------------------------------------   
//    case MSG_DATA_READY:
int8_t basic_library_data_ready(dvm_state_t* dvm_st, Message *msg)
{
  DVMBasiclib_state_t *s =  (&dvm_st->basiclib_st);
  MsgParam* param = (MsgParam*) (msg->data);
  uint16_t photo_data = param->word;
  s->busy = 0;
  if (s->executing != NULL) {
    DEBUG("[BASIC_LIB] basic_lib_data_ready: Context %d resumed.\n",s->executing->which);
    DEBUG("[BASIC_LIB] basic_lib_data_ready: Sensor Value: %d\n", photo_data);
    pushValue( (DvmState *)(s->executing), photo_data, DVM_TYPE_INTEGER); 
    resumeContext(dvm_st, s->executing, s->executing);
    s->executing = NULL;
#ifdef OUTLIER_SCRIPT_DBG
    uint16_t* sensor_val;
    sensor_val = (uint16_t*)sys_malloc(sizeof(uint16_t));
    *sensor_val = param->word;
    sys_post_uart(OUTLIER_DETECTION_PID, MSG_SENSOR_VALUE, sizeof(uint16_t), 
		  sensor_val, SOS_MSG_RELEASE , UART_ADDRESS);
#endif

  }
  
  if (!queue_empty(&(s->getDataWaitQueue))) {
    DvmContext *current = queue_dequeue( NULL, &(s->getDataWaitQueue));
    DEBUG("[BASIC_LIB] basic_lib_data_ready: Context %d waiting to get photo data.\n",current->which);
    tryget(dvm_st, current, PHOTO, s);
  }
  return SOS_OK;
}			
//--------------------------------------------------------------------   
// EXTERNAL FUNCTIONS
//--------------------------------------------------------------------
int8_t execute(dvm_state_t* dvm_st, DvmState *eventState) 
{
  DVMBasiclib_state_t *s = (&dvm_st->basiclib_st);
  DvmContext *context = &(eventState->context);
	
  while ((context->state == DVM_STATE_RUN) && (context->num_executed < DVM_CPU_SLICE)) { 
    DvmOpcode instr = getOpcode(dvm_st, context->which, context->pc);
    DEBUG("-----------------------------------------------\n");
    DEBUG("[BASIC_LIB] execute: (%d) PC: %d. INSTR: %d\n",context->which, context->pc, instr);
    if(instr & LIB_ID_BIT)
      {	//Extension Library opcode encountered
	return SOS_OK;
      }
    context->num_executed++;
    switch(instr)
      {
      case OP_START:
	{
	  __asm __volatile("__sleep_measure_start:");
	  context->pc += 1;
	  break;
	}
      case OP_STOP:
	{
	  context->pc += 1;
	  break;
	}
      case OP_HALT:
	{
	  DEBUG("<<<<<<<<<<<=====================>>>>>>>>>>>\n");
	  DEBUG("[BASIC_LIB] execute: (%d): HALT executed.\n", (int)context->which);
	  haltContext(dvm_st, context);
	  context->state = DVM_STATE_HALT;
	  context->pc = 0;
	  break;
	}
      case OP_LED:
	{
	  DvmStackVariable* arg = popOperand( eventState);
	  led_op(arg->value.var);
	  context->pc += 1;
	  break;
	}

      case OP_GETVAR + 0: case OP_GETVAR + 1: case OP_GETVAR + 2:
      case OP_GETVAR + 3: case OP_GETVAR + 4: case OP_GETVAR + 5:
      case OP_GETVAR + 6: case OP_GETVAR + 7:
	{
	  uint8_t arg = instr - OP_GETVAR;
	  DEBUG("[BASIC_LIB] execute: OPGETVAR (%d):: Pushing value %d.\n", (int)arg,(int)s->shared_vars[arg].value.var);
	  pushOperand( eventState, &s->shared_vars[arg]);
	  context->pc += 1;
	  break;
	}
      case OP_SETVAR + 0: case OP_SETVAR + 1: case OP_SETVAR + 2:
      case OP_SETVAR + 3: case OP_SETVAR + 4: case OP_SETVAR + 5:
      case OP_SETVAR + 6: case OP_SETVAR + 7:
	{
	  uint8_t arg = instr - OP_SETVAR;
	  DvmStackVariable* var = popOperand( eventState);
	  DEBUG("[BASIC_LIB] execute: OPSETVAR (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
	  s->shared_vars[arg] = *var;
	  context->pc += 1;
	  break;
	}
      case OP_GETVARF + 0: case OP_GETVARF + 1: case OP_GETVARF + 2:
      case OP_GETVARF + 3: case OP_GETVARF + 4: case OP_GETVARF + 5:
      case OP_GETVARF + 6: case OP_GETVARF + 7:
	{	// Use for type casting an integer shared var to float
	  uint8_t arg = instr - OP_GETVARF;
	  int32_t res = 0;
	  uint16_t res_part = 0;
	  DEBUG("[BASIC_LIB] execute: OPGETVARF (%d):: Pushing value %d.\n", (int)arg,(int)s->shared_vars[arg].value.var);
	  res = (int32_t)(s->shared_vars[arg].value.var * FLOAT_PRECISION);
	  res_part = res & 0xFFFF;
	  pushValue( eventState, res_part, DVM_TYPE_FLOAT_DEC);
	  res_part = res >> 16;
	  pushValue( eventState, res_part, DVM_TYPE_FLOAT);
	  context->pc += 1;
	  break;
	}
      case OP_SETVARF + 0: case OP_SETVARF + 1: case OP_SETVARF + 2:
      case OP_SETVARF + 3: case OP_SETVARF + 4: case OP_SETVARF + 5: 
      case OP_SETVARF + 6:
	{	// Type-casting an integer to float and saving it in shared var
	  uint8_t arg = instr - OP_SETVARF;
	  DvmStackVariable* var = popOperand( eventState);
	  int32_t res = 0;
	  uint16_t res_part;
	  DEBUG("[BASIC_LIB] execute: OPSETVARF (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
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
	  DvmStackVariable* arg = popOperand( eventState);
	  DEBUG("[BASIC_LIB] execute: Setting Timer %d period to %d.\n", timerID, arg->value.var);
	  //msec = 102 * arg->value.var + (4 * arg->value.var) / 10;
	  // Set the timer timeout argument in seconds
	  msec = arg->value.var * 1024;
	  DEBUG("[BASIC_LIB] execute: <<<<< WARNING - Timer %d not being stopped >>>>\n", timerID);
	  //	  sys_timer_stop(timerID);
	  if (msec > 0) {
	    // Ram - Where is the init ??
	    sys_timer_start(timerID, msec, TIMER_REPEAT);
	    DEBUG("[BASIC_LIB] execute: Timer ID: %d started. Period: %d msec.\n", timerID, msec);
	  }
	  context->pc += 1;
	  break;
	}
      case OP_RAND:
	{
	  DvmStackVariable* arg = popOperand( eventState);
	  uint16_t rnd;
	  rnd = sys_rand() % arg->value.var;
	  pushValue( eventState, rnd, DVM_TYPE_INTEGER);
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
	  DvmStackVariable* arg1 = popOperand( eventState);
	  DvmStackVariable* arg2 = popOperand( eventState);
	  DvmStackVariable* arg3 = NULL, *arg4 = NULL;
	  int32_t fl_arg1, fl_arg2;
	  int32_t res = 0;
	  uint16_t res_part;
	  int16_t int_res = 0;

	  if (arg1->type == DVM_TYPE_FLOAT) {
	    fl_arg1 = convert_to_float(arg1, arg2);
	    arg3 = popOperand( eventState);
	    if (arg3->type == DVM_TYPE_FLOAT) {
	      // FLOAT <op> FLOAT
	      arg4 = popOperand( eventState);
	      fl_arg2 = convert_to_float(arg3, arg4);
	      if(instr == OP_ADD) {
		res = (int32_t)(fl_arg1 + fl_arg2);
		DEBUG("[BASIC_LIB] execute: FLOAT ADD FLOAT %d\n", res);
	      } else if(instr == OP_SUB) {
		res = (int32_t)(fl_arg1 - fl_arg2);
		DEBUG("[BASIC_LIB] execute: FLOAT SUB FLOAT %d\n", res);               
	      } else if(instr == OP_DIV) {
		res = (int32_t)((fl_arg1 * FLOAT_PRECISION) / fl_arg2);
		DEBUG("[BASIC_LIB] execute: FLOAT DIV FLOAT: %d\n", res);
	      } else if(instr == OP_MUL) {
		res = (int32_t)((fl_arg1 * fl_arg2) / FLOAT_PRECISION);
		DEBUG("[BASIC_LIB] execute: FLOAT MULT FLOAT %d\n", res);
	      }
	    } else {
	      // FLOAT <OP> INTEGER
	      if(instr == OP_ADD) {
		res = (int32_t)(fl_arg1 + (arg3->value.var * FLOAT_PRECISION));
		DEBUG("[BASIC_LIB] execute: FLOAT ADD INT: %d\n", res);
	      } else if(instr == OP_SUB) {
		res = (int32_t)(fl_arg1 - (arg3->value.var * FLOAT_PRECISION)); 
		DEBUG("[BASIC_LIB] execute: FLOAT SUB INT: %d\n", res);
	      } else if(instr == OP_DIV) {
		res = (int32_t)(fl_arg1 / arg3->value.var); 
		DEBUG("[BASIC_LIB] execute: FLOAT DIV INT: %d\n", res);
		sys_led(LED_RED_TOGGLE);
#ifdef OUTLIER_SCRIPT_DBG
		int32_t* post_avg;
		post_avg = (int32_t*)sys_malloc(sizeof(int32_t));
		*post_avg = res;
		sys_post_uart(OUTLIER_DETECTION_PID, MSG_AVERAGE, sizeof(int32_t),
			      post_avg, SOS_MSG_RELEASE, UART_ADDRESS);
#endif
	      } else if(instr == OP_MUL) {
		res = (int32_t)(fl_arg1 * arg3->value.var);           
		DEBUG("[BASIC_LIB] execute: FLOAT MULT INT %d\n", res);
	      }
	    }
	    res_part = res & 0xFFFF;
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT_DEC);
	    res_part = res >> 16;
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT);
	    context->pc += 1;
	    break;
	  } else if (arg2->type == DVM_TYPE_FLOAT) {
	    arg3 = popOperand( eventState);
	    fl_arg2 = convert_to_float(arg2, arg3);
	    if(instr == OP_ADD) {
	      res = (int32_t)((arg1->value.var * FLOAT_PRECISION) + fl_arg2) ;
	      DEBUG("[BASIC_LIB] execute: INT ADD FLOAT: %d\n", res);
	    } else if(instr == OP_SUB) {
	      res = (int32_t)((arg1->value.var * FLOAT_PRECISION) - fl_arg2);     
	      DEBUG("[BASIC_LIB] execute: INT SUB FLOAT: %d\n", res);
	    } else if(instr == OP_DIV) {
	      res = (int32_t)((arg1->value.var * FLOAT_PRECISION) / fl_arg2);
	      DEBUG("[BASIC_LIB] execute: INT DIV FLOAT: %d\n", res);
	    } else if(instr == OP_MUL) {
	      res = (int32_t)((arg1->value.var * FLOAT_PRECISION) * fl_arg2);               
	      DEBUG("[BASIC_LIB] execute: INT MULT FLOAT: %d\n", res);
	    }
	    res_part = res & 0xFFFF;
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT_DEC);
	    res_part = res >> 16;
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT);
	    context->pc += 1;
	    break;
	  }
	  if(instr == OP_ADD) {
	    int_res = arg1->value.var + arg2->value.var;
	    DEBUG("[BASIC_LIB] execute: INT ADD INT: %d\n", int_res);
	  } else if(instr == OP_SUB) {
	    int_res = arg1->value.var - arg2->value.var;
	    DEBUG("[BASIC_LIB] execute: INT SUB INT: %d\n", int_res);
	  } else if(instr == OP_DIV) {
	    int_res = (int16_t)(arg1->value.var / arg2->value.var);
	    DEBUG("[BASIC_LIB] execute: INT DIV INT: %d\n", int_res);                                                          
	  } else if(instr == OP_MUL) {
	    int_res = (int16_t)(arg1->value.var * arg2->value.var);
	    DEBUG("[BASIC_LIB] execute: INT MULT INT: %d\n", int_res);
	  }
	  pushValue( eventState, int_res, DVM_TYPE_INTEGER);
	  context->pc += 1;
	  break;
	}
      case OP_ABS:
      case OP_INCR:                                                 
      case OP_DECR:
	{                                                             
	  DvmStackVariable* arg1 = popOperand( eventState);    
	  int32_t fl_arg1 = 0;                                      
	  int32_t res = 0;                                          
	  uint16_t res_part;                                        

	  if (arg1->type == DVM_TYPE_FLOAT) {                       
	    DvmStackVariable* arg2 = popOperand( eventState);
	    fl_arg1 = convert_to_float(arg1, arg2);               
	    if( instr == OP_ABS ) {	      
	      if (fl_arg1 < 0) {                                    
		res = 0 - fl_arg1; 
	      }
	      else{
		res = fl_arg1;
	      }
	      DEBUG("[BASIC_LIB] execute: ABS FLOAT: %d\n", res);                               		
	    } else if( instr == OP_INCR ) {
	      res = (int32_t)(fl_arg1 + (1 * FLOAT_PRECISION));
	      DEBUG("[BASIC_LIB] execute: INCR FLOAT: %d\n", res);                               		     
	    } else if( instr == OP_DECR ) {
	      res = (int32_t)(fl_arg1 - (1 * FLOAT_PRECISION));     
	      DEBUG("[BASIC_LIB] execute: DECR FLOAT: %d\n", res);                               		     
	    }
	    res_part = res & 0xFFFF;                              
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT_DEC);            
	    res_part = res >> 16;                                           
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT);
	    context->pc += 1;                                     
	    break;                                                
	  } else {                                                  
	    if( instr == OP_ABS ) {
	      if (arg1->value.var < 0) { 
		res = 0 - arg1->value.var;
	      } else {
		res = arg1->value.var;
	      }
	      DEBUG("[BASIC_LIB] execute: ABS INT: %d\n", res); 								
	    } else if( instr == OP_INCR ) {
	      res = arg1->value.var + 1;
	      DEBUG("[BASIC_LIB] execute: INCR INT: %d\n", res); 								
	    } else if( instr == OP_DECR ) {
	      res = arg1->value.var - 1;
	      DEBUG("[BASIC_LIB] execute: DECR INT: %d\n", res); 								
	    }
	    pushValue( eventState, (uint16_t)(res & 0xFFFF), DVM_TYPE_INTEGER);   
	  }                                                         

	  context->pc += 1;                                         
	  break;                                                    
	}
      case OP_MOD:                                                  
	{                                                             
	  DvmStackVariable* arg1 = popOperand( eventState);    
	  DvmStackVariable* arg2 = popOperand( eventState);    

	  pushValue( eventState, arg1->value.var % arg2->value.var, DVM_TYPE_INTEGER);
	  DEBUG("[BASIC_LIB] execute: MOD INT: %d\n", arg1->value.var % arg2->value.var);
	  context->pc += 1;                                         
	  break;                                                    
	}                                                             
      case OP_JMP:
	{
	  DvmOpcode line_num = getOpcode(dvm_st, context->which, ++context->pc);
	  context->pc = line_num;
	  DEBUG("[BASIC_LIB] execute: JMP to %d\n", context->pc);
	  break;
	}
      case OP_JNZ:
      case OP_JZ:
	{
	  DvmStackVariable* arg1 = popOperand( eventState);
	  DvmOpcode line_num = getOpcode(dvm_st, context->which, ++context->pc);
	  int32_t fl_arg1 = 1;

	  DEBUG("[BASIC_LIB] execute: (%d) JNZ or JZ\n", context->pc);
	  if (arg1->type == DVM_TYPE_FLOAT) {
	    DvmStackVariable* arg2 = popOperand( eventState);
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
	  DvmStackVariable* arg1 = popOperand( eventState);
	  DvmStackVariable* arg2 = popOperand( eventState);
	  DvmOpcode line_num = getOpcode( dvm_st,  context->which, context->pc + 1);
	  context->pc += 1;
	  DEBUG("[BASIC_LIB] execute: (%d) Executing JG %d.\n",context->which, line_num);
	  int32_t fl_arg1, fl_arg2;

	  if (arg1->type == DVM_TYPE_FLOAT) {
	    DvmStackVariable* arg3 = popOperand( eventState);
	    fl_arg1 = convert_to_float(arg1, arg2);
	    if (arg3->type == DVM_TYPE_FLOAT) {
	      DvmStackVariable* arg4 = popOperand( eventState);
	      fl_arg2 = convert_to_float(arg3, arg4);
	    } else {
	      fl_arg2 = arg3->value.var * FLOAT_PRECISION;
	    }
	  } else if (arg2->type == DVM_TYPE_FLOAT) {
	    DvmStackVariable* arg3 = popOperand( eventState);
	    fl_arg1 = arg1->value.var * FLOAT_PRECISION;
	    fl_arg2 = convert_to_float(arg2, arg3);
	  } else {
	    fl_arg1 = arg1->value.var;
	    fl_arg2 = arg2->value.var;
	  }

	  DEBUG("[BASIC_LIB] execute: (%d) Executing JG. Compare %d %d.\n", context->which, fl_arg1, fl_arg2);

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
	  break;                                         
	}
		

      case OP_GET_DATA + 0: case OP_GET_DATA + 1: case OP_GET_DATA + 2:
      case OP_GET_DATA + 3: case OP_GET_DATA + 4: case OP_GET_DATA + 5:
      case OP_GET_DATA + 6:
	{
	  uint8_t sensor_type = instr - OP_GET_DATA;
	  int8_t res = SOS_OK;
	  context->pc += 1;
	  if (s->busy) {
	    context->state = DVM_STATE_WAITING;
	    queue_enqueue( context, &(s->getDataWaitQueue), context);
	    DEBUG("[BASIC_LIB] execute: (%d) Calling get sensor data - Sensor ID: %d - Sensor busy\n",context->which, sensor_type);
	    res = SOS_OK;
	  } else {
	    DEBUG("[BASIC_LIB] execute: (%d) Calling get sensor data - Sensor ID: %d\n",context->which, sensor_type);
	    res = tryget(dvm_st, context, sensor_type, s);
	  }
	  return res;
	}
      case OP_NOP:
	{
	  context->pc += 1;
	  s->delay_cnt = 0;
	  s->nop_executing = context;
	  context->state = DVM_STATE_BLOCKED;
	  sys_post_value(DVM_MODULE_PID, MOD_MSG_START, 0, 0);
	  yieldContext(dvm_st, context);
	  return SOS_OK;
	}
      case OP_POST:	//Asynchronous call
	{
	  if (post_op(dvm_st, eventState, s) < 0)
	    return SOS_OK;
	  break;
	}
      case OP_CALL:	//synchronous call
	{
	  call_op(dvm_st, eventState, s);
	  break;
	}
      case OP_BPUSH + 0: case OP_BPUSH + 1:
      case OP_BPUSH + 2: case OP_BPUSH + 3:
	{
	  uint8_t buf_idx = instr - OP_BPUSH;
	  DEBUG("[BASIC_LIB] execute: (%d) BPUSH %d\n", context->pc, buf_idx);
	  pushBuffer( eventState, &(s->buffers[buf_idx]));
	  context->pc += 1;
	  break;
	}

      case OP_GETLOCAL + 0: case OP_GETLOCAL + 1: case OP_GETLOCAL + 2:
      case OP_GETLOCAL + 3: case OP_GETLOCAL + 4:
	{
	  uint8_t arg = instr - OP_GETLOCAL;
	  if (eventState->vars[arg].type == DVM_TYPE_FLOAT) {
	    DEBUG("[BASIC_LIB] execute: OPGETLOCAL (%d):: Pushing LSB value %d.\n", (int)arg,(int)eventState->vars[arg+1].value.var);
	    pushOperand( eventState, &eventState->vars[arg+1]);
	  }
	  DEBUG("[BASIC_LIB] execute: OPGETLOCAL (%d):: Pushing value %d.\n", (int)arg,(int)eventState->vars[arg].value.var);
	  pushOperand( eventState, &eventState->vars[arg]);
	  context->pc += 1;
	  break;
	}
      case OP_SETLOCAL + 0: case OP_SETLOCAL + 1: case OP_SETLOCAL + 2:
      case OP_SETLOCAL + 3: case OP_SETLOCAL + 4:
	{
	  uint8_t arg = instr - OP_SETLOCAL;
	  DvmStackVariable* var = popOperand( eventState), *var1 = NULL;
	  DEBUG("[BASIC_LIB] execute: OPSETLOCAL (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
	  eventState->vars[arg] = *var;
	  if (var->type == DVM_TYPE_FLOAT) {
	    var1 = popOperand( eventState);
	    eventState->vars[arg+1] = *var1;
	    DEBUG("[BASIC_LIB] execute: OPSETLOCAL (%d):: Setting LSB value to %d.\n",(int)arg,(int)var1->value.var);
	  }
	  context->pc += 1;
	  break;
	}
      case OP_GETLOCALF + 0: case OP_GETLOCALF + 1: case OP_GETLOCALF + 2:
      case OP_GETLOCALF + 3: case OP_GETLOCALF + 4:
	{	// Only used for type-casting integer to float
	  uint8_t arg = instr - OP_GETLOCALF;
	  int32_t res = 0;
	  uint16_t res_part = 0;
	  DEBUG("[BASIC_LIB] execute: OPGETLOCALF (%d):: Pushing value %d.\n", (int)arg,(int)eventState->vars[arg].value.var);
				
	  if (eventState->vars[arg].type == DVM_TYPE_INTEGER) {
	    res = (int32_t)(eventState->vars[arg].value.var * FLOAT_PRECISION);
	    res_part = res & 0xFFFF;
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT_DEC);
	    res_part = res >> 16;
	    pushValue( eventState, res_part, DVM_TYPE_FLOAT);
	  } else if (eventState->vars[arg].type == DVM_TYPE_FLOAT) {
	    pushOperand( eventState, &eventState->vars[arg+1]);
	    pushOperand( eventState, &eventState->vars[arg]);
	  }
	  context->pc += 1;
	  break;
	}
      case OP_SETLOCALF + 0: case OP_SETLOCALF + 1:
      case OP_SETLOCALF + 2: case OP_SETLOCALF + 3:
	{	// Only used for type-casting integer to float
	  uint8_t arg = instr - OP_SETLOCALF;
	  DvmStackVariable* var = popOperand( eventState);
	  int32_t res = 0;
	  uint16_t res_part;
	  DEBUG("[BASIC_LIB] execute: OPSETLOCALF (%d):: Setting value to %d.\n",(int)arg,(int)var->value.var);
	  if (var->type == DVM_TYPE_INTEGER) {
	    res = (int32_t)(var->value.var * FLOAT_PRECISION);
	    res_part = res & 0xFFFF;
	    eventState->vars[arg+1].type = DVM_TYPE_FLOAT_DEC;
	    eventState->vars[arg+1].value.var = res_part;
	    res_part = res >> 16;
	    eventState->vars[arg].type = DVM_TYPE_FLOAT;
	    eventState->vars[arg].value.var = res_part;
	  } else if (var->type == DVM_TYPE_FLOAT) {
	    DvmStackVariable* var1 = popOperand( eventState);
	    eventState->vars[arg] = *var;
	    eventState->vars[arg+1] = *var1;
	  }
	  context->pc += 1;
	  break;
	}


      case OP_PUSH:
	{
	  context->pc += 1;
	  DvmOpcode arg = getOpcode( dvm_st,  context->which, context->pc);
	  pushValue( eventState, arg, DVM_TYPE_INTEGER);
	  context->pc += 1;
	  DEBUG("[BASIC_LIB] execute: (%i) PUSH %hi\n", (int)context->which, arg);
	  break;
	}
      case OP_PUSHF:
	{
	  uint16_t arg;
	  context->pc += 1;
	  DvmOpcode arg1 = getOpcode( dvm_st,  context->which, context->pc), arg2, arg3, arg4;
	  context->pc += 1;
	  arg2 = getOpcode( dvm_st,  context->which, context->pc);
	  context->pc += 1;
	  arg3 = getOpcode( dvm_st,  context->which, context->pc);
	  context->pc += 1;
	  arg4 = getOpcode( dvm_st,  context->which, context->pc);
	  arg = arg3;
	  arg = (arg << 8) + arg4;
	  pushValue( eventState, arg, DVM_TYPE_FLOAT_DEC);
	  DEBUG("[BASIC_LIB] execute: (%i) PUSHF LSW %d\n", (int)context->which, arg);
	  arg = arg1;
	  arg = (arg << 8) + arg2;
	  pushValue( eventState, arg, DVM_TYPE_FLOAT);
	  DEBUG("[BASIC_LIB] execute: (%i) PUSHF MSW %d \n", (int)context->which, arg);
	  context->pc += 1;
	  break;
	}
      case OP_POP:
	{
	  DEBUG("[BASIC_LIB execute: (%i) POP\n", (int)context->which);
	  popOperand( eventState);
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
	  uint8_t opt = instr - OP_BAPPEND + 1;    
	  uint8_t i = 0;    
	  DvmStackVariable* bufarg;

	  bufarg = popOperand( eventState);     
	  DEBUG("[BASIC_LIB] execute: Before BAPPEND. Buffer size is %d.\n",bufarg->buffer.var->size);
	  for (; i < opt; i++) {   
	    DvmStackVariable* apparg = popOperand( eventState);  
	    if (apparg->type == DVM_TYPE_BUFFER) {
	      DEBUG("[BASIC_LIB] execute: OPBAPPEND (%d) BUFFER\n", opt);
	      buffer_concatenate(bufarg->buffer.var, apparg->buffer.var); 
	    } else if (apparg->type == DVM_TYPE_FLOAT) {  
	      DvmStackVariable* apparg_dec = popOperand( eventState);
	      buffer_append(bufarg->buffer.var, 2, apparg_dec->value.var);
	      buffer_append(bufarg->buffer.var, 2, apparg->value.var);
#ifdef PC_PLATFORM
	      int32_t float_val;
	      float_val = convert_to_float(apparg, apparg_dec);
	      DEBUG("[BASIC_LIB] execute: OPBAPPEND (%d) FLOAT %d\n", opt, float_val);
#endif
	    } else {
	      buffer_append(bufarg->buffer.var, 2, apparg->value.var);   
	      DEBUG("[BASIC_LIB] execute:  OPBAPPEND (%d) INT %d\n", opt, apparg->value.var);
	    }
	  }
	  DEBUG("[BASIC_LIB] execute: After BAPPEND. Buffer size is %d.\n",bufarg->buffer.var->size);
	  pushOperand( eventState, bufarg);        
	  context->pc += 1;                             
	  break;              
	}
      case OP_BCLEAR:                                   
        {                                                 
	  DvmStackVariable* bufarg;
	  DEBUG("[BASIC_LIB] execute: BCLEAR\n");
	  bufarg = popOperand( eventState);    
	  buffer_clear(bufarg->buffer.var);             
	  pushOperand( eventState, bufarg);        
	  context->pc += 1;                             
	  break;                                        
	} 
      case OP_BREADF:
	{                                                 
	  DvmStackVariable* bufarg = popOperand( eventState);
	  DvmStackVariable* indexarg = popOperand( eventState);  
	  uint16_t res_part, res_part2;

	  buffer_get(bufarg->buffer.var, 2, indexarg->value.var, &res_part);
	  DEBUG("[BASIC_LIB] execute:  OPBREADF. Read LSW %d at index %d\n", res_part, indexarg->value.var);
	  buffer_get(bufarg->buffer.var, 2, indexarg->value.var+2, &res_part2);                                                         
	  DEBUG("[BASIC_LIB] execute:  OPBREADF. Read MSW %d at index %d\n", res_part2, (indexarg->value.var+2));
	  //	  DEBUG("[BASIC_LIB] execute:  OPBREADF. Read Float 32-bit %d from index %d\n", (int32_t)(((int32_t)res_part2 << 16) + res_part), indexarg->value.var);
	  pushValue( eventState, res_part, DVM_TYPE_FLOAT_DEC);  
	  pushValue( eventState, res_part2, DVM_TYPE_FLOAT);     
	  context->pc += 1;                             
	  break;                                        
	}
      case OP_BSET:                                     
	{                                                 
	  DvmStackVariable* bufarg = popOperand( eventState);    
	  DvmStackVariable* valuearg = popOperand( eventState);  
	  DvmStackVariable* indexarg = NULL;            

	  if (valuearg->type == DVM_TYPE_FLOAT) {       
	    DvmStackVariable* valuearg1 = popOperand( eventState);
	    indexarg = popOperand( eventState);  
	    buffer_set(bufarg->buffer.var, 2, indexarg->value.var, valuearg1->value.var);                                             
	    DEBUG("[BASIC_LIB] execute:  OPBSET. Float LSW in buffer set to %d at index %d\n", valuearg1->value.var, indexarg->value.var);                                            
	    buffer_set(bufarg->buffer.var, 2, indexarg->value.var+2, valuearg->value.var);                                            
	    DEBUG("[BASIC_LIB] execute:  OPBSET. Float MSW in buffer set to %d at index %d\n", valuearg->value.var, (indexarg->value.var + 2));
	  } else {                                      
	    indexarg = popOperand( eventState);  
	    buffer_set(bufarg->buffer.var, 2, indexarg->value.var, valuearg->value.var);                                              
	    DEBUG("[BASIC_LIB] execute:  OPSET. Integer in buffer set to %d at index %d\n", valuearg->value.var, indexarg->value.var);                                                
	  }                                             

	  context->pc += 1;                             
	  break;                                        
	}
		
      case OP_POSTNET:
	{
	  DvmStackVariable* addarg = popOperand( eventState);
	  DvmStackVariable* modarg = popOperand( eventState);
	  DvmStackVariable* typearg = popOperand( eventState);
	  DvmStackVariable* bufarg = popOperand( eventState);

	  if (bufarg->type == DVM_TYPE_BUFFER) {
	    sys_post_net(modarg->value.var, typearg->value.var, bufarg->buffer.var->size, 
			 bufarg->buffer.var->entries, 0, addarg->value.var);
	  } else if (bufarg->type == DVM_TYPE_FLOAT) {
	    DvmStackVariable* bufarg_dec = popOperand( eventState);
	    s->fl_post = convert_to_float(bufarg, bufarg_dec);
	    sys_post_net(modarg->value.var, typearg->value.var, sizeof(int32_t), 
			 &s->fl_post, 0, addarg->value.var);
	  } else {
	    sys_post_net(modarg->value.var, typearg->value.var, sizeof(int16_t), 
			 &bufarg->value.var, 0, addarg->value.var);
	  }

	  context->pc += 1;
	  break;
	}	
      case OP_BCAST:
	{
	  DvmStackVariable* modarg = popOperand( eventState);
	  DvmStackVariable* typearg = popOperand( eventState);
	  DvmStackVariable* bufarg = popOperand( eventState);

	  if (bufarg->type == DVM_TYPE_BUFFER) {
	    DEBUG("\n\n[BASIC_LIB] (%d): BCAST BUFFER\n\n", context->which);
	    sys_post_net(modarg->value.var, typearg->value.var, bufarg->buffer.var->size, bufarg->buffer.var->entries, 0, BCAST_ADDRESS);
	    //DEBUG("\n\n[BASIC_LIB] (%d): BCAST \n\n", context->which);
	  } else if (bufarg->type == DVM_TYPE_FLOAT) {
	    DEBUG("\n\n[BASIC_LIB] (%d): BCAST FLOAT\n\n", context->which);
	    DvmStackVariable* bufarg_dec = popOperand( eventState);
	    s->fl_post = convert_to_float(bufarg, bufarg_dec);
	    sys_post_net(modarg->value.var, typearg->value.var, sizeof(int32_t), &s->fl_post, 0, BCAST_ADDRESS);
	  } else {
	    DEBUG("\n\n[BASIC_LIB] (%d): BCAST VALUE\n\n", context->which);
	    sys_post_net(modarg->value.var, typearg->value.var, sizeof(int16_t), &bufarg->value.var, 0, BCAST_ADDRESS);
	  }
	  //DEBUG("\n\n[BASIC_LIB] (%d): BCAST \n\n", context->which);

	  context->pc += 1;
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
  uint8_t op = (val >> 3) & 0x03;
  uint8_t led	= val & 0x07;
  DEBUG("[BASIC_LIB] execute:  Executing LED with %d and %d\n", op, led);
  switch (op) 
    {
    case 0:			/* set */
      {
	if (led & 1) 
	  sys_led(LED_RED_ON);
	else 
	  sys_led(LED_RED_OFF);
	if (led & 2)
	  sys_led(LED_GREEN_ON);
	else
	  sys_led(LED_GREEN_OFF);
	if (led & 4)
	  sys_led(LED_YELLOW_ON);
	else
	  sys_led(LED_YELLOW_OFF);
	break;
      }
    case 1:			/* OFF 0 bits */
      {
	if (!(led & 1)) sys_led(LED_RED_OFF);
	if (!(led & 2)) sys_led(LED_GREEN_OFF);
	if (!(led & 4)) sys_led(LED_YELLOW_ON);
	break;
      }
    case 2:			/* on 1 bits */
      {
	if (led & 1) sys_led(LED_RED_ON);
	if (led & 2) sys_led(LED_GREEN_ON);
	if (led & 4) sys_led(LED_YELLOW_ON);
	break;
      }
    case 3:			/* TOGGLE 1 bits */
      {
	if (led & 1) sys_led(LED_RED_TOGGLE);
	if (led & 2) sys_led(LED_GREEN_TOGGLE);
	if (led & 4) sys_led(LED_YELLOW_TOGGLE);
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
static inline int8_t call_op(dvm_state_t* dvm_st, DvmState *eventState, DVMBasiclib_state_t *s) {
  // Result will depend on the function being called, and the 
  // script compiler will take care of pushing the appropriate variable
  
  DvmContext *context = &(eventState->context);
  uint8_t size = 0;
  DvmStackVariable *callArgs = popOperand( eventState);
  DvmStackVariable *retValue = popOperand( eventState);

  if (retValue->type != DVM_TYPE_BUFFER) {DEBUG("\n\n\nPROBLEM IN RET BUFFER\n\n\n");}

  context->pc += 1;
  uint8_t mod_id = getOpcode( dvm_st,  context->which, context->pc);
  context->pc += 1;
  uint8_t fnid = getOpcode( dvm_st,  context->which, context->pc);

  if (sys_fntable_subscribe(mod_id, EXECUTE_SYNCALL, 0) != SOS_OK) {
    DEBUG("\n\n\n\nSUBSCRIPTION PROBLEMS\n\n\n\n\n");
  }

  if (callArgs->type == DVM_TYPE_BUFFER) {
    size = callArgs->buffer.var->size;
    DEBUG("[BASIC_LIB] execute:  OPCALL arg size = %d mod = %d fnid = %d.\n", size, mod_id, fnid);
  } else if (callArgs->type == DVM_TYPE_INTEGER) {
    size = 2;
  } 
  SOS_CALL(dvm_st->execute_syncall, execute_syncall_func_t, fnid, callArgs, size, retValue);
  pushOperand( eventState, retValue);

  context->pc += 1;
  
  return SOS_OK;
}
//--------------------------------------------------------------------
static inline int8_t post_op(dvm_state_t* dvm_st, DvmState *eventState, DVMBasiclib_state_t *s) {
  // Post the stack variable + fnid.
  // The "post stub" should extract and COPY the required information, and post back to itself
  post_msg_type *pmsg = (post_msg_type *)sys_malloc(sizeof(uint8_t)+sizeof(DvmStackVariable*));
  DvmContext *context = &(eventState->context);

  DvmStackVariable *callArgs = popOperand( eventState);
  context->pc += 1;
  uint8_t mod_id = getOpcode( dvm_st, context->which, context->pc);
  context->pc += 1;
  uint8_t fnid = getOpcode( dvm_st, context->which, context->pc);

  pmsg->fnid = fnid;
  pmsg->argBuf = callArgs;

  if (sys_post(mod_id, POST_EXECUTE, sizeof(post_msg_type), pmsg, SOS_MSG_RELEASE) != SOS_OK) {
    //unable to post message for some reason 
    //So, restore the stack condition and PC
    //And, yield the context and put it in ready queue
    pushOperand( eventState, callArgs);
    context->pc -= 2;
    yieldContext(dvm_st, context);
    resumeContext(dvm_st, context, context);
    context->state = DVM_STATE_RUN;
    //Context should be put back in ready queue
    //so that the post operation can be retried
    return -EINVAL;
  }
  //context->state = DVM_STATE_BLOCKED;
  //queue_enqueue( context, &s->blockedQueue, context);
  context->pc += 1;
  DEBUG("[BASIC_LIB] (%d): Posting to tree routing module.\n",context->which);
  return SOS_OK;
}
//--------------------------------------------------------------------
static int8_t tryget(dvm_state_t* dvm_st, DvmContext *context, uint8_t sensor_type, DVMBasiclib_state_t *s) 
{
  if (sys_sensor_get_data(sensor_type) == SOS_OK) {
    s->busy = 1;
    s->executing = context;
    context->state = DVM_STATE_BLOCKED;
    yieldContext(dvm_st, context);
  } else {
    context->state = DVM_STATE_WAITING;
    queue_enqueue( context, &(s->getDataWaitQueue), context);
  }
  return SOS_OK;
}
//--------------------------------------------------------------------
void rebooted(dvm_state_t* dvm_st) 
{
  DVMBasiclib_state_t *s = (&dvm_st->basiclib_st);
  int i;

  queue_init(&(s->getDataWaitQueue));

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
  int32_t float_val;
  float_val = (((uint32_t)arg1->value.var << 16) & 0xFFFF0000)|(arg2->value.var & 0xFFFF);
  return float_val;
  /*

  uint32_t res = (uint16_t)arg1->value.var;
  res <<= 16;
  res += (uint16_t)arg2->value.var;
  return (int32_t)res;
  */
}
//--------------------------------------------------------------------------------------------  
// The following code is used to debug the
/*
#ifdef OUTLIER_SCRIPT_DBG
#ifdef PC_PLATFORM
    if (148 == context->pc){
      DvmDataBuffer* dbuff;
      int32_t *ratings, *readings;
      int32_t i, j;
      printf("#####################################################\n");
      dbuff = &(s->buffers[2]);
      ratings = (int32_t*)dbuff->entries;
      printf("Ratings: ");
      for (i = 0; i < 4; i++)
	printf("%3d ", ratings[i]);
      printf("\n");
      dbuff = &(s->buffers[0]);
      readings = (int32_t*)dbuff->entries;
      printf("Distance: ");
      for (i = 0; i < 4; i++){
	for (j = 0; j < 4; j++){
	  printf("dist [%d][%d] = %4d, ", i, j , readings[i*4+j]);
	}
	printf("\n");
      }
      printf("#####################################################\n");
    }
    if (51 == context->pc){
      printf("#####################################################\n");
      printf("Counter j\n");
    }
    if (57 == context->pc){
      printf("#####################################################\n");
      printf("Counter i\n");
    }
#endif
#endif
*/
