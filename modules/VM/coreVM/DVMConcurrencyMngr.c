/**
 * \file DVMConcurrencyMngr.c
 * \author Rahul Balani
 * \author Ilias Tsigkogiannis
 * \author Ram Kumar - Port to sos-2.x
 */

#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMqueue.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMBasiclib.h>
#ifdef PC_PLATFORM
#include <sos_sched.h>
#endif

//-----------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------------------
static inline uint8_t isRunnable(DvmContext* context, DVMConcurrencyMngr_state_t *s) ;
static inline int8_t obtainLocks(DvmContext* caller, DvmContext* obtainer, DVMConcurrencyMngr_state_t *s) ;
static inline int8_t releaseLocks(DvmContext* caller, DvmContext* releaser, DVMConcurrencyMngr_state_t *s) ;
static inline int8_t releaseAllLocks(DvmContext* caller, DvmContext* releaser, DVMConcurrencyMngr_state_t *s) ;
static inline void locks_reset(DVMConcurrencyMngr_state_t *s) ;
static inline int8_t lock(DvmContext *, uint8_t, DVMConcurrencyMngr_state_t *) ;
static inline int8_t unlock(DvmContext *, uint8_t, DVMConcurrencyMngr_state_t *) ;
static inline uint8_t isLocked(uint8_t, DVMConcurrencyMngr_state_t *) ;

