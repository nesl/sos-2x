/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @brief Example module for SOS
 * This module shows some of concepts in SOS
 */

/**
 * Module needs to include <module.h>
 */
#include <module.h>
#define LED_DEBUG
#include <led_dbg.h>

#include <../../../platform/cyclops/include/radioDump.h>

#include "cyclops_nic.h"

#define CYCLOPS_NIC_PID	DFLT_APP_ID0+14
#define CYCLOPS_NIC_TIMER_INTERVAL	224L
#define BLINK_TIMER_INTERVAL	1024L
#define CYCLOPS_NIC_TID               0
#define BLINK_TID               1
#define MSG_RAW_IMAGE_FRAGMENT PLAT_MSG_START+3
#define MSG_DATA_NACK PLAT_MSG_START+4
#define MSG_DATA_ACK PLAT_MSG_START+5
#define MSG_DATA_ACK_BACK PLAT_MSG_START+6
#define MSG_GET_SERVO PLAT_MSG_START+7

#define RADIO_DUMP_PID PROC_MAX_PID+19

#define CAMERA_ADDRESS NODE_ADDR+1

/**
 * Module can define its own state
 */
typedef struct {
  uint8_t pid;
  uint8_t state;
  uint8_t msg_len;
  uint8_t to_i2c;
  radioDumpFrame_t *radioFrame;
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

static int8_t cyclops_nic_msg_handler(void *start, Message *e);

/**
 * This is the only global variable one can have.
 */
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = CYCLOPS_NIC_PID,
  .state_size     = sizeof(app_state_t),
  .num_timers     = 0,
  .num_sub_func   = 0,
  .num_prov_func  = 0,
  .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
  .processor_type = MCU_TYPE,
  .code_id        = ehtons(CYCLOPS_NIC_PID),
  .module_handler = cyclops_nic_msg_handler,
};


static int8_t cyclops_nic_msg_handler(void *state, Message *msg)
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
	  LED_DBG(LED_GREEN_OFF);
	  LED_DBG(LED_YELLOW_OFF);
	  LED_DBG(LED_RED_OFF);
	  s->pid = msg->did;
	  s->state = 0;
	  /**
	   * The timer API takes the following parameters:
	   * - PID of the module the timer is for
	   * - Timer ID (used to distinguish multiple timers of different
	   *   ..types on the same host)
	   * - Type of timer
	   * - Timer delay in
	   */
	  
	  /*
	  	s->radioFrame = (radioDumpFrame_t *) ker_malloc (sizeof (radioDumpFrame_t), RADIO_DUMP_PID);		  
		if (NULL == s->radioFrame)
		{
		return -ENOMEM;
		}		
	  */		  
			
	  DEBUG_PID(s->pid, "Blink Start\n");
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
	  DEBUG_PID(s->pid, "Blink Stop\n");
	  break;
	}


	/**
	 * All timers addressed to this PID, regardless of the timer ID, are of
	 * type MSG_TIMER_TIMEOUT and handled by this handler.  Timers with
	 * different timer IDs can be further distinguished by testing for the
	 * type, as demonstrated in the relay module.
	 */
	
	// from cyclops to air
  case MSG_DATA_ACK_BACK:
  case MSG_RAW_IMAGE_FRAGMENT:
	{
	  LED_DBG(LED_YELLOW_TOGGLE);	
		
	  uint8_t msgLen = msg->len;
	  uint8_t *msgOut = ker_msg_take_data(CYCLOPS_NIC_PID, msg);
		
	  post_net(msg->did, msg->sid, msg->type, msgLen, msgOut, SOS_MSG_RELEASE, BCAST_ADDRESS);
	  //to use the tree routing algorithm use a call
	  // post_long(TREE_ROUTING_PID, msg->sid ,MSG_SEND_PACKET, msgLen, msgOut, SOS_MSG_RELEASE);

	  break;
	}
	
	// from base to cyclops
  case MSG_DATA_ACK:
  case MSG_DATA_NACK:
	{
	  //	  LED_DBG(LED_RED_TOGGLE);
	  // Kevin - do I have to release msg->data also???
		
	  uint8_t msgLen = msg->len;
	  uint8_t *msgIn = ker_msg_take_data(CYCLOPS_NIC_PID, msg);
	  post_i2c(RADIO_DUMP_PID, msg->sid, msg->type, msgLen, msgIn, SOS_MSG_RELEASE, CAMERA_ADDRESS);
	  break;
	}
	
  case MSG_GET_SERVO:
	{
	  uint8_t result =SOS_OK;
	  uint8_t *msgIn = &result;
	  post_i2c(RADIO_DUMP_PID, msg->sid, MSG_GET_SERVO, sizeof(uint8_t), msgIn, SOS_MSG_RELEASE, CAMERA_ADDRESS);
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
mod_header_ptr cyclops_nic_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

