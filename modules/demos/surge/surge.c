/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA,
 * 94704.  Attention:  Intel License Inquiry.
 */

/**
 * @brief Surge application
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

/**
 * Periodically samples sensor data and sends it to the tree routing module
 * MODULE DEPENDENCIES - TREE_ROUTING, PHOTOTEMP_SENSOR
 */


#include <sys_module.h>
//#include <module.h>
#include <led_dbg.h>
#include "surge.h"
#include <routing/tree_routing/tree_routing.h>
#include <mts310sb.h>

#ifndef SOS_SURGE_DEBUG
#undef DEBUG
#define DEBUG(...)
#endif

#ifdef TEST_SURGE
// This memory is used by Avrora to collect Surge statistic
static uint16_t data_node_id;
#endif


//-------------------------------------------------------------
// MODULE STATE
//-------------------------------------------------------------
/**
 * @struct surge_state_t
 * @brief State of the Surge module
 * @note Size is 11 bytes
 */
typedef struct {
  func_cb_ptr get_hdr_size; //! Function pointer to get the size of the tree routing header
  int16_t timer_ticks;      //! Count of number of timer ticks
  uint32_t seq_no;          //! Sequence number of the surge message
  sos_pid_t dest_pid;       //! The destination module that will receive the message
  SurgeMsg* smsg;           //! Surge Message pointer
} surge_state_t;



//-------------------------------------------------------------
// MODULE TIMERS
//-------------------------------------------------------------
enum 
  {
	SURGE_TIMER_TID    = 0,
	SURGE_BACKOFF_TID  = 1,
  };


//-------------------------------------------------------------
// STATIC FUNCTIONS
//-------------------------------------------------------------
int8_t surge_module_handler(void *state, Message *msg);


//-------------------------------------------------------------
// MODULE HEADER
//-------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id        = SURGE_MOD_PID,
  .state_size    = sizeof(surge_state_t),
  .num_sub_func  = 1,
  .num_prov_func = 0,
  .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
  .processor_type = MCU_TYPE,
  .code_id       = ehtons(SURGE_MOD_PID),
  .module_handler = surge_module_handler,
  .funct = {
	[0] = {error_8, "Cvv0", TREE_ROUTING_PID, MOD_GET_HDR_SIZE_FID},
  },
};


//-------------------------------------------------------------
// MODULE IMPLEMENTATION
//-------------------------------------------------------------
int8_t surge_module_handler(void *state, Message *msg)
{
  surge_state_t *s = (surge_state_t*)state;

  switch (msg->type){

	//! First message from the SOS kernel
  case MSG_INIT:
	{
	  s->seq_no = 0;
	  sys_timer_start(SURGE_BACKOFF_TID, ker_rand() % 1024L, TIMER_ONE_SHOT);
	  s->dest_pid = SURGE_MOD_PID;
	  break;
	}

	//! Timer timeout message
  case MSG_TIMER_TIMEOUT:
	{
	  MsgParam *param = (MsgParam*) (msg->data);
	  switch (param->byte){
	  case SURGE_TIMER_TID:
		{
		  s->timer_ticks++;
		  if (s->timer_ticks % TIMER_GETADC_COUNT == 0){
			if(sys_sensor_get_data(MTS310_PHOTO_SID) < 0){
			  LED_DBG(LED_RED_TOGGLE);
			  sys_post_value(SURGE_MOD_PID, MSG_DATA_READY, 0xffff, 0);
			}
		  }
		  return SOS_OK;
		}
	  case SURGE_BACKOFF_TID:
		{
		  sys_timer_start(SURGE_TIMER_TID, INITIAL_TIMER_RATE, TIMER_REPEAT);
		  return SOS_OK;
		}
	  default: return -EINVAL;
	  }
	  return SOS_OK;
	}

	// Requested sensor data ready
  case MSG_DATA_READY:
	{
	  MsgParam* param = (MsgParam*) (msg->data);
	  int8_t hdr_size;
	  uint8_t *pkt;
      
	  LED_DBG(LED_YELLOW_TOGGLE);
	  hdr_size = SOS_CALL(s->get_hdr_size, get_hdr_size_proto);
	  if( hdr_size < 0 ) { return SOS_OK; }
	  pkt = (uint8_t*)sys_malloc(hdr_size + sizeof(SurgeMsg));
	  if (pkt == NULL) break; 
	  s->smsg = (SurgeMsg*)(pkt + hdr_size);
	  s->smsg->type = SURGE_TYPE_SENSORREADING;
	  s->smsg->reading = ehtons(param->word);
	  s->smsg->seq_no  = ehtonl(s->seq_no);
	  s->smsg->originaddr = ehtons(ker_id());
	  LED_DBG(LED_YELLOW_TOGGLE);
	  DEBUG("<SURGE> Send Surge PKT addr = %d, seq = %d, val = %d\n",
			s->smsg->originaddr,
			s->smsg->seq_no,
			s->smsg->reading);
	  s->seq_no++;
	  sys_post(TREE_ROUTING_PID, MSG_SEND_PACKET, hdr_size + sizeof(SurgeMsg), (void*)pkt, SOS_MSG_RELEASE);
	  break;
	}

	// Message from the tree routing module upon delivery of the data at the base-station
  case MSG_TR_DATA_PKT:
	{
	  uint8_t hdr_size;
	  SurgeMsg *smsg;
	  hdr_size = SOS_CALL(s->get_hdr_size, get_hdr_size_proto);
	  smsg = (SurgeMsg*)(((uint8_t*)(msg->data)) + hdr_size);
	  DEBUG("<SURGE> Recv Surge PKT addr = %d, seq = %d, val = %d \n",
			entohs(smsg->originaddr),
			entohl(smsg->seq_no),
			entohs(smsg->reading));
	  if (sys_id() == SURGE_BASE_STATION_ADDRESS){
		uint8_t *payload;
		uint8_t msg_len;
		msg_len = msg->len;
		payload = sys_msg_take_data(msg); 
#ifdef TEST_SURGE
		data_node_id = entohs(smsg->originaddr);
#endif
		sys_post_uart(SURGE_MOD_PID,
					  msg->type,
					  msg_len,
					  payload,
					  SOS_MSG_RELEASE,
					  BCAST_ADDRESS);
		LED_DBG(LED_GREEN_TOGGLE);
		return SOS_OK;
	  }
	  break;
	}

	//! Last message from SOS kernel before the module is kicked out
  case MSG_FINAL:
	{
	  break;
	}

  default:
	return -EINVAL;
  }

  return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr surge_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif
