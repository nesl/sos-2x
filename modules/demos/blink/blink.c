/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief Example module for SOS
 * This module shows some of concepts in SOS
 */

/**
 * Module needs to include <module.h>
 */
//#undef _MODULE_

#include <sys_module.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include "blink.h"

#define BLINK_TIMER_INTERVAL	1024L
#define BLINK_TID               0
/**
 * Module can define its own state
 */
typedef struct {
  uint8_t pid;
  uint8_t state;
} app_state_t;

/**
 * Blink module
 *
 * @param msg Message being delivered to the module
 * @return int8_t SOS status message
 *
 * Modules implement a module function that acts as a message handler.  The
 * module function is typically implemented as a switch acting on the message
 * type.
 *
 * All modules should included a handler for MSG_INIT to initialize module
 * state, and a handler for MSG_FINAL to release module resources.
 */

static int8_t blink_msg_handler(void *start, Message *e);

/**
 * This is the only global variable one can have.
 */
static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = DFLT_APP_ID0,
	.state_size     = sizeof(app_state_t),
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(DFLT_APP_ID0),
	.module_handler = blink_msg_handler,
};


static int8_t blink_msg_handler(void *state, Message *msg)
{
  /**
   * The module is passed in a void* that contains its state.  For easy
   * reference it is handy to typecast this variable to be of the
   * applications state type.  Note that since we are running as a module,
   * this state is not accessible in the form of a global or static
   * variable.
   */
  app_state_t *s = (app_state_t*)state;

  /**
   * Switch to the correct message handler
   */
  switch (msg->type){
	/**
	 * MSG_INIT is used to initialize module state the first time the
	 * module is used.  Many modules set timers at this point, so that
	 * they will continue to receive periodic (or one shot) timer events.
	 */
  case MSG_INIT:
  {
	  s->pid = msg->did;
	  s->state = 0;
	  sys_led(LED_RED_TOGGLE);
	  /**
	   * The timer API takes the following parameters:
	   * - Timer ID (used to distinguish multiple timers of different
	   *   ..types on the same host)
	   * - Timer delay in
	   * - Type of timer
	   */
	  DEBUG_PID(s->pid, "Blink Start\n");
	  sys_timer_start(BLINK_TID, BLINK_TIMER_INTERVAL, TIMER_REPEAT);
	  break;
	}


	/**
	 * MSG_FINAL is used to shut modules down.  Modules should release all
	 * resources at this time and take care of any final protocol
	 * shutdown.
	 */
  case MSG_FINAL:
	{
	  /**
	   * Stop the timer
	   */
	  sys_timer_stop(BLINK_TID);
	  DEBUG_PID(s->pid, "Blink Stop\n");
	  break;
	}


	/**
	 * All timers addressed to this PID, regardless of the timer ID, are of
	 * type MSG_TIMER_TIMEOUT and handled by this handler.  Timers with
	 * different timer IDs can be further distinguished by testing for the
	 * type, as demonstrated in the relay module.
	 */
  case MSG_TIMER_TIMEOUT:
	{
	  sys_led(LED_GREEN_TOGGLE);
	  break;
	}

	/**
	 * The default handler is used to catch any messages that the module
	 * does no know how to handle.
	 */
	default:
	return -EINVAL;
  }

  /**
   * Return SOS_OK for those handlers that have successfully been handled.
   */
  return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr blink_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif


