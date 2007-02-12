/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <module.h>
 */
#include <module.h>
#include <sys_module.h>
#include <codemem.h>
#include <loader/loader.h>


#define THIS_MODULE_ID  DFLT_APP_ID3


static int8_t module(void *state, Message *e);


static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = THIS_MODULE_ID,
  .state_size     = 0,
  .num_timers     = 0,
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
  .processor_type = MCU_TYPE,
  .code_id        = ehtons(THIS_MODULE_ID),
  .module_handler = module,
};


static void load_script( uint8_t *script, uint8_t size );
static int8_t module(void *state, Message *msg)
{
  switch (msg->type){

  case MSG_INIT:
	{
	  // reboot context 
	  uint8_t reboot_script_buf[11] = { 0xa5,0x0,0x0,0x0,0x4,0x0,0x0,0x1,0xe8,0x16,0x0, };
	  load_script(reboot_script_buf, 11);
	  // timer0 context 
	  uint8_t timer0_script_buf[11] = { 0xa5,0x1,0x4,0x2,0x4,0x0,0x0,0x1,0x1c,0x3,0x0, };
	  load_script(timer0_script_buf, 11);
	  break;
	}
  case MSG_TIMER_TIMEOUT:
	{
	  break;
	}
  case MSG_FINAL:
	{
	  break;
	}
  default:
	return -EINVAL;
  }

  return SOS_OK;
}

static void load_script( uint8_t *script, uint8_t size )
{
  codemem_t cm = ker_codemem_alloc(size, CODEMEM_TYPE_EXECUTABLE);
  ker_codemem_write( cm, THIS_MODULE_ID, script, size, 0);
  ker_codemem_flush( cm, THIS_MODULE_ID);
  post_short(script[0], THIS_MODULE_ID, MSG_LOADER_DATA_AVAILABLE, script[1], cm, 0);
  return;
}

#ifndef _MODULE_
mod_header_ptr script_loader_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

