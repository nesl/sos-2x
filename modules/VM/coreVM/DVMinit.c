#include <VM/dvm_init.h>
#include <VM/DVMBasiclib.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMStacks.h>
#include <VM/DVMResourceManager.h>
#include <VM/DVMEventHandler.h>
//#include <VM/MVirus.h>
int8_t dvm_mathlib_init();
int8_t dvm_buffer_init();
int8_t resmanager_init();
int8_t stacks_init();
int8_t eventhandler_init();
int8_t concurrency_init();
int8_t basiclib_init();
int8_t VMscheduler_init();
int8_t dvm_queue_init();

int8_t vm_init() {
  DEBUG("Starting VM init\n");
  resmanager_init();
  DEBUG("Resource manager initialized\n");
  stacks_init();
  DEBUG("Stacks initialized\n");
  eventhandler_init();
  DEBUG("Event manager initialized\n");
  concurrency_init();
  DEBUG("Concurrency manager initialized\n");
  basiclib_init();
  DEBUG("Basiclib manager initialized\n");
  //mvirus_init();
  //DEBUG("Trickle control initialized\n");
  VMscheduler_init();
  DEBUG("Scheduler manager initialized\n");
  return SOS_OK;
}

