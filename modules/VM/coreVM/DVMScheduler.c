/**
 * \file DVMScheduler.c
 * \author Rahul Balani
 * \author Ram Kumar - Port to sos-2.x
 */
//------------------------------------------------------------------------
// INCLUDES
//------------------------------------------------------------------------
#include <VM/DVMScheduler.h>
#include <VM/DVMqueue.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMBasiclib.h>

//------------------------------------------------------------------------
// TYPEDEFS
//------------------------------------------------------------------------
typedef int8_t (*execute_lib_func_t)(func_cb_ptr cb, DvmContext* context, DvmOpcode instr);

typedef struct _str_dvm_state {
  DVMScheduler_state_t sched_st;
  DVMResourceManager_state_t resmgr_st;
  DVMEventHandler_state_t evhdlr_st;
  DVMConcurrencyMngr_state_t conmgr_st;
} dvm_state_t;


//------------------------------------------------------------------------
// GLOBAL STATE
//------------------------------------------------------------------------
static DVMScheduler_state_t sched_state;
//------------------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//------------------------------------------------------------------------
static inline int8_t computeInstruction(DVMScheduler_state_t *s) ;
static int8_t executeContext(DvmContext* context, DVMScheduler_state_t *s) ;
static int8_t opdone(DVMScheduler_state_t *s) ;
static int8_t vm_scheduler(void *state, Message *msg) ;
//------------------------------------------------------------------------
// MODULE HEADER
//------------------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = 
  {
    .mod_id         =   DVM_MODULE,
    .code_id        =   ehtons(DVM_MODULE),
    .platform_type  =   HW_TYPE,
    .processor_type =   MCU_TYPE,
    .state_size     =   sizeof(DVMScheduler_state_t),
    .num_sub_func   =    4,
    .num_prov_func  =    0,
    .module_handler =    vm_scheduler,
    .funct          = {
      {error_8, "czy2", M_EXT_LIB, EXECUTE},
      {error_8, "czy2", M_EXT_LIB, EXECUTE},
      {error_8, "czy2", M_EXT_LIB, EXECUTE},
      {error_8, "czy2", M_EXT_LIB, EXECUTE},
    },
  };

