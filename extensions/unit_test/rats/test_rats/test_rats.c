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
 * @brief Rate Adaptive Time Synchronization (RATS) test module
 * @author Ilias Tsigkogiannis {ilias@ee.ucla.edu}
 */
 
#include <module.h>
#include <sys_module.h>
#define LED_DEBUG
#include <led_dbg.h>
#include <rats/rats.h>

#define ROOT_ID 0

#define TIMER_ID 0

#define MSG_REPLY (MOD_MSG_START + 0)

#define TIMESYNC_PERIOD 10240 //10 sec

/**
 * Module can define its own state
 */
typedef struct 
{
	sos_pid_t pid;
} __attribute__ ((packed)) app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t test_rats_msg_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER = 
{
	mod_id : DFLT_APP_ID0,
	state_size : sizeof(app_state_t),
	num_timers : 1,
	num_sub_func : 0,
	num_prov_func : 0,
	platform_type  : HW_TYPE /* or PLATFORM_ANY */,
	processor_type : MCU_TYPE,
	code_id        : ehtons(DFLT_APP_ID0),	
	module_handler : test_rats_msg_handler,	
};


static int8_t test_rats_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;
	MsgParam *p = (MsgParam*)(msg->data);
	
	/**
	 * Switch to the correct message handler
	 */
	switch (msg->type)
	{
		case MSG_INIT:
		{
			s->pid = msg->did;

			if(ker_id() == ROOT_ID)
			{
				DEBUG("RATS_TEST: Node %d starting\n", ker_id());
				LED_DBG(LED_GREEN_OFF);
				return SOS_OK;
			}	
			
			DEBUG("RATS_TEST: Node %d starting\n", ker_id());
			DEBUG("Sending MSG_RATS_CLIENT_START with node_id=%u, precision=%u\n", ROOT_ID, 1);
			post_short(RATS_TIMESYNC_PID, s->pid, MSG_RATS_CLIENT_START, 1, ROOT_ID, 0);			
			
			sys_timer_start(TIMER_ID, TIMESYNC_PERIOD, TIMER_REPEAT);
			return SOS_OK;
		}
		case MSG_TIMER_TIMEOUT:
		{
			switch(p->byte)
			{
				case TIMER_ID:
				{
					DEBUG("RATS_TEST: Node %d asking for time of %d\n", ker_id(), ROOT_ID);
					rats_t *rats_ptr = (rats_t *)ker_malloc(sizeof(rats_t), s->pid);
					rats_ptr->mod_id = s->pid;
					rats_ptr->source_node_id = ker_id();
					rats_ptr->target_node_id = ROOT_ID;
					rats_ptr->time_at_source_node = ker_systime32();
					rats_ptr->msg_type = MSG_REPLY;
					post_long(RATS_TIMESYNC_PID, s->pid, MSG_RATS_GET_TIME, sizeof(rats_t), rats_ptr, SOS_MSG_RELEASE);
					break;					
				}
				default:
					break;
			}
			break;
		}		
		case MSG_REPLY:
		{
			rats_t *rats_ptr = (rats_t *)msg->data;
			DEBUG("RATS_TEST: Converted time %u of current node to time %u of node %d\n", 
				rats_ptr->time_at_source_node, rats_ptr->time_at_target_node, rats_ptr->target_node_id);
			rats_ptr = NULL; //otherwise it won't compile for mica2 (rats_ptr would be unused)
			break;
		}
		case MSG_FINAL:
		{
			post_short(RATS_TIMESYNC_PID, s->pid, MSG_RATS_CLIENT_STOP, 0, ROOT_ID, 0);			
            sys_timer_stop(TIMER_ID);
			return SOS_OK;
		}
		default:
			return -EINVAL;
	}

	/**
	 * Return SOS_OK for those handlers that have successfully been handled.
	 */
	return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr test_rats_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

