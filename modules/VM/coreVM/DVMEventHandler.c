/**
 * \file DVMEventHandler.c
 * \author Rahul Balani
 * \author Ilias Tsigkogiannis
 * \author Ram Kumar - Port to sos-2.x
 */

#include <VM/DVMEventHandler.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMResourceManager.h>
//--------------------------------------------------------------------
// TYPEDEFS
//--------------------------------------------------------------------
typedef struct {
  sos_pid_t pid;
  DvmState* stateBlock[DVM_CAPSULE_NUM];
} app_state;
//--------------------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------------------
static inline void rebootContexts(app_state *s) ;
static int8_t event_handler(void *state, Message *msg) ;
//--------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//--------------------------------------------------------------------
static int8_t initEventHandler(func_cb_ptr p, DvmState *eventState, uint8_t capsuleID);
static DvmCapsuleLength getCodeLength(func_cb_ptr p, uint8_t id); 
static uint8_t getLibraryMask(func_cb_ptr p, uint8_t id);
static DvmOpcode getOpcode(func_cb_ptr p, uint8_t id, uint16_t which);
static DvmState *getStateBlock (func_cb_ptr p, uint8_t id); 
//--------------------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id        = M_HANDLER_STORE,
  .code_id       = ehtons(M_HANDLER_STORE),
  .platform_type  = HW_TYPE,    
  .processor_type = MCU_TYPE,
  .state_size    = 0,
  .num_sub_func  = 0,//9
  .num_prov_func = 0,//5
  //    .num_timers    = 8,
  .module_handler = event_handler,
  .funct          = {},
};
//--------------------------------------------------------------------
static int8_t event_handler(void *state, Message *msg) {
  app_state *s = (app_state *) state;                       
  switch (msg->type) {
  case MSG_INIT: 
    {
      s->pid = msg->did;
      memset(s->stateBlock, 0, DVM_CAPSULE_NUM*sizeof(void*));
      DEBUG("HANDLER STORE: Initialized\n");
      break;
    }
  case MSG_FINAL: 
    {
      uint8_t i = 0;
      for (; i < DVM_CAPSULE_NUM; i++)
	mem_freeDL(s->mem_free, i);
      break;
    }
  case MSG_TIMER_TIMEOUT:
    {
      MsgParam *timerID = (MsgParam *)msg->data;
      DEBUG("EVENT HANDLER: TIMER %d EXPIRED\n", timerID->byte);
      if ((s->stateBlock[timerID->byte] != NULL) && (s->stateBlock[timerID->byte]->context.moduleID == TIMER_PID)
	  && (s->stateBlock[timerID->byte]->context.type == MSG_TIMER_TIMEOUT)) {
	initializeContextDL(s->ctx_init, &s->stateBlock[timerID->byte]->context);
	resumeContextDL(s->ctx_resume, &s->stateBlock[timerID->byte]->context, &s->stateBlock[timerID->byte]->context);
      }
      break;
    }
  default:
    {
      __asm __volatile("st_sos2:");
      uint8_t i;
      for (i = 0; i < DVM_CAPSULE_NUM; i++) {
	if ((s->stateBlock[i] != NULL) && (s->stateBlock[i]->context.moduleID == msg->sid)
	    && (s->stateBlock[i]->context.type == msg->type)) {
	  if (s->stateBlock[i]->context.state == DVM_STATE_HALT) {
	    DvmStackVariable *stackArg;
	    initializeContextDL(s->ctx_init, &s->stateBlock[i]->context);
	    stackArg = (DvmStackVariable *)ker_msg_take_data(s->pid, msg);
	    if (stackArg != NULL)
	      pushOperandDL(s->push_operand, s->stateBlock[i], stackArg);
	    resumeContextDL(s->ctx_resume, &s->stateBlock[i]->context, &s->stateBlock[i]->context);
	  }
	}
      }
      break;
    }
  }
    
  return SOS_OK;
}
//--------------------------------------------------------------------
int8_t initEventHandler(DvmState *eventState, uint8_t capsuleID) 
{
  app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);

  if (capsuleID >= DVM_CAPSULE_NUM) {return -EINVAL;}
  s->stateBlock[capsuleID] = eventState;

  analyzeVarsDL(s->analyzeVars, capsuleID);
  initializeContextDL(s->ctx_init, &s->stateBlock[capsuleID]->context);
  {
#ifdef PC_PLATFORM
    int i;
    DEBUG("EventHandler: Installing capsule %d:\n\t", (int)capsuleID);
    for (i = 0; i < s->stateBlock[capsuleID]->context.dataSize; i++) {
      DEBUG_SHORT("[%hhx]", getOpcode(capsuleID, i));
    }
    DEBUG_SHORT("\n");
#endif
  }
  engineRebootDL(s->reboot);
  rebootContexts(s);
  if (s->stateBlock[capsuleID]->context.moduleID == TIMER_PID) {
    ker_timer_init(s->pid, capsuleID, TIMER_REPEAT);
    DEBUG("VM (%d): TIMER INIT\n", capsuleID);
  }
  if (s->stateBlock[DVM_CAPSULE_REBOOT] != NULL) {
    initializeContextDL(s->ctx_init, &(s->stateBlock[DVM_CAPSULE_REBOOT]->context));
    resumeContextDL(s->ctx_resume, &s->stateBlock[DVM_CAPSULE_REBOOT]->context, &s->stateBlock[DVM_CAPSULE_REBOOT]->context);
  }
  return SOS_OK;
}
//--------------------------------------------------------------------
DvmCapsuleLength getCodeLength(uint8_t id) 
{
  app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);

  if (s->stateBlock[id] != NULL)
    return s->stateBlock[id]->context.dataSize;
  else
    return 0;
}
//--------------------------------------------------------------------
uint8_t getLibraryMask(uint8_t id) 
{
  app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
  if (s->stateBlock[id] != NULL)
    return s->stateBlock[id]->context.libraryMask;
  else
    return 0;
}
//--------------------------------------------------------------------	
DvmOpcode getOpcode(uint8_t id, uint16_t which) 
{
  app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);

  if (s->stateBlock[id] != NULL) {
    DvmState *ds = s->stateBlock[id];
    DvmOpcode op;
    // get the opcode at index "which" from codemem and return it.
    // handler for codemem is in s->stateBlock[id]->script
    if(ker_codemem_read(ds->cm, M_HANDLER_STORE,
			&(op), sizeof(op), offsetof(DvmScript, data) + which) == SOS_OK) {
      DEBUG("Event Handler: getOpcode. correct place.\n");
      return op;
    }
    return OP_HALT;
  } else {
    return OP_HALT;
  }
}
//--------------------------------------------------------------------	
DvmState *getStateBlock (uint8_t id) 
{
  app_state *s = (app_state *) ker_get_module_state(M_HANDLER_STORE);
  if (s->stateBlock[id] != NULL) {
    return s->stateBlock[id];
  }
  return NULL;
}
//--------------------------------------------------------------------	
static inline void rebootContexts(app_state *s)
{
  int i, j;
  for (i = 0; i < DVM_CAPSULE_NUM; i++) {
    if (s->stateBlock[i] != NULL) {
      for (j = 0; j < DVM_NUM_LOCAL_VARS; j++) {
	s->stateBlock[i]->vars[j].type = DVM_TYPE_INTEGER;
	s->stateBlock[i]->vars[j].value.var = 0;
      }
      if (s->stateBlock[i]->context.state != DVM_STATE_HALT)
	haltContextDL(s->ctx_halt, &s->stateBlock[i]->context);
    }
  }
}
//--------------------------------------------------------------------	
