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
#include <VM/DVMResourceManager.h>
#include <VM/DVMBasiclib.h>
#include <loader/loader.h> // For loader message type
#include <sys_module.h>
#include <led.h>

//------------------------------------------------------------------------
// TYPEDEFS
//------------------------------------------------------------------------
typedef int8_t (*execute_lib_func_t)(func_cb_ptr cb, DvmContext* context, DvmOpcode instr);

//------------------------------------------------------------------------
// STATIC FUNCTIONS
//------------------------------------------------------------------------
static int8_t dvm_handler(void* state, Message *msg);
static int8_t error_dvm(func_cb_ptr p);

//------------------------------------------------------------------------
// MODULE HEADER
//------------------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         =   DVM_MODULE_PID,
  .code_id        =   ehtons(DVM_MODULE_PID),
  .platform_type  =   HW_TYPE,
  .processor_type =   MCU_TYPE,
  .state_size     =   sizeof(dvm_state_t),
  .num_sub_func   =    5,
  .num_prov_func  =    0,
  .module_handler =    dvm_handler,
  .funct          = {
    {error_dvm, "czy2", M_EXT_LIB, EXECUTE},
    {error_dvm, "czy2", M_EXT_LIB, EXECUTE},
    {error_dvm, "czy2", M_EXT_LIB, EXECUTE},
    {error_dvm, "czy2", M_EXT_LIB, EXECUTE},
    {error_dvm, "cCz4", RUNTIME_PID, EXECUTE_SYNCALL},
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
      DEBUG("DVM: Start of init routines ... \n");
      sys_led(LED_RED_TOGGLE);
      resmanager_init(dvm_st, msg);
      event_handler_init(dvm_st, msg);
      concurrency_init(dvm_st, msg);
      dvmsched_init(dvm_st, msg);
      basic_library_init(dvm_st, msg);
      DEBUG("DVM: Init done\n");
      break;
    }
    
  case MSG_FINAL:
    {
      concurrency_final(dvm_st, msg);      
      event_handler_final(dvm_st, msg);
      resmanager_final(dvm_st, msg);
      dvmsched_final(dvm_st, msg);
      basic_library_final(dvm_st, msg);
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

  case MSG_DATA_READY:
    {
      basic_library_data_ready(dvm_st, msg);
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
      sys_led(LED_GREEN_TOGGLE);
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
//------------------------------------------------------------------------
// ERROR HANDLER
//------------------------------------------------------------------------
static int8_t error_dvm(func_cb_ptr p)
{
  return -EINVAL;
}
//------------------------------------------------------------------------
// HEADER GRABBER
//------------------------------------------------------------------------
#ifndef _MODULE_
mod_header_ptr dvm_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif


