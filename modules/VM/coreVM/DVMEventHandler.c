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
#include <sys_module.h>
#include <led.h>

//--------------------------------------------------------------------
// STATIC FUNCTION DEFINITIONS
//--------------------------------------------------------------------
static inline void rebootContexts(dvm_state_t *dvms_st) ;
//--------------------------------------------------------------------
// MESSAGE HANDLERS
//--------------------------------------------------------------------
//  case MSG_INIT: 
int8_t event_handler_init(dvm_state_t *dvm_st, Message *msg)
{
  DEBUG("EV HDLR: Start init\n");
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);                       
  memset(s->stateBlock, 0, DVM_CAPSULE_NUM*sizeof(void*));
  DEBUG("EV HDLR: Initialized\n");
  return SOS_OK;
}
//--------------------------------------------------------------------
//  case MSG_FINAL: 
int8_t event_handler_final(dvm_state_t *dvm_st, Message *msg)
{
  uint8_t i = 0;
  for (; i < DVM_CAPSULE_NUM; i++)
    mem_free(dvm_st, i);
  return SOS_OK;
}
//--------------------------------------------------------------------
//  case MSG_TIMER_TIMEOUT:
int8_t event_handler_timeout(dvm_state_t *dvm_st, Message *msg)
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);                       
  MsgParam *timerID = (MsgParam *)msg->data;
  DEBUG("********************************************\n");
  DEBUG("EV HDLR: TIMER %d EXPIRED\n", timerID->byte);
  sys_led(LED_GREEN_TOGGLE);
  if ((s->stateBlock[timerID->byte] != NULL) && (s->stateBlock[timerID->byte]->context.moduleID == TIMER_PID)
      && (s->stateBlock[timerID->byte]->context.type == MSG_TIMER_TIMEOUT)) {
    initializeContext(dvm_st, &s->stateBlock[timerID->byte]->context);
    resumeContext(dvm_st, &s->stateBlock[timerID->byte]->context, &s->stateBlock[timerID->byte]->context);
  }
  return SOS_OK;
}
//--------------------------------------------------------------------
//  default: User-defined Events are handled in default
int8_t event_handler_default(dvm_state_t *dvm_st, Message *msg)
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);                       
  __asm __volatile("st_sos2:");
  uint8_t i;
  for (i = 0; i < DVM_CAPSULE_NUM; i++) {
    if ((s->stateBlock[i] != NULL) && (s->stateBlock[i]->context.moduleID == msg->sid)
	&& (s->stateBlock[i]->context.type == msg->type)) {
      if (s->stateBlock[i]->context.state == DVM_STATE_HALT) {
	DvmStackVariable *stackArg;
	initializeContext(dvm_st, &s->stateBlock[i]->context);
	stackArg = (DvmStackVariable *)sys_msg_take_data(msg);
	if (stackArg != NULL)
	      pushOperand(s->stateBlock[i], stackArg);
	resumeContext(dvm_st, &s->stateBlock[i]->context, &s->stateBlock[i]->context);
      }
    }
  }
  return SOS_OK;
}
//--------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//--------------------------------------------------------------------
int8_t initEventHandler(dvm_state_t* dvm_st, DvmState *eventState, uint8_t capsuleID) 
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);

  if (capsuleID >= DVM_CAPSULE_NUM) {return -EINVAL;}
  s->stateBlock[capsuleID] = eventState;

  analyzeVars(dvm_st, capsuleID);
  initializeContext(dvm_st, &s->stateBlock[capsuleID]->context);
  {
#ifdef PC_PLATFORM
    int i;
    DEBUG("EV HDLR: Installing capsule %d:\n\t", (int)capsuleID);
    for (i = 0; i < s->stateBlock[capsuleID]->context.dataSize; i++) {
      DEBUG_SHORT("[%hhx]", getOpcode(dvm_st, capsuleID, i));
    }
    DEBUG_SHORT("\n");
#endif
  }
  engineReboot(dvm_st);
  rebootContexts(dvm_st);
  if (s->stateBlock[capsuleID]->context.moduleID == TIMER_PID) {
    // Ram - There is no init in the sys_ API. What should be done here ?
    //sys_timer_init(s->pid, capsuleID, TIMER_REPEAT);
    DEBUG("[EV HDLR] initEventHandler: Capsule Id = (%d): <<<< WARNING TIMER HAS NOT BEEN INITIALIZED >>>>> \n", capsuleID);
  }
  if (s->stateBlock[DVM_CAPSULE_REBOOT] != NULL) {
    initializeContext(dvm_st, &(s->stateBlock[DVM_CAPSULE_REBOOT]->context));
    resumeContext(dvm_st, &s->stateBlock[DVM_CAPSULE_REBOOT]->context, &s->stateBlock[DVM_CAPSULE_REBOOT]->context);
  }
  return SOS_OK;
}
//--------------------------------------------------------------------
DvmCapsuleLength getCodeLength(dvm_state_t* dvm_st, uint8_t id) 
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);

  if (s->stateBlock[id] != NULL)
    return s->stateBlock[id]->context.dataSize;
  else
    return 0;
}
//--------------------------------------------------------------------
uint8_t getLibraryMask(dvm_state_t* dvm_st, uint8_t id) 
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);
  if (s->stateBlock[id] != NULL)
    return s->stateBlock[id]->context.libraryMask;
  else
    return 0;
}
//--------------------------------------------------------------------	
DvmOpcode getOpcode(dvm_state_t* dvm_st, uint8_t id, uint16_t which) 
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);

  if (s->stateBlock[id] != NULL) {
    DvmState *ds = s->stateBlock[id];
    DvmOpcode op;
    // get the opcode at index "which" from codemem and return it.
    // handler for codemem is in s->stateBlock[id]->script
    if(sys_codemem_read(ds->cm, &(op), sizeof(op), offsetof(DvmScript, data) + which) == SOS_OK) {
      DEBUG("EV HDLR: getOpcode. correct place.\n");
      return op;
    }
    return OP_HALT;
  } else {
    return OP_HALT;
  }
}
//--------------------------------------------------------------------	
DvmState *getStateBlock (dvm_state_t* dvm_st, uint8_t id) 
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);
  if (s->stateBlock[id] != NULL) {
    return s->stateBlock[id];
  }
  return NULL;
}
//--------------------------------------------------------------------
// STATIC FUNCTIONS	
//--------------------------------------------------------------------
static inline void rebootContexts(dvm_state_t *dvm_st)
{
  DVMEventHandler_state_t *s = &(dvm_st->evhdlr_st);
  int i, j;
  for (i = 0; i < DVM_CAPSULE_NUM; i++) {
    if (s->stateBlock[i] != NULL) {
      for (j = 0; j < DVM_NUM_LOCAL_VARS; j++) {
	s->stateBlock[i]->vars[j].type = DVM_TYPE_INTEGER;
	s->stateBlock[i]->vars[j].value.var = 0;
      }
      if (s->stateBlock[i]->context.state != DVM_STATE_HALT)
	haltContext(dvm_st, &s->stateBlock[i]->context);
    }
  }
}
//--------------------------------------------------------------------	
