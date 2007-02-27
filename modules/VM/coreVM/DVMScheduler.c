/**
 * \file DVMScheduler.c
 * \author Rahul Balani
 * \author Ram Kumar - Port to sos-2.x
 */

#include <VM/dvm.h>
#include <VM/DvmConstants.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMqueue.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMBasiclib.h>
#include <VM/dvm_types.h>
#include <led.h>
#include <sys_module.h>


typedef int8_t (*execute_lib_func_t)(func_cb_ptr cb, DvmContext* context, DvmOpcode instr);

//------------------------------------------------------------------------
// STATIC FUNCTION FUNCTIONS
//------------------------------------------------------------------------
static inline int8_t computeInstruction(dvm_state_t *dvm_st);
static int8_t executeContext(dvm_state_t *dvm_st, DvmContext* context);
static int8_t opdone(dvm_state_t *dvm_st);

//------------------------------------------------------------------------
// MESSAGE HANDLERS
//------------------------------------------------------------------------
//  case MSG_INIT: 
int8_t dvmsched_init(dvm_state_t* dvm_st, Message *msg){
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  s->flags.inErrorState = FALSE;
  s->flags.halted = FALSE;
  s->flags.taskRunning = FALSE;
  s->flags.errorFlipFlop = 0;
  s->libraries = 1;	             //Basic library is already there.
  s->runningContext = NULL;
  DEBUG("DVM ENGINE: Initialized\n");
  engineReboot(dvm_st);
  return SOS_OK;
}
//------------------------------------------------------------------------
// case MSG_FINAL:
int8_t dvmsched_final(dvm_state_t* dvm_st, Message *msg)
{
  return SOS_OK;
}
//------------------------------------------------------------------------
//  case MSG_RUN_TASK:
int8_t dvmsched_run_task(dvm_state_t* dvm_st, Message *msg)
{
  opdone(dvm_st);
  return SOS_OK;
}
//------------------------------------------------------------------------
//  case MSG_HALT:
int8_t dvmsched_halt(dvm_state_t* dvm_st, Message *msg)
{
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  DEBUG("DVM ENGINE: DvmEngineM halted.\n");
  s->flags.halted = TRUE;
  return SOS_OK;
}
//------------------------------------------------------------------------
//  case RESUME:
int8_t dvmsched_resume(dvm_state_t* dvm_st, Message *msg)
{
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  s->flags.halted = FALSE;
  DEBUG("DVM ENGINE: DvmEngineM resumes....\n");
  if (!s->flags.taskRunning) {
    s->flags.taskRunning = TRUE;
    DEBUG("DVM ENGINE: MSG_RUN_TASK posted...\n");
    opdone(dvm_st);
  }
  return SOS_OK;
}
//------------------------------------------------------------------------
//  case MSG_TIMER_TIMEOUT:
int8_t dvmsched_timeout(dvm_state_t* dvm_st, Message *msg)
{
  // Periodic error broadcast
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  MsgParam *t = (MsgParam *)msg->data;
  if (t->byte == ERROR_TIMER) {
    DEBUG("DVM ENGINE: VM: ERROR\n");
    if (!s->flags.inErrorState) {
      sys_timer_stop(ERROR_TIMER);
      return -EINVAL;
    }
    if (s->flags.errorFlipFlop) {
      sys_post_uart(DVM_MODULE_PID, DVM_ERROR_MSG, sizeof(DvmErrorMsg), &(s->errorMsg), 0, UART_ADDRESS);
    } else {
      // TODO: Need to figure out what this is...
      //sys_post_net(M_VIRUS, DVM_ERROR_MSG, sizeof(DvmErrorMsg), &(s->errorMsg), 0, BCAST_ADDRESS);
    }
    s->flags.errorFlipFlop = !s->flags.errorFlipFlop;
  }
  return SOS_OK;
}
//------------------------------------------------------------------------
//  case MSG_ADD_LIBRARY:
int8_t dvmsched_add_lib(dvm_state_t* dvm_st, Message *msg)
{
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  MsgParam *param = (MsgParam *)msg->data;
  uint8_t lib_id = param->byte;
  uint8_t lib_bit = 0x1 << lib_id;
  s->libraries |= lib_bit;
  sys_fntable_subscribe(msg->sid, EXECUTE, lib_id); 
  DEBUG("DVM_ENGINE : Adding library id %d bit %d. So libraries become %02x.\n",lib_id,lib_bit,s->libraries);
  engineReboot(dvm_st);
  return SOS_OK;
}
//------------------------------------------------------------------------
//  case MSG_REMOVE_LIBRARY:
int8_t dvmsched_rem_lib(dvm_state_t* dvm_st, Message *msg)
{
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  MsgParam *param = (MsgParam *)msg->data;
  uint8_t lib_id = param->byte;
  uint8_t lib_bit = 0x1 << lib_id;
  s->libraries ^= lib_bit;	//XOR
  DEBUG("DVM_ENGINE : Removing library id %d bit %d. So libraries become %02x.\n",lib_id,lib_bit,s->libraries);
  engineReboot(dvm_st);
  return SOS_OK;
}
//------------------------------------------------------------------------
// STATIC FUNCTIONS
//------------------------------------------------------------------------
static int8_t opdone(dvm_state_t *dvm_st) 
{
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
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
    queue_enqueue(s->runningContext, &(s->runQueue), s->runningContext);
    s->runningContext = NULL;
  }

  if (s->runningContext == NULL && !s->flags.inErrorState && !queue_empty(&s->runQueue) ) 
    {
      DEBUG("DVM ENGINE: Dequeue a new context.\n");
      s->runningContext = queue_dequeue(NULL, &s->runQueue);
      s->runningContext->num_executed = 0;
      s->runningContext->state = DVM_STATE_RUN;
    }
  if (s->runningContext != NULL) {
    if (s->runningContext->state != DVM_STATE_RUN) {
      //Context was halted by library
      //Release the CPU
      s->runningContext = NULL;
      if (sys_post_value(DVM_MODULE_PID, MSG_RUN_TASK, 0, 0) != SOS_OK) {
	s->flags.taskRunning = FALSE;
      }
    } else {
      computeInstruction(dvm_st);
    }
  } else {
    DEBUG("DVM ENGINE: Running_context was NULL\n");
    s->flags.taskRunning = FALSE;
  }
  DEBUG("DVM ENGINE: opdone call over\n");
  return SOS_OK;
}	
//------------------------------------------------------------------------
static inline int8_t computeInstruction(dvm_state_t *dvm_st) {
  int8_t r;
  DVMScheduler_state_t *s = &(dvm_st->sched_st);  
  DvmContext *context = s->runningContext;
  DvmOpcode instr = getOpcode(dvm_st, context->which, context->pc);
  if (context->state != DVM_STATE_RUN) {
    return -EINVAL;
  }
  DEBUG("DVM_ENGINE: Inside compute_instruction \n");
  if (context->pc == 0) {
    uint8_t libMask = getLibraryMask(dvm_st, context->which);
    if ((s->libraries & libMask) != libMask) {
      DEBUG("DVM_ENGINE: Library missing for context %d. Halting execution.\n", context->which);
      haltContext(dvm_st, context);
      s->runningContext = NULL;
      sys_post_value(DVM_MODULE_PID, MSG_RUN_TASK, 0, 0);
      return -EINVAL;
    }
  }
  while ((context->state == DVM_STATE_RUN) && (context->num_executed < DVM_CPU_SLICE)) {
    if (instr & LIB_ID_BIT) {	//Extension Library
      uint8_t library_mod = (instr ^ LIB_ID_BIT) >> EXT_LIB_OP_SHIFT;
      r = SOS_CALL(s->ext_execute[library_mod], execute_lib_func_t, s->runningContext, instr);
      context->num_executed++;
      //if (r < 0) signal error, halt context, break
    } else {			//Basic Library
      DEBUG("DVM_ENGINE: Library being called is %d\n", DVM_MODULE_PID);
      r = execute(dvm_st, getStateBlock(dvm_st, context->which));
      //if (r < 0) signal error, halt context, break
    }
    instr = getOpcode(dvm_st, context->which, context->pc);
  }
  if (context->num_executed >= DVM_CPU_SLICE)
    sys_post_value(DVM_MODULE_PID, MSG_RUN_TASK, 0, 0);
  else if (context->state != DVM_STATE_RUN)
    return opdone(dvm_st);
  return SOS_OK;
}
//------------------------------------------------------------------------
static int8_t executeContext(dvm_state_t *dvm_st, DvmContext* context)
{
  DVMScheduler_state_t *s = &(dvm_st->sched_st);
  if (context->state != DVM_STATE_READY) {
    DEBUG("DVM_ENGINE: Failed to submit context %i: not in READY state.\n", (int)context->which);
    return -EINVAL;
  }  
  queue_enqueue(context, &s->runQueue, context);
  if (!s->flags.taskRunning) {
    s->flags.taskRunning = TRUE;
    DEBUG("DVM_ENGINE: Executing context.. Posting run task.\n");
    sys_post_value(DVM_MODULE_PID, MSG_RUN_TASK, 0, 0); 
  }
  return SOS_OK;
}
//------------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//------------------------------------------------------------------------
void engineReboot(dvm_state_t* dvm_st) 
{
  DvmCapsuleID id;
  DVMScheduler_state_t *s = &(dvm_st->sched_st);

  DEBUG("DVM ENGINE: VM: Dvm rebooting.\n");
  s->runningContext = NULL;
  
  queue_init(&s->runQueue);
  
  synch_reset(dvm_st);
  
  for (id = 0; id < DVM_CAPSULE_NUM; id++) {
    clearAnalysis(dvm_st, id);
  }
  DEBUG("DVM ENGINE: VM: Analyzing lock sets.\n");
  for (id = 0; id < DVM_CAPSULE_NUM; id++) {
    analyzeVars(dvm_st, id);
  }
  s->flags.inErrorState = FALSE;
  s->flags.halted = FALSE;
  
  DEBUG("DVM ENGINE: VM: Signaling reboot to libraries.\n");    
  rebooted(dvm_st);

  sys_led(LED_RED_OFF);
  sys_led(LED_GREEN_OFF);
  sys_led(LED_YELLOW_OFF);
}
//------------------------------------------------------------------------
int8_t scheduler_submit(dvm_state_t* dvm_st, DvmContext* context) 
{
  DEBUG("DVM_ENGINE: VM: Context %i submitted to run.\n", (int)context->which);
  context->state = DVM_STATE_READY;
  return executeContext(dvm_st, context);
}
//------------------------------------------------------------------------
int8_t error(DvmContext* context, uint8_t cause) 
{
#ifdef PC_PLATFORM
  fprintf(stderr, "[SCHEDULER] error\n");
  exit(EXIT_FAILURE);
#endif
  dvm_state_t* dvm_st = sys_get_state();
  DVMScheduler_state_t *s = &(dvm_st->sched_st);
  s->flags.inErrorState = TRUE;
  DEBUG("DVM_ENGINE: VM: Entering ERROR state. Context: %i, cause %i\n", (int)context->which, (int)cause);
  sys_timer_start(ERROR_TIMER, 1000, TIMER_REPEAT);
  if (context != NULL) {
    s->errorMsg.context = context->which;
    s->errorMsg.reason = cause;
    s->errorMsg.capsule = context->which;
    s->errorMsg.instruction = context->pc - 1;
    s->errorMsg.me = sys_id();
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
//------------------------------------------------------------------------
