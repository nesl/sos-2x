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

//-----------------------------------------------------------------
// TYPEDEFS
//-----------------------------------------------------------------
typedef struct 
{
  uint8_t pid;
  uint8_t usedVars[DVM_CAPSULE_NUM][(DVM_LOCK_COUNT + 7) / 8];
  DvmQueue readyQueue;
  uint8_t libraries;
  uint8_t extlib_module[4];
  DvmLock locks[DVM_LOCK_COUNT];
} app_state;  
//-----------------------------------------------------------------
// STATIC FUNCTION PROTOTYPES
//-----------------------------------------------------------------
static inline uint8_t isRunnable(DvmContext* context, app_state *s) ;
static inline int8_t obtainLocks(DvmContext* caller, DvmContext* obtainer, app_state *s) ;
static inline int8_t releaseLocks(DvmContext* caller, DvmContext* releaser, app_state *s) ;
static inline int8_t releaseAllLocks(DvmContext* caller, DvmContext* releaser, app_state *s) ;
static inline void locks_reset(app_state *s) ;
static inline int8_t lock(DvmContext *, uint8_t, app_state *) ;
static inline int8_t unlock(DvmContext *, uint8_t, app_state *) ;
static inline uint8_t isLocked(uint8_t, app_state *) ;
static int8_t concurrency_handler(void *state, Message *msg) ;
//-----------------------------------------------------------------
// EXTERNAL FUNCTION PROTOTYPES
//-----------------------------------------------------------------
static void synch_reset(func_cb_ptr p);
static void analyzeVars(func_cb_ptr p, DvmCapsuleID id); 
static void clearAnalysis(func_cb_ptr p, DvmCapsuleID id);
static void initializeContext(func_cb_ptr p, DvmContext *context);
static void yieldContext(func_cb_ptr p, DvmContext* context);
static uint8_t resumeContext(func_cb_ptr p, DvmContext* caller, DvmContext* context); 
static void haltContext(func_cb_ptr p, DvmContext* context); 
static uint8_t isHeldBy(func_cb_ptr p, uint8_t lockNum, DvmContext* context); 
//-----------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = 
  {
    .mod_id           =  M_CONTEXT_SYNCH,
    .state_size       =  sizeof(app_state),
    .code_id          = ehtons(M_CONTEXT_SYNCH),
    .platform_type    = HW_TYPE,
    .processor_type   = MCU_TYPE,
    .num_sub_func     =  12,
    .num_prov_func    =  8,
    .module_handler   =  concurrency_handler,
    .funct = {},
  };
