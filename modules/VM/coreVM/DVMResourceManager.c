/**
 * \file DVMResourceManager.c
 * \author Rahul Balani
 * \author Ram Kumar - Port to sos-2.x
 */
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include <loader/loader.h>
#include <VM/DVMResourceManager.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMBasiclib.h>
//----------------------------------------------------------------------------
int8_t resmanager_handler(void *state, Message *msg){
  DVMResourceManager_state_t *s = (DVMResourceManager_state_t *) state;
  switch (msg->type) {
  case MSG_INIT: 
    {
      uint8_t i;
      s->script_block_ptr = (DvmState *)sys_malloc(DVM_DEFAULT_MEM_ALLOC_SIZE);
      DEBUG("script block got this addr %08x \n", (uint32_t)s->script_block_ptr);
      if (s->script_block_ptr == NULL)
	DEBUG("No space for script blocks\n");
      for (i = 0; i < DVM_CAPSULE_NUM; i++) {
	s->scripts[i] = NULL;
      }
      for (i = 0; i < DVM_NUM_SCRIPT_BLOCKS; i++)
	s->script_block_owners[i] = NULL_CAPSULE;

      DEBUG("Resource Manager: Initialized\n");
      break;
    }
  case MSG_LOADER_DATA_AVAILABLE:
    {
      MsgParam* params = ( MsgParam * )( msg->data );
      uint8_t id = params->byte;
      codemem_t cm = params->word;
      DvmState *ds;

      DEBUG("RESOURCE_MANAGER: Data received. ID - %d.\n", id);
      ds = mem_allocate(id);
      if( ds == NULL ) { 
	// XXX may want to set a timer for retry
	return -ENOMEM;
      }
      ds->context.which = id;
      sys_codemem_read(cm, DVM_MODULE,
		       &(ds->context.moduleID), 
		       sizeof(ds->context.moduleID), offsetof(DvmScript, moduleID));
      DEBUG("RESOURCE_MANAGER: Event - module = %d, offset = %ld \n", ds->context.moduleID, offsetof(DvmScript, moduleID));
      sys_codemem_read(cm, DVM_MODULE,
		       &(ds->context.type), sizeof(ds->context.type),
		       offsetof(DvmScript, eventType));
      DEBUG("RESOURCE_MANAGER: Event - event type = %d, offset = %ld \n", ds->context.type, offsetof(DvmScript, eventType));
      sys_codemem_read(cm, DVM_MODULE,
		       &(ds->context.dataSize), sizeof(ds->context.dataSize),
		       offsetof(DvmScript, length));
      ds->context.dataSize = ehtons(ds->context.dataSize);
      DEBUG("RESOURCE_MANAGER: Length of script = %d, offset = %ld \n", ds->context.dataSize, offsetof(DvmScript, length));
      sys_codemem_read(cm, DVM_MODULE,
		       &(ds->context.libraryMask), sizeof(ds->context.libraryMask),
		       offsetof(DvmScript, libraryMask));
      DEBUG("RESOURCE_MANAGER: Library Mask = 0x%x, offset = %ld \n", ds->context.libraryMask, offsetof(DvmScript, libraryMask));
      ds->cm = cm;
      initEventHandler(ds, id);
      break;
    }
  case MSG_FINAL:
    {
      uint8_t i;
      for (i = 0; i < DVM_CAPSULE_NUM; i++) {
	mem_free(i);
      }
      sys_free(s->script_block_ptr);
      break;
    }
  default:
    break;
  }
  return SOS_OK;
}
//----------------------------------------------------------------------------
// Allocate continuous space to context, stack and local variables.
DvmState *mem_allocate(uint8_t capsuleNum)
{
  DVMResourceManager_state_t *s = (DVMResourceManager_state_t*)sys_get_module_state();
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
int8_t mem_free(uint8_t capsuleNum)
{
  DVMResourceManager_state_t *s = (DVMResourceManager_state_t*)sys_get_module_state();
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