//------------------------------------------------------------------------
// STATIC FUNCTIONS
//------------------------------------------------------------------------
// Ram - This is a super message handler that dispatches messages to the
// rest of the message handlers in the VM system
static int8_t vm_scheduler(void *state, Message *msg)
{
  dvm_state_t* dvm_st = (dvm_state_t*)state; 
  DVMScheduler_state_t *s = &dvm_st->sched_st;
  switch (msg->type){
  case MSG_INIT: 
    {
      DEBUG("Starting VM init\n");
      resmanager_handler(&dvm_st->resmgr_st, msg);
      event_handler(&dvm_st->evhdlr_st, msg);
      concurrency_handler(&dvm_st->conhdlr_st, msg);      

      s->flags.inErrorState = FALSE;
      s->flags.halted = FALSE;
      s->flags.taskRunning = FALSE;
      s->flags.errorFlipFlop = 0;
      s->libraries = 1;	             //Basic library is already there.
      s->runningContext = NULL;

      DEBUG("Scheduler manager initialized\n");
      DEBUG("DVM ENGINE: Dvm initializing DONE.\n");
      engineReboot();
      
      return SOS_OK;
    }
  case MSG_FINAL:
    {
      concurrency_handler(&dvm_st->conhdlr_st, msg);      
      event_handler(&dvm_st->evhdlr_st, msg);
      resmanager_handler(&dvm_st->resmgr_st, msg);
      DEBUG("DVM ENGINE: VM: Stopping.\n");
      return SOS_OK;
    }
  case MSG_RUN_TASK:
    {
      return opdone(s);
    }
  case MSG_HALT:
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
	DEBUG("DVM ENGINE: MSG_RUN_TASK posted...\n");
	opdone(s);
      }
      return SOS_OK;
    }
  case MSG_TIMER_TIMEOUT:
    {
      // Ram - For periodic error handler
      MsgParam *t = (MsgParam *)msg->data;

      if (t->byte == ERROR_TIMER) {
	DEBUG("DVM ENGINE: VM: ERROR\n");
	if (!s->flags.inErrorState) {
	  sys_timer_stop(ERROR_TIMER);
	  return -EINVAL;
	}
	if (s->flags.errorFlipFlop) {
	  sys_post_uart(DVM_MODULE, DVM_ERROR_MSG, sizeof(DvmErrorMsg), &(s->errorMsg), 0, UART_ADDRESS);
	} else {
	  // TODO: Need to figure out what this is...
	  //sys_post_net(M_VIRUS, DVM_ERROR_MSG, sizeof(DvmErrorMsg), &(s->errorMsg), 0, BCAST_ADDRESS);
	}
	s->flags.errorFlipFlop = !s->flags.errorFlipFlop;
	return SOS_OK;
      }
      else{
	event_handler(&dvm_st->evhdlr_st, msg);
      }
      break;
    }
  case MSG_ADD_LIBRARY:
    {
      MsgParam *param = (MsgParam *)msg->data;
      uint8_t lib_id = param->byte;
      uint8_t lib_bit = 0x1 << lib_id;
      s->libraries |= lib_bit;
      sys_fntable_subscribe(msg->sid, EXECUTE, lib_id); 
      DEBUG("DVM_ENGINE : Adding library id %d bit %d. So libraries become %02x.\n",lib_id,lib_bit,s->libraries);
      concurrency_handler(&dvm_st->conhdlr_st, msg);      
      engineReboot();
      return SOS_OK;
    }
  case MSG_REMOVE_LIBRARY:
    {
      MsgParam *param = (MsgParam *)msg->data;
      uint8_t lib_id = param->byte;
      uint8_t lib_bit = 0x1 << lib_id;
      s->libraries ^= lib_bit;	//XOR
      DEBUG("DVM_ENGINE : Removing library id %d bit %d. So libraries become %02x.\n",lib_id,lib_bit,s->libraries);
      concurrency_handler(&dvm_st->conhdlr_st, msg);      
      engineReboot();
      return SOS_OK;
    }
    /*
#ifdef PC_PLATFORM
  case MSG_FROM_USER:
    {
      DEBUG("DVM ENGINE: Removing Math library\n");
      ker_deregister_module(M_MATH_LIB);
    }
#endif
    */
  default:
    break;
  }
  return SOS_OK;
}
//------------------------------------------------------------------------
static int8_t opdone(DVMScheduler_state_t *s) 
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
    queue_enque(s->runningContext, &(s->runQueue), s->runningContext);
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
      if (sys_post_value(DVM_MODULE, MSG_RUN_TASK, 0, 0) != SOS_OK) {
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
//------------------------------------------------------------------------
static inline int8_t computeInstruction(DVMScheduler_state_t *s) {
  int8_t r;
  DvmContext *context = s->runningContext;
  DvmOpcode instr = getOpcode(context->which, context->pc);
  if (context->state != DVM_STATE_RUN) {
    return -EINVAL;
  }
  DEBUG("DVM_ENGINE: Inside compute_instruction \n");
  if (context->pc == 0) {
    uint8_t libMask = getLibraryMask(context->which);
    if ((s->libraries & libMask) != libMask) {
      DEBUG("DVM_ENGINE: Library missing for context %d. Halting execution.\n", context->which);
      haltContext(context);
      s->runningContext = NULL;
      sys_post_value(DVM_MODULE, MSG_RUN_TASK, 0, 0);
      return -EINVAL;
    }
  }
  while ((context->state == DVM_STATE_RUN) && (context->num_executed < DVM_CPU_SLICE)) {
    if (instr & LIB_ID_BIT) {	//Extension Library
      uint8_t library_mod = (instr ^ LIB_ID_BIT) >> EXT_LIB_OP_SHIFT;
      __asm __volatile("st_call1:");
      r = SOS_CALL(s->ext_execute[library_mod], execute_lib_func_t, s->runningContext, instr);
      __asm __volatile("en_call1:");
      context->num_executed++;
      //if (r < 0) signal error, halt context, break
    } else {			//Basic Library
      DEBUG("DVM_ENGINE: Library being called is %d\n", M_BASIC_LIB);
      r = execute(getStateBlock(context->which));
      //if (r < 0) signal error, halt context, break
    }
    instr = getOpcode(context->which, context->pc);
  }
  if (context->num_executed >= DVM_CPU_SLICE)
    sys_post_value(DVM_MODULE, MSG_RUN_TASK, 0, 0);
  else if (context->state != DVM_STATE_RUN)
    return opdone(s);
  return SOS_OK;
}
//------------------------------------------------------------------------
static int8_t executeContext(DvmContext* context, DVMScheduler_state_t *s) {
  if (context->state != DVM_STATE_READY) {
    DEBUG("DVM_ENGINE: Failed to submit context %i: not in READY state.\n", (int)context->which);
    return -EINVAL;
  }  
  queue_enque(context, &s->runQueue, context);
  if (!s->flags.taskRunning) {
    s->flags.taskRunning = TRUE;
    DEBUG("DVM_ENGINE: Executing context.. Posting run task.\n");
    sys_post_value(DVM_MODULE, MSG_RUN_TASK, 0, 0); 
  }
  return SOS_OK;
}
//------------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//------------------------------------------------------------------------
void engineReboot(void) 
{
  DVMScheduler_state_t *s = (DVMScheduler_state_t*)sys_get_module_state(DVM_MODULE);
  DvmCapsuleID id;

  DEBUG("DVM ENGINE: VM: Dvm rebooting.\n");
  s->runningContext = NULL;
  
  queue_init(&s->runQueue);
  
  synch_reset();
  
  for (id = 0; id < DVM_CAPSULE_NUM; id++) {
    clearAnalysis(id);
  }
  DEBUG("DVM ENGINE: VM: Analyzing lock sets.\n");
  for (id = 0; id < DVM_CAPSULE_NUM; id++) {
    analyzeVars(id);
  }
  s->flags.inErrorState = FALSE;
  s->flags.halted = FALSE;
  
  DEBUG("DVM ENGINE: VM: Signaling reboot to libraries.\n");    
  rebooted();

  sys_led(LED_RED_OFF);
  sys_led(LED_GREEN_OFF);
  sys_led(LED_YELLOW_OFF);
}
//------------------------------------------------------------------------
int8_t scheduler_submit(DvmContext* context) 
{
  DVMScheduler_state_t *s = (DVMScheduler_state_t*)sys_get_module_state(DVM_MODULE);
  DEBUG("DVM_ENGINE: VM: Context %i submitted to run.\n", (int)context->which);
  context->state = DVM_STATE_READY;
  return executeContext(context, s);
}
//------------------------------------------------------------------------
int8_t error(DvmContext* context, uint8_t cause) 
{
  DVMScheduler_state_t *s = (DVMScheduler_state_t*)sys_get_module_state(DVM_MODULE);
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
