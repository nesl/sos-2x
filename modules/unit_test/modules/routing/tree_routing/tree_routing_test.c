/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * \file tree_routing_test.c
 * \brief Unit Test for Tree Routing Module
 * \author Ram Kumar {ram@ee.ucla.edu}
 */

#include <sys_module.h>
#include <module.h>
#define LED_DEBUG
#include <led_dbg.h>
#include <routing/tree_routing/tree_routing.h>
#include <unit_test/modules/routing/tree_routing/tree_routing_test.h>


//-------------------------------------------------------------
// MODULE STATE
//-------------------------------------------------------------
/**
 * \brief Local state of the tree routing test module
 */
typedef struct {
  func_cb_ptr get_hdr_size; //! Function pointer to get the size of the tree routing header
  uint32_t seq_no;          //! Sequence number of the current message
} tr_test_state_t;


//-------------------------------------------------------------
// MODULE TIMERS
//-------------------------------------------------------------
enum 
  {
    TR_TEST_TIMER_TID    = 0,
    TR_TEST_BACKOFF_TID  = 1,
    TR_TEST_TIMER_RATE = 4096,
  };

//-------------------------------------------------------------
// STATIC FUNCTIONS
//-------------------------------------------------------------
int8_t tr_test_module(void *state, Message *msg);


//-------------------------------------------------------------
// MODULE HEADER
//-------------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id        = TR_TEST_PID,
  .state_size    = sizeof(tr_test_state_t),
  .num_sub_func  = 1,
  .num_prov_func = 0,
  .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
  .processor_type = MCU_TYPE,
  .code_id       = ehtons(TR_TEST_PID),
  .module_handler = tr_test_module,
  .funct = {
    [0] = {error_8, "Cvv0", TREE_ROUTING_PID, MOD_GET_HDR_SIZE_FID},
  },
};


//-------------------------------------------------------------
// MODULE IMPLEMENTATION
//-------------------------------------------------------------
int8_t tr_test_module(void *state, Message *msg)
{
  tr_test_state_t *s  = (tr_test_state_t*)state;
  
  switch (msg->type){
    
    // Init Message
  case MSG_INIT:
    {
      s->seq_no = 0;
      sys_timer_start(TR_TEST_BACKOFF_TID, sys_rand() % 1024L, TIMER_ONE_SHOT);
      break;
    }

    // Timeout Message
  case MSG_TIMER_TIMEOUT:
    {
      sos_timeout_t* timeout = (sos_timeout_t*)msg->data;
      // Periodic Timer
      if (TR_TEST_TIMER_TID == timeout->tid){
		uint8_t* pkt;
		tr_test_pkt_t* testpkt;
		int8_t hdr_size;
		LED_DBG(LED_YELLOW_TOGGLE);
		hdr_size = SOS_CALL(s->get_hdr_size, get_hdr_size_proto);
		if (hdr_size < 0) {return SOS_OK;}
		pkt = (uint8_t*)sys_malloc(hdr_size + sizeof(tr_test_pkt_t));
		testpkt = (tr_test_pkt_t*)(pkt + hdr_size);
		testpkt->seq_no = s->seq_no;
		testpkt->node_addr = sys_id();
		sys_post(TREE_ROUTING_PID, MSG_SEND_PACKET, hdr_size + sizeof(tr_test_pkt_t),
				 (void*)pkt, SOS_MSG_RELEASE);
		s->seq_no++;
      }
      // Backoff Timer
      else if (TR_TEST_BACKOFF_TID == timeout->tid){
		sys_timer_start(TR_TEST_TIMER_TID, TR_TEST_TIMER_RATE, TIMER_REPEAT);
      }
      break;
    }

    // Tree Routing Data Packet
    // Delivered by the tree routing module to its client at the base-station
  case MSG_TR_DATA_PKT:
    {
      if (sys_id() == BASE_STATION_ADDRESS){
		uint8_t *payload;
		uint8_t msg_len;
		msg_len = msg->len;
		payload = sys_msg_take_data(msg);
		sys_post_uart(TR_TEST_PID, msg->type, msg_len, payload, SOS_MSG_RELEASE, BCAST_ADDRESS);
		LED_DBG(LED_GREEN_TOGGLE);
      }
      break;
    }

  default:
    return -EINVAL;
  }
  return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr tree_routing_test_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif
