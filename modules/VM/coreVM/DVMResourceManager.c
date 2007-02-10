/**
 * \file DVMResourceManager.c
 * \author Rahul Balani
 * \author Ram Kumar - Port to sos-2.x
 */
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include <VM/DVMResourceManager.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMBasiclib.h>
#include <sys_module.h>
#ifdef PC_PLATFORM
#include <sos_sched.h>
#endif

//----------------------------------------------------------------------------
// HANDLER FUNCTIONS
//----------------------------------------------------------------------------
//  case MSG_INIT: 
int8_t resmanager_init(dvm_state_t* dvm_st, Message *msg)
{
  DEBUG("RES MNGR: Start Init\n");
  uint8_t i;
  DVMResourceManager_state_t *s = &(dvm_st->resmgr_st);
  s->script_block_ptr = (DvmState *)sys_malloc(DVM_DEFAULT_MEM_ALLOC_SIZE);
  DEBUG("script block @ addr %08x \n", (uint32_t)s->script_block_ptr);
  if (s->script_block_ptr == NULL)
    DEBUG("No space for script blocks\n");
  for (i = 0; i < DVM_CAPSULE_NUM; i++) {
    s->scripts[i] = NULL;
  }
  for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++)
    s->script_block_owners[i] = NULL_CAPSULE;
  
  DEBUG("RES MNGR: Initialized\n");
  return SOS_OK;
}
//----------------------------------------------------------------------------
//  case MSG_FINAL:
int8_t resmanager_final(dvm_state_t* dvm_st, Message *msg)
{
  uint8_t i;
  DVMResourceManager_state_t *s = &(dvm_st->resmgr_st);
  for (i = 0; i < DVM_CAPSULE_NUM; i++) {
    mem_free(dvm_st, i);
  }
  sys_free(s->script_block_ptr);
  return SOS_OK;
}
//----------------------------------------------------------------------------
//  case MSG_LOADER_DATA_AVAILABLE:
int8_t resmanager_loader_data_handler(dvm_state_t* dvm_st, Message *msg)
{
  MsgParam* params = ( MsgParam * )( msg->data );
  uint8_t id = params->byte;
  codemem_t cm = params->word;
  DvmState *ds;
  
  DEBUG("RESOURCE_MANAGER: Data received. ID - %d.\n", id);
  ds = mem_allocate(dvm_st, id);
  if( ds == NULL ) { 
    // XXX may want to set a timer for retry
    return -ENOMEM;
  }
  ds->context.which = id;
  sys_codemem_read(cm, &(ds->context.moduleID), sizeof(ds->context.moduleID), offsetof(DvmScript, moduleID));
  DEBUG("RESOURCE_MANAGER: Event - module = %d, offset = %ld \n", ds->context.moduleID, offsetof(DvmScript, moduleID));
  sys_codemem_read(cm, &(ds->context.type), sizeof(ds->context.type), offsetof(DvmScript, eventType));
  DEBUG("RESOURCE_MANAGER: Event - event type = %d, offset = %ld \n", ds->context.type, offsetof(DvmScript, eventType));
  sys_codemem_read(cm, &(ds->context.dataSize), sizeof(ds->context.dataSize), offsetof(DvmScript, length));
  ds->context.dataSize = ehtons(ds->context.dataSize);
  DEBUG("RESOURCE_MANAGER: Length of script = %d, offset = %ld \n", ds->context.dataSize, offsetof(DvmScript, length));
  sys_codemem_read(cm, &(ds->context.libraryMask), sizeof(ds->context.libraryMask), offsetof(DvmScript, libraryMask));
  DEBUG("RESOURCE_MANAGER: Library Mask = 0x%x, offset = %ld \n", ds->context.libraryMask, offsetof(DvmScript, libraryMask));
  ds->cm = cm;
  initEventHandler(dvm_st, ds, id);
  return SOS_OK;
}
//----------------------------------------------------------------------------
// EXTERNAL FUNCTIONS
//----------------------------------------------------------------------------
// Allocate continuous space to context, stack and local variables.
DvmState *mem_allocate(dvm_state_t* dvm_st, uint8_t capsuleNum)
{
  DVMResourceManager_state_t *s = &(dvm_st->resmgr_st);
  uint8_t i;
  int8_t script_index = -1;
  if (capsuleNum >= DVM_CAPSULE_NUM) return NULL;
  if (s->scripts[capsuleNum] == NULL) {
    //Try to allocate space
    for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++) {
      if (s->script_block_owners[i] == NULL_CAPSULE) {
	script_index = i;
	s->script_block_owners[script_index] = capsuleNum;
	s->scripts[capsuleNum] = s->script_block_ptr + script_index;
	break;
      }
    }
    if (script_index < 0) {
      s->scripts[capsuleNum] = (DvmState *)sys_malloc(DVM_STATE_SIZE);
      if (s->scripts[capsuleNum] == NULL) return NULL;
    }
    // If control reaches here, this means space has been successfully
    // allocated for context etc.
  }
  memset(s->scripts[capsuleNum], 0, DVM_STATE_SIZE);
  return s->scripts[capsuleNum];
}
//----------------------------------------------------------------------------
int8_t mem_free(dvm_state_t* dvm_st, uint8_t capsuleNum)
{
  DVMResourceManager_state_t *s = &(dvm_st->resmgr_st);
  uint8_t i;
  //Free script space
  for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++) {
    if (s->script_block_owners[i] == capsuleNum) {
      s->script_block_owners[i] = NULL_CAPSULE;
      break;
    }
  }
  if (i >= DVM_NUM_SCRIPT_BLOCKS) 
    sys_free(s->scripts[capsuleNum]); 
  s->scripts[capsuleNum] = NULL;
  return SOS_OK;
}
//----------------------------------------------------------------------------
