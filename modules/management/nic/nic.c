/**
 * \file nic.c
 * \brief This module should be installed on the base station of the sensor network.
 * \Description:
 *    1. This module monitors all the messages coming into the node.
 *    2. No action is taken if the message is addressed to this node.
 *    3. If the message is broadcast or not for this node, then a copy of the message is created.
 *    4. The copy of the message is dispatched to the radio if it is received over the
 */
#include <sos.h>
#include <monitor.h>
#include <malloc.h>
#include <sos_uart.h>
#include <sos_info.h>
#include <message_queue.h>
#include <hardware.h>
#include <led.h>
#include "nic.h"

// forward declaration
static int8_t sosbase_handler(void *state, Message *msg);
void send_group(uint16_t daddr, uint8_t did);

static mod_header_t mod_header SOS_MODULE_HEADER =
  {
    .mod_id = SOSBASE_PID,
    .state_size = 0,
    .num_sub_func = 0,
    .num_prov_func = 0,
    .module_handler= sosbase_handler,
  };

static monitor_cb mon_cb;
//static sos_module_t sosbase_module;

static int8_t sosbase_handler(void *state, Message *msg)
{
  if (msg->did == SOSBASE_PID){
    switch (msg->type){
    case MSG_INIT: {
      ker_register_monitor(SOSBASE_PID, MON_NET_INCOMING, &mon_cb);
      return SOS_OK;
    }
    case MSG_SEND_GROUP: {
      send_group(msg->saddr, msg->sid);
      return SOS_OK;
    }
    case MSG_SET_GROUP: {
      sos_group_t *cmd = (sos_group_t*)msg->data;
      ker_set_group(cmd->group);
      send_group(msg->saddr, msg->sid);
      return SOS_OK;
    }
    default:
      return -EINVAL;
    }
  } else {
    sos_module_t *mod;
    Message *m;
    uint8_t* pmsgdata;

    // No action if the message is addressed to this node
    if(msg->daddr == ker_id())
      return SOS_OK;


    // Copy the message header
    m = msg_create();
    if (m == NULL) return -ENOMEM;
    *m = *msg;

    // Copy the message payload
    mod = ker_get_module(msg->did);
    if(mod != NULL) {
      // destination exist...
      // deep copy and forward
      pmsgdata = ker_malloc(msg->len, SOSBASE_PID);
      if(pmsgdata != NULL) {
	// Deep copy because the scheduler will destroy this message
	memcpy(pmsgdata, msg->data, msg->len);
      }
    } else {
      // get the data
      pmsgdata = ker_msg_take_data(SOSBASE_PID, msg);
    }
    if(pmsgdata == NULL && m->len != 0) {
      //! cannot do anything, simply give up
      msg_dispose(m);
      return -ENOMEM;
    }

    // convert to network order and dispatch
    m->daddr = ehtons(m->daddr);
    m->saddr = ehtons(m->saddr);
    m->data = pmsgdata;
    m->flag = SOS_MSG_RELEASE;
    if (flag_msg_from_uart(msg->flag)){
      // send to network
      ker_led(LED_YELLOW_TOGGLE);
      radio_msg_alloc(m);
    } else {
      // send to UART
      ker_led(LED_RED_TOGGLE);
      uart_msg_alloc(m);
    }
  }

  return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr nic_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif


void send_group(uint16_t daddr, uint8_t did)
{
  sos_group_t *reply = (sos_group_t*)ker_malloc(sizeof(sos_group_t),SOSBASE_PID);
  if (reply) {
    reply->group = ker_get_group();
    post_net(
	     did,
	     SOSBASE_PID,
	     MSG_SOS_GROUP,
	     sizeof(sos_group_t),
	     reply,
	     SOS_MSG_RELEASE,
	     daddr);
  }
}