//-----------------------------------------------------------------
// MESSAGE HANDLERS
//-----------------------------------------------------------------
//    case MSG_INIT:
int8_t concurrency_init(dvm_state_t* dvm_st, Message *msg)
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  s->libraries = 0;
  queue_init(&s->readyQueue);
  DEBUG("CONTEXT SYNCH: Initialized\n");
  return SOS_OK;
}
//-----------------------------------------------------------------
//    case MSG_FINAL:
int8_t concurrency_final(dvm_state_t* dvm_st, Message *msg)
{
  return SOS_OK;	
}
//-----------------------------------------------------------------
//    case MSG_ADD_LIBRARY:
int8_t concurrency_add_lib(dvm_state_t* dvm_st, Message *msg)
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  MsgParam *param = (MsgParam *)msg->data;
  uint8_t lib_id = param->byte;
  uint8_t lib_bit = 0x1 << lib_id;
  s->libraries |= lib_bit;
  s->extlib_module[lib_id] = msg->sid;
  return SOS_OK;
}
//-----------------------------------------------------------------
//    case MSG_REMOVE_LIBRARY:
int8_t concurrency_rem_lib(dvm_state_t* dvm_st, Message *msg)
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  MsgParam *param = (MsgParam *)msg->data;
  uint8_t lib_id = param->byte;
  uint8_t lib_bit = 0x1 << lib_id;
  s->libraries ^= lib_bit;	//XOR
  s->extlib_module[lib_id] = 0;
  return SOS_OK;
}
//-----------------------------------------------------------------
// EXTERNAL FUNCTIONS
//-----------------------------------------------------------------
void synch_reset(dvm_state_t* dvm_st) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  queue_init(&s->readyQueue);
  locks_reset(s);
}
//-----------------------------------------------------------------	
void analyzeVars(dvm_state_t* dvm_st, DvmCapsuleID id) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  uint16_t i;
  uint16_t handlerLen;
  DvmOpcode instr = 0;
  uint8_t opcode_mod = 0, libMask = 0;	
  DEBUG("VM: Analyzing capsule vars for handler %d: \n", (int)(id));
  for (i = 0; i < ((DVM_LOCK_COUNT + 7) / 8); i++) {
    s->usedVars[id][i] = 0;
  }
  handlerLen = getCodeLength(dvm_st, id);	
  DEBUG("CONTEXTSYNCH: Handler length for handler %d is %d.\n",id,handlerLen);
	
  libMask = getLibraryMask(dvm_st, id);
  if ((s->libraries & libMask) != libMask) {			//library missing
    DEBUG("CONTEXT_SYNCH: Library required 0x%x. Missing for handler %d.\n",libMask,id);
    return;
  }	
  i = 0;
  while (i < handlerLen) {
    int16_t locknum;
    instr = getOpcode(dvm_st, id, i);
    if (!(instr & LIB_ID_BIT)) {
      opcode_mod = DVM_MODULE_PID;
      locknum = lockNum(instr);
      i += bytelength(instr);
    } else {
      opcode_mod = (instr & BASIC_LIB_OP_MASK) >> EXT_LIB_OP_SHIFT;
      opcode_mod = s->extlib_module[opcode_mod];
      // Ram - XXX - Need to modify this for loadable extensions
      //      sys_fntable_subscribe(opcode_mod, LOCKNUM, 1);
      locknum = lockNum(instr);
      //      sys_fntable_subscribe(opcode_mod, BYTELENGTH, 0);
      i += bytelength(instr);
    }
    if (locknum >= 0) 
      s->usedVars[id][locknum / 8] |= (1 << (locknum % 8)); 
  } 
}
//-----------------------------------------------------------------	
void clearAnalysis(dvm_state_t* dvm_st, DvmCapsuleID id) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  memset(s->usedVars, 0, DVM_CAPSULE_NUM * ((DVM_LOCK_COUNT + 7)/8));
}
//-----------------------------------------------------------------	
void initializeContext(dvm_state_t* dvm_st, DvmContext *context) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  int i;
  for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) {
    context->heldSet[i] = 0;
    context->releaseSet[i] = 0;
  }
  memcpy(context->acquireSet, s->usedVars[context->which], (DVM_LOCK_COUNT + 7) / 8);
  context->pc = 0;
  resetStacks((DvmState *)context);
  if (context->queue) {
    queue_remove(context, context->queue, context);
  }
  context->state = DVM_STATE_HALT;
  context->num_executed = 0;
}
//-----------------------------------------------------------------	
void yieldContext(dvm_state_t* dvm_st, DvmContext* context) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  DvmContext* start = NULL;
  DvmContext* current = NULL;
  
  DEBUG("VM (%i): Yielding.\n", (int)context->which);
  releaseLocks(context, context, s);
  if (!queue_empty(&s->readyQueue)){
    do {
      current = queue_dequeue(context, &s->readyQueue);
      if (!resumeContext(dvm_st, context, current)) {
	DEBUG("VM (%i): Context %i not runnable.\n", (int)context->which, (int)current->which);
	if (start == NULL) {
	  start = current;
	}
	else if (start == current) {
	  DEBUG("VM (%i): Looped on ready queue. End checks.\n", (int)context->which);
	  break;
	}
      }
    }
    while (!queue_empty(&s->readyQueue));
  }
  else {
    DEBUG("VM (%i): Ready queue empty.\n", (int)context->which);
  }
}
//-----------------------------------------------------------------	
uint8_t resumeContext(dvm_state_t* dvm_st, DvmContext* caller, DvmContext* context) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  context->state = DVM_STATE_WAITING;
  if (isRunnable(context, s)) {
    obtainLocks(caller, context, s);
    if(scheduler_submit(dvm_st, context) == SOS_OK){
      DEBUG("VM (%i): Resumption of %i successful.\n", (int)caller->which, (int)context->which);
      context->num_executed = 0;
      return TRUE;
    }
    else {
      DEBUG("VM (%i): Resumption of %i FAILED.\n", (int)caller->which, (int)context->which);
      return -EINVAL;
    }
  }
  else {
    DEBUG("VM (%i): Resumption of %i unsuccessful, putting on the queue.\n", (int)caller->which, (int)context->which);
    queue_enqueue(caller, &s->readyQueue, context);
    return FALSE;
  }	
}
//-----------------------------------------------------------------	
void haltContext(dvm_state_t* dvm_st, DvmContext* context) 
{
  DVMConcurrencyMngr_state_t *s = &(dvm_st->conmgr_st);
  releaseAllLocks(context, context, s);
  yieldContext(dvm_st, context);
  if (context->queue && context->state != DVM_STATE_HALT) {
    queue_remove(context, context->queue, context);
  }
  if (context->state != DVM_STATE_HALT) {
    context->state = DVM_STATE_HALT;
  }
}
//-----------------------------------------------------------------	
// STATIC FUNCTIONS
//-----------------------------------------------------------------	
static inline uint8_t isRunnable(DvmContext* context, DVMConcurrencyMngr_state_t *s) 
{ 
  int8_t i;
  uint8_t* neededLocks = (context->acquireSet);  
  DEBUG("VM: Checking whether context %i runnable: ", (int)context->which);
  for (i = 0; i < DVM_LOCK_COUNT; i++){
    DEBUG_SHORT("%i,", (int)i); 
    if ((neededLocks[i / 8]) & (1 << (i % 8))) {
      if (isLocked(i, s)){
	DEBUG_SHORT(" - no\n");
	return FALSE;
      }
    }
  }
  DEBUG_SHORT(" - yes\n");
  return TRUE;
}
//-----------------------------------------------------------------	
static int8_t obtainLocks(DvmContext* caller, DvmContext* obtainer, DVMConcurrencyMngr_state_t *s) 
{ 
  int i;
  uint8_t* neededLocks = (obtainer->acquireSet);
  DEBUG("VM: Attempting to obtain necessary locks for context %i: ", obtainer->which);
  for (i = 0; i < DVM_LOCK_COUNT; i++) {
    DEBUG_SHORT("%i", (int)i);
    if ((neededLocks[i / 8]) & (1 << (i % 8))) {
      DEBUG_SHORT("+"); 
      lock(obtainer, i, s);
    }
    DEBUG_SHORT(","); 
  }
  for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) {
    obtainer->acquireSet[i] = 0;
  }
  DEBUG_SHORT("\n");
  return SOS_OK;		
}
//-----------------------------------------------------------------	
static inline int8_t releaseLocks(DvmContext* caller, DvmContext* releaser, DVMConcurrencyMngr_state_t *s) 
{
  int i;
  uint8_t* lockSet = (releaser->releaseSet);
	
  DEBUG("VM: Attempting to release specified locks for context %i.\n", releaser->which);
  for (i = 0; i < DVM_LOCK_COUNT; i++) {
    if ((lockSet[i / 8]) & (1 << (i % 8))) {
      unlock(releaser, i, s);
    }
  }
  for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) {
    releaser->releaseSet[i] = 0;
  }
  return SOS_OK;		
}
//-----------------------------------------------------------------	
static int8_t releaseAllLocks(DvmContext* caller, DvmContext* releaser, DVMConcurrencyMngr_state_t *s) 
{
  int i;
  uint8_t* lockSet = (releaser->heldSet);	
  DEBUG("VM: Attempting to release all locks for context %i.\n", releaser->which);
  for (i = 0; i < DVM_LOCK_COUNT; i++) {
    if ((lockSet[i / 8]) & (1 << (i % 8))) {
      unlock(releaser, i, s);
    }
  }
  for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) {
    releaser->releaseSet[i] = 0;
  }
  return SOS_OK;
}
//-----------------------------------------------------------------	
static inline void locks_reset(DVMConcurrencyMngr_state_t *s) 
{
  uint16_t i;
  for( i = 0; i < DVM_LOCK_COUNT; i++) 
    {
      s->locks[i].holder = NULL;
    }
}
//-----------------------------------------------------------------	
static inline int8_t lock(DvmContext* context, uint8_t lockNum, DVMConcurrencyMngr_state_t *s) 
{
  s->locks[lockNum].holder = context;
  context->heldSet[lockNum / 8] |= (1 << (lockNum % 8));
  DEBUG("VM: Context %i locking lock %i\n", (int)context->which, (int)lockNum);
  return SOS_OK;
}
//-----------------------------------------------------------------	
static inline int8_t unlock(DvmContext* context, uint8_t lockNum, DVMConcurrencyMngr_state_t *s) 
{
  context->heldSet[lockNum / 8] &= ~(1 << (lockNum % 8));
  s->locks[lockNum].holder = 0;
  DEBUG("VM: Context %i unlocking lock %i\n", (int)context->which, (int)lockNum);
  return SOS_OK;
}
//-----------------------------------------------------------------	
static inline uint8_t isLocked(uint8_t lockNum, DVMConcurrencyMngr_state_t *s) 
{
  return (s->locks[lockNum].holder != 0);	
}
//-----------------------------------------------------------------	