//-----------------------------------------------------------------
static int8_t concurrency_handler(void *state, Message *msg)
{    
  app_state *s = (app_state *) state;
  switch (msg->type)
    {
    case MSG_INIT:
      {
			
	s->pid = msg->did;		    
	s->libraries = 0;
	queue_initDL(s->q_init, &s->readyQueue );
			
	DEBUG("CONTEXT SYNCH: Initialized\n");
	return SOS_OK;
      }
    case MSG_ADD_LIBRARY:
      {
	MsgParam *param = (MsgParam *)msg->data;
	uint8_t lib_id = param->byte;
	uint8_t lib_bit = 0x1 << lib_id;
	s->libraries |= lib_bit;
	s->extlib_module[lib_id] = msg->sid;
	return SOS_OK;
      }
    case MSG_REMOVE_LIBRARY:
      {
	MsgParam *param = (MsgParam *)msg->data;
	uint8_t lib_id = param->byte;
	uint8_t lib_bit = 0x1 << lib_id;
	s->libraries ^= lib_bit;	//XOR
	s->extlib_module[lib_id] = 0;
	return SOS_OK;
      }
    case MSG_FINAL:
      {
	return SOS_OK;	
      }
    default:
      return SOS_OK;
    }
  return SOS_OK;
}
//-----------------------------------------------------------------
void synch_reset() 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
  queue_initDL(s->q_init, &s->readyQueue);
  locks_reset(s);
}
//-----------------------------------------------------------------	
void analyzeVars(DvmCapsuleID id) 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
  uint16_t i;
  uint16_t handlerLen;
  DvmOpcode instr = 0;
  uint8_t opcode_mod = 0, libMask = 0;	
  DEBUG("VM: Analyzing capsule vars for handler %d: \n", (int)(id));
  for (i = 0; i < ((DVM_LOCK_COUNT + 7) / 8); i++) {
    s->usedVars[id][i] = 0;
  }
  handlerLen = getCodeLengthDL(s->getCodeLength, id);	
  DEBUG("CONTEXTSYNCH: Handler length for handler %d is %d.\n",id,handlerLen);
	
  libMask = getLibraryMaskDL(s->get_libmask, id);
  if ((s->libraries & libMask) != libMask) {			//library missing
    DEBUG("CONTEXT_SYNCH: Library required 0x%x. Missing for handler %d.\n",libMask,id);
    return;
  }	
  i = 0;
  while (i < handlerLen) {
    int16_t locknum;
    instr = getOpcodeDL(s->get_opcode, id, i);
    if (!(instr & LIB_ID_BIT)) {
      opcode_mod = M_BASIC_LIB;
      locknum = lockNumDL(s->lockNum, instr);
      i += bytelengthDL(s->bytelength, instr);
    } else {
      opcode_mod = (instr & BASIC_LIB_OP_MASK) >> EXT_LIB_OP_SHIFT;
      opcode_mod = s->extlib_module[opcode_mod];
      ker_fntable_subscribe(s->pid, opcode_mod, LOCKNUM, 1);
      locknum = SOS_CALL(s->lockNum, func_i16u8_t, instr);
      ker_fntable_subscribe(s->pid, opcode_mod, BYTELENGTH, 0);
      i += SOS_CALL(s->bytelength, func_u8u8_t, instr);
    }
    if (locknum >= 0) 
      s->usedVars[id][locknum / 8] |= (1 << (locknum % 8)); 
  } 
}
//-----------------------------------------------------------------	
void clearAnalysis(DvmCapsuleID id) 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
  memset(s->usedVars, 0, DVM_CAPSULE_NUM * ((DVM_LOCK_COUNT + 7)/8));
}
//-----------------------------------------------------------------	
void initializeContext(DvmContext *context) 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
  int i;
  for (i = 0; i < (DVM_LOCK_COUNT + 7) / 8; i++) {
    context->heldSet[i] = 0;
    context->releaseSet[i] = 0;
  }
  memcpy(context->acquireSet, s->usedVars[context->which], (DVM_LOCK_COUNT + 7) / 8);
  context->pc = 0;
  resetStacksDL(s->reset_stack, (DvmState *)context);
  if (context->queue) {
    queue_removeDL(s->q_remove, context, context->queue, context);
  }
  context->state = DVM_STATE_HALT;
  context->num_executed = 0;
}
//-----------------------------------------------------------------	
void yieldContext(DvmContext* context) 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
  DvmContext* start = NULL;
  DvmContext* current = NULL;
  
  DEBUG("VM (%i): Yielding.\n", (int)context->which);
  releaseLocks(context, context, s);
  if (!queue_emptyDL(s->q_empty, &s->readyQueue)){
    do {
      current = queue_dequeueDL(s->q_dequeue, context, &s->readyQueue);
      if (!resumeContext(context, current)) {
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
    while ( !queue_emptyDL(s->q_empty,  &s->readyQueue ) );
  }
  else {
    DEBUG("VM (%i): Ready queue empty.\n", (int)context->which);
  }
}
//-----------------------------------------------------------------	
uint8_t resumeContext(DvmContext* caller, DvmContext* context) 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);   
  context->state = DVM_STATE_WAITING;
  if (isRunnable(context, s)) {
    obtainLocks(caller, context, s);
    if(scheduler_submitDL(s->sched_submit, context) == SOS_OK){
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
    queue_enqueueDL(s->q_enqueue, caller, &s->readyQueue, context);
    return FALSE;
  }	
}
//-----------------------------------------------------------------	
void haltContext(DvmContext* context) 
{
  app_state *s = (app_state*)ker_get_module_state(M_CONTEXT_SYNCH);
  releaseAllLocks(context, context, s);
  yieldContext(context);
  if (context->queue && context->state != DVM_STATE_HALT) {
    queue_removeDL(s->q_remove, context, context->queue, context);
  }
  if (context->state != DVM_STATE_HALT) {
    context->state = DVM_STATE_HALT;
  }
}
//-----------------------------------------------------------------	
//MLocks functions
//-----------------------------------------------------------------	
static inline uint8_t isRunnable(DvmContext* context, app_state *s) 
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
static int8_t obtainLocks(DvmContext* caller, DvmContext* obtainer, app_state *s) 
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
static inline int8_t releaseLocks(DvmContext* caller, DvmContext* releaser, app_state *s) 
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
static int8_t releaseAllLocks(DvmContext* caller, DvmContext* releaser, app_state *s) 
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
static inline void locks_reset(app_state *s) 
{
  uint16_t i;
  for( i = 0; i < DVM_LOCK_COUNT; i++) 
    {
      s->locks[i].holder = NULL;
    }
}
//-----------------------------------------------------------------	
static inline int8_t lock(DvmContext* context, uint8_t lockNum, app_state *s) 
{
  s->locks[lockNum].holder = context;
  context->heldSet[lockNum / 8] |= (1 << (lockNum % 8));
  DEBUG("VM: Context %i locking lock %i\n", (int)context->which, (int)lockNum);
  return SOS_OK;
}
//-----------------------------------------------------------------	
static inline int8_t unlock(DvmContext* context, uint8_t lockNum, app_state *s) 
{
  context->heldSet[lockNum / 8] &= ~(1 << (lockNum % 8));
  s->locks[lockNum].holder = 0;
  DEBUG("VM: Context %i unlocking lock %i\n", (int)context->which, (int)lockNum);
  return SOS_OK;
}
//-----------------------------------------------------------------	
static inline uint8_t isLocked(uint8_t lockNum, app_state *s) 
{
  return (s->locks[lockNum].holder != 0);	
}
//-----------------------------------------------------------------	
