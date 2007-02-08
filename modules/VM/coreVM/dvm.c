/**
 * \file dvm.c
 * \brief Top level virtual machine handler
 * \author Ram Kumar - Port to sos-2.x
 */


//------------------------------------------------------------------------
// INCLUDES
//------------------------------------------------------------------------
#include <VM/dvm_types.h>
#include <VM/DVMScheduler.h>
#include <VM/DVMqueue.h>
#include <VM/DVMConcurrencyMngr.h>
#include <VM/DVMEventHandler.h>
#include <VM/DVMBasiclib.h>
#include <loader/loader.h> // For loader message type


//------------------------------------------------------------------------
// TYPEDEFS
//------------------------------------------------------------------------
typedef int8_t (*execute_lib_func_t)(func_cb_ptr cb, DvmContext* context, DvmOpcode instr);

//------------------------------------------------------------------------
// STATIC FUNCTIONS
//------------------------------------------------------------------------
static int8_t dvm_handler(void* state, Message *msg);

//------------------------------------------------------------------------
// MODULE HEADER
//------------------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         =   DVM_MODULE,
  .code_id        =   ehtons(DVM_MODULE),
  .platform_type  =   HW_TYPE,
  .processor_type =   MCU_TYPE,
  .state_size     =   sizeof(dvm_state_t),
  .num_sub_func   =    4,
  .num_prov_func  =    0,
  .module_handler =    dvm_handler,
  .funct          = {
    {error_8, "czy2", M_EXT_LIB, EXECUTE},
    {error_8, "czy2", M_EXT_LIB, EXECUTE},
    {error_8, "czy2", M_EXT_LIB, EXECUTE},
    {error_8, "czy2", M_EXT_LIB, EXECUTE},
  },
};


//------------------------------------------------------------------------
// Ram - Do not change the order in which the functions are called in this handler !!
//       i.e. Do not change it unless you are absolutey sure what you are doing
static int8_t dvm_handler(void* state, Message *msg)
{
  dvm_state_t* dvm_st = (dvm_state_t*)state;
  switch (msg->type){
  case MSG_INIT:
    {
      DEBUG("Starting VM Initialization\n");
      resmanager_init(dvm_st, msg);
      event_handler_init(dvm_st, msg);
      concurrency_init(dvm_st, msg);
      dvmsched_init(dvm_st, msg);
      DEBUG("DVM ENGINE: Dvm initializing DONE.\n");
      break;
    }
    
  case MSG_FINAL:
    {
      concurrency_final(dvm_st, msg);      
      event_handler_final(dvm_st, msg);
      resmanager_final(dvm_st, msg);
      dvmsched_final(dvm_st, msg);
      DEBUG("DVM ENGINE: VM: Stopping.\n");
      break;
    }

  case MSG_RUN_TASK:
    {
      dvmsched_run_task(dvm_st, msg);
      break;
    }
    
  case MSG_HALT:
    {
      dvmsched_halt(dvm_st, msg);
      break;
    }

  case MSG_RESUME:
    {
      dvmsched_resume(dvm_st, msg);
      break;
    }

  case MSG_TIMER_TIMEOUT:
    {
      dvmsched_timeout(dvm_st, msg);
      event_handler_timeout(dvm_st, msg);
      break;
    }

  case MSG_ADD_LIBRARY:
    {
      concurrency_add_lib(dvm_st, msg);
      dvmsched_add_lib(dvm_st, msg);
      break;
    }

  case MSG_REMOVE_LIBRARY:
    {
      concurrency_rem_lib(dvm_st, msg);
      dvmsched_rem_lib(dvm_st, msg);
      break;
    }

  case MSG_LOADER_DATA_AVAILABLE:
    {
      resmanager_loader_data_handler(dvm_st, msg);
      break;
    }

  default:
    {
      event_handler_default(dvm_st, msg);
      break;
    }
  }
  return SOS_OK;
}
