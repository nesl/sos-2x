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
typedef int8_t (*func_i8z_t)(func_cb_ptr cb, DvmContext* context, DvmOpcode instr);

typedef struct {
  func_cb_ptr ext_execute[4];
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

//------------------------------------------------------------------------
// GLOBAL STATE
//------------------------------------------------------------------------
//static sos_module_t sched_module;
static app_state sched_state;
//------------------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//------------------------------------------------------------------------
static inline int8_t computeInstruction(app_state *s) ;
static int8_t executeContext(DvmContext* context, app_state *s) ;
static int8_t opdone(app_state *s) ;
static int8_t vm_scheduler(void *state, Message *msg) ;
//------------------------------------------------------------------------
// EXTERNAL FUNCTION PROTOTYPES (Need to move to the header file)
//------------------------------------------------------------------------
void engineReboot(); 
int8_t scheduler_submit(DvmContext* context);
int8_t error(DvmContext* context, uint8_t cause); 
//------------------------------------------------------------------------
// MODULE HEADER
//------------------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = 
  {
    .mod_id         =   DVM_ENGINE_M,
    .code_id        =   ehtons(DVM_ENGINE_M),
    .platform_type  =   HW_TYPE,
    .processor_type =   MCU_TYPE,
    .state_size     =   sizeof(app_state),
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
static int8_t vm_scheduler(void *state, Message *msg)
{
  app_state *s = (app_state*) state;
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
      engineReboot();
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
      engineReboot();
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
//------------------------------------------------------------------------
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
//------------------------------------------------------------------------
static inline int8_t computeInstruction(app_state *s) {
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
      r = execute(getStateBlock(context->which));
      //if (r < 0) signal error, halt context, break
    }
    instr = getOpcode(context->which, context->pc);
  }
  if (context->num_executed >= DVM_CPU_SLICE)
    post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0);
  else if (context->state != DVM_STATE_RUN)
    return opdone(s);
  return SOS_OK;
}
//------------------------------------------------------------------------
static int8_t executeContext(DvmContext* context, app_state *s) {
  if (context->state != DVM_STATE_READY) {
    DEBUG("DVM_ENGINE: Failed to submit context %i: not in READY state.\n", (int)context->which);
    return -EINVAL;
  }  
  queue_enque(context, &s->runQueue, context);
  if (!s->flags.taskRunning) {
    s->flags.taskRunning = TRUE;
    DEBUG("DVM_ENGINE: Executing context.. Posting run task.\n");
    post_short(s->pid, s->pid, RUN_TASK, 0, 0, 0); 
  }
  return SOS_OK;
}
//------------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//------------------------------------------------------------------------
void engineReboot(void) 
{
  app_state *s = (app_state*)ker_get_module_state(DVM_ENGINE_M);
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

  ker_led(LED_RED_OFF);
  ker_led(LED_GREEN_OFF);
  ker_led(LED_YELLOW_OFF);
}
//------------------------------------------------------------------------
int8_t scheduler_submit(DvmContext* context) 
{
  app_state *s = (app_state*)ker_get_module_state(DVM_ENGINE_M);
  DEBUG("DVM_ENGINE: VM: Context %i submitted to run.\n", (int)context->which);
  context->state = DVM_STATE_READY;
  return executeContext(context, s);
}
//------------------------------------------------------------------------
int8_t error(DvmContext* context, uint8_t cause) 
{
  app_state *s = (app_state*)ker_get_module_state(DVM_ENGINE_M);
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
//------------------------------------------------------------------------
