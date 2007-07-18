/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
#include <sys_module.h>
#include <led_dbg.h>
#include "pre_blink.h"

#define BLINK_TIMER_INTERVAL	1024L
#define BLINK_TID               0
#ifdef SOS_USE_PREEMPTION
#define BLINK_PRIORITY          3
#endif

typedef struct {
  uint8_t pid;
} app_state_t;

static int8_t pre_blink_msg_handler(void *start, Message *e);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = DFLT_APP_ID0,
	.state_size     = sizeof(app_state_t),
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(DFLT_APP_ID0),
	.module_handler = pre_blink_msg_handler,
#ifdef SOS_USE_PREEMPTION
    .init_priority  = BLINK_PRIORITY,
#endif
};


static int8_t pre_blink_msg_handler(void *state, Message *msg)
{
  switch (msg->type){
  case MSG_INIT:
	{
	  sys_timer_start(BLINK_TID, BLINK_TIMER_INTERVAL, TIMER_REPEAT);
	  break;
	}

  case MSG_FINAL:
	{
	  sys_timer_stop(BLINK_TID);
	  break;
	}

  case MSG_TIMER_TIMEOUT:
	{
	  sys_led(LED_GREEN_TOGGLE);
	  break;
	}

	default:
	return -EINVAL;
  }

  return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr pre_blink_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif


