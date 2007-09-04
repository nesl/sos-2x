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
 * @brief Time-synchronization Protocol for Sensor Networks
 * @author Ilias Tsigkogiannis {ilias@ee.ucla.edu}
 */
 
#include <sys_module.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include <string.h> //for memcpy
#include <systime.h>
#include "tpsn.h"

//#define UART_DEBUG 

enum {TPSN_REQUEST, TPSN_REPLY};
enum {EMPTY, FULL};

typedef struct
{
	uint32_t time[2];
	uint8_t type;
	uint8_t seq_no;
} PACK_STRUCT tpsn_req_t;

typedef struct
{
	uint32_t time[2];
	uint8_t type;
	uint8_t seq_no;
	uint32_t previous_time[2];
} PACK_STRUCT tpsn_reply_t;

typedef struct tpsn_node_s
{
	tpsn_t *data;
	uint8_t seq_no;
	uint8_t unused_padding_flags : 4;	
	uint8_t free : 1;
	uint8_t counter :3;
} PACK_STRUCT tpsn_node_t;
	

/**
 * Module can define its own state
 */
typedef struct {
	sos_pid_t pid;
	uint8_t current_seq_no;
	tpsn_node_t tpsn_node[TPSN_BUFFER_SIZE];
} PACK_STRUCT app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t module(void *state, Message *e);
static void add_value(app_state_t *s, tpsn_t *tpsn_ptr);
static tpsn_t* get_data(app_state_t *s, uint8_t seq_no);
/**
 * This is the only global variable one can have.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id         = TPSN_TIMESYNC_PID,
	.state_size     = sizeof(app_state_t),
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(TPSN_TIMESYNC_PID),	
	.module_handler = module,
};


static int8_t module(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	/**
	 * Switch to the correct message handler
	 */
	switch (msg->type)
	{

		case MSG_INIT:
		{
			DEBUG("TPSN: Started\n");
			s->pid = msg->did;
			s->current_seq_no = 0;
			memset(s->tpsn_node, 0, TPSN_BUFFER_SIZE*sizeof(tpsn_node_t));
			return SOS_OK;
		}
		case MSG_GET_INSTANT_TIMESYNC:
		{
			DEBUG("TPSN: Received MSG_GET_INSTANT_TIMESYNC\n");
			tpsn_t* tpsn_ptr = (tpsn_t *)msg->data;
			
			//If the module passed a NULL pointer or if the data size is wrong, then discard
			if( (tpsn_ptr == NULL) || (msg->len < sizeof(tpsn_t) ) )
			{
				LED_DBG(LED_RED_TOGGLE);
				break;
			}
			
			add_value(s, tpsn_ptr);
			
			tpsn_req_t * tpsn_req_ptr = (tpsn_req_t *)sys_malloc(sizeof(tpsn_req_t));
			tpsn_req_ptr->type = TPSN_REQUEST;
			tpsn_req_ptr->seq_no = s->current_seq_no-1;
			DEBUG("TPSN: Transmitting TPSN packet to node %d with seq_no=%d\n", tpsn_ptr->node_id, tpsn_req_ptr->seq_no);			
			sys_post_net(s->pid, MSG_TIMESTAMP, sizeof(tpsn_req_t), tpsn_req_ptr, SOS_MSG_RELEASE, tpsn_ptr->node_id);
		
			break;
		}
		case MSG_TIMESTAMP:
		{
			tpsn_req_t *tpsn_req_ptr = (tpsn_req_t *)msg->data;
			switch(tpsn_req_ptr->type)
			{
				case TPSN_REQUEST:
				{
			        LED_DBG(LED_YELLOW_TOGGLE);
					//LED_DBG(LED_GREEN_TOGGLE);
					DEBUG("TPSN: Received TPSN_REQUEST (seq_no=%d) from node %d\n", tpsn_req_ptr->seq_no, msg->saddr);
					DEBUG("TPSN: Transmitting TPSN_REPLY to node %d at time %d\n", msg->saddr, ker_systime32());
					tpsn_reply_t *tpsn_reply_ptr = (tpsn_reply_t *)sys_malloc(sizeof(tpsn_reply_t));
					memcpy(tpsn_reply_ptr->previous_time, tpsn_req_ptr->time, sizeof(tpsn_req_ptr->time));
					tpsn_reply_ptr->type = TPSN_REPLY;					
					tpsn_reply_ptr->seq_no = tpsn_req_ptr->seq_no;
                    if(msg->saddr == UART_ADDRESS){
					    sys_post_uart(s->pid, MSG_TIMESTAMP, sizeof(tpsn_reply_t), tpsn_reply_ptr, SOS_MSG_RELEASE, msg->saddr);
                    } else {
					    sys_post_net(s->pid, MSG_TIMESTAMP, sizeof(tpsn_reply_t), tpsn_reply_ptr, SOS_MSG_RELEASE, msg->saddr);
                    }
					break;					
				}
				case TPSN_REPLY:
				{
					uint8_t return_msg_type;
					//LED_DBG(LED_YELLOW_TOGGLE);

					DEBUG("TPSN: Received TPSN_REPLY from node %d\n", msg->saddr);
					tpsn_reply_t *tpsn_reply_ptr = (tpsn_reply_t *)msg->data;
					tpsn_t* tpsn_ptr = get_data(s, tpsn_reply_ptr->seq_no);
					
					if(tpsn_ptr == NULL)
						break;
					
					//T1=tpsn_reply_ptr->previous_time[0]
					//T2=tpsn_reply_ptr->previous_time[1]
					//T3=tpsn_reply_ptr->time[0]
					//T4=tpsn_reply_ptr->time[1]
					//CLOCK_DRIFT = ((T2 - T1) - (T4 - T3))/2
					//PROPAGATION_DELAY=((T2 - T1) + (T4 - T3))/2
					
					//Take care of overflow in the node that sent the TPSN request (T1 > T4)
					if(tpsn_reply_ptr->previous_time[0] > tpsn_reply_ptr->time[1])
						tpsn_reply_ptr->time[1] += INT_MAX_GTIME;
						
					//Take care of overflow in the node that sent the TPSN reply (T2 > T3)
					if(tpsn_reply_ptr->previous_time[1] > tpsn_reply_ptr->time[0])
						tpsn_reply_ptr->time[0] += INT_MAX_GTIME;
						
					#ifdef SECURITY_ENABLED
					int32_t propagation_delay;
						
					//Propagation delay must be between [AVG_PROPAGATION_DELAY - SIGMA, AVG_PROPAGATION_DELAY + SIGMA]
					//If it is outside of these bounds, then the data should be invalidated => (msg_type = REPLY_INVALID_PROPAGATION_DELAY), (clock_drift=)
					propagation_delay = ( ((int32_t)tpsn_reply_ptr->previous_time[1] - (int32_t)tpsn_reply_ptr->previous_time[0]) +
											((int32_t)tpsn_reply_ptr->time[1] - (int32_t)tpsn_reply_ptr->time[0]) )/2;
																
					if((propagation_delay < (AVG_PROPAGATION_DELAY - SIGMA)) || 
						(propagation_delay > (AVG_PROPAGATION_DELAY + SIGMA)) )
					{
						DEBUG("TPSN: Discarded clock offset (%d). Propagation delay (%d) not between [%d, %d]\n", 
							 tpsn_ptr->clock_drift, propagation_delay, (AVG_PROPAGATION_DELAY - SIGMA), (AVG_PROPAGATION_DELAY + SIGMA));
						return_msg_type = tpsn_ptr->msg_type;
						tpsn_ptr->msg_type = 0;
						tpsn_ptr->clock_drift = REPLY_INVALID_PROPAGATION_DELAY;
					}
					else
					{
						
					#endif // SECURITY_ENABLED
						
						return_msg_type = tpsn_ptr->msg_type;
						tpsn_ptr->clock_drift = ( ((int32_t)tpsn_reply_ptr->previous_time[1] - (int32_t)tpsn_reply_ptr->previous_time[0]) -
							((int32_t)tpsn_reply_ptr->time[1] - (int32_t)tpsn_reply_ptr->time[0]) )/2;
							
					#ifdef SECURITY_ENABLED
					}
					#endif // SECURITY_ENABLED
												
					DEBUG("TPSN: The clock offset for node %d is %d\n", tpsn_ptr->node_id, tpsn_ptr->clock_drift);												
					sys_post(tpsn_ptr->mod_id, return_msg_type, sizeof(tpsn_t), tpsn_ptr, 0);
					
					#ifdef UART_DEBUG
					tpsn_reply_t *tpsn_temp_ptr = (tpsn_reply_t *)sys_msg_take_data(msg);
					sys_post_uart(s->pid, TPSN_REPLY, sizeof(tpsn_reply_t), tpsn_temp_ptr, SOS_MSG_RELEASE, BCAST_ADDRESS);
					#endif //UART_DEBUG
					
					break;
				}
				default:
				{
					DEBUG("Received unknown packet\n");
					break;
				}
			}
		}
		case MSG_FINAL:
		{
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

void add_value(app_state_t *s, tpsn_t *tpsn_ptr)
{
	uint8_t i;
	uint8_t oldest_position = 0;
	uint8_t found_duplicate = FALSE;
	int8_t empty_position = -1;

	//The idea is that each position has a counter, which is initialized to TPSN_BUFFER_SIZE
	//Whenever we insert a new item we decrease the counter from all the inserted items
	//If a counter reaches zero, then this is the oldest item (so it'll be removed next)
	//If the new request is a duplicate of an old one, then it is stored in the same position
	DEBUG("TPSN: Adding request with sequence number %d to buffer\n", s->current_seq_no);
	
	//Search for empty position
	for(i = 0; i < TPSN_BUFFER_SIZE; i++)
	{
		//If the new request is a duplicate of an old one, then store in the same position
		//Old pointers are not being freed. The memory belongs to the requesting module, 
		//which should have already freed the old space
		if( (found_duplicate == FALSE) && (s->tpsn_node[i].data != NULL) && 
			(s->tpsn_node[i].data->mod_id == tpsn_ptr->mod_id) &&
			(s->tpsn_node[i].data->msg_type == tpsn_ptr->msg_type)	&&
			(s->tpsn_node[i].data->node_id == tpsn_ptr->node_id) )
		{
			DEBUG("TPSN: Writing data in position %d (already occupied by module %d)\n", i, tpsn_ptr->mod_id);
			s->tpsn_node[i].seq_no = s->current_seq_no++;
			s->tpsn_node[i].data = tpsn_ptr;
			s->tpsn_node[i].counter = TPSN_BUFFER_SIZE;
			found_duplicate = TRUE;
		}
		// Empty position found
		else if( (found_duplicate == FALSE) && (empty_position < 0) && (s->tpsn_node[i].free == EMPTY) )
		{
			DEBUG("TPSN: Found empty position (%d)\n", i);
			empty_position = i;
		}
		else if(s->tpsn_node[i].free == FULL)
		{
			s->tpsn_node[i].counter--;
			if(s->tpsn_node[i].counter == 0)
				oldest_position = i;
		}
	}
	
	if(found_duplicate == TRUE)
		return;
	
	if(empty_position >= 0)
	{
		DEBUG("TPSN: Storing to empty position %d\n", empty_position);
		s->tpsn_node[empty_position].free = FULL;
		s->tpsn_node[empty_position].seq_no = s->current_seq_no++;
		s->tpsn_node[empty_position].data = tpsn_ptr;
		s->tpsn_node[empty_position].counter = TPSN_BUFFER_SIZE;
		return;
	}
		
	//No position found, so we will replace the oldest item
	//By definition, a message is sent to the node that created the request
	//by both the msg_type is set to 0 and the clock_drift is set to REPLY_REQUEST_OVERWRITTEN (=1)
	DEBUG("TPSN: Removing old request (%d) with sequence number %d\n", oldest_position, s->tpsn_node[oldest_position].seq_no);
	uint8_t msg_type = s->tpsn_node[oldest_position].data->msg_type;
	s->tpsn_node[oldest_position].data->msg_type = REPLY_REQUEST_OVERWRITTEN;
	s->tpsn_node[oldest_position].data->clock_drift = 0;

	sys_post(tpsn_ptr->mod_id, msg_type, sizeof(tpsn_t), s->tpsn_node[oldest_position].data, 0);
	
	s->tpsn_node[oldest_position].seq_no = s->current_seq_no++;
	s->tpsn_node[oldest_position].data = tpsn_ptr;
	s->tpsn_node[oldest_position].counter = TPSN_BUFFER_SIZE;
	return;
}

tpsn_t* get_data(app_state_t *s, uint8_t seq_no)
{
	uint8_t i;
	uint8_t replaced_counter = 0;
	tpsn_t* tpsn_ptr = NULL;
	DEBUG("TPSN: Retrieving data for sequence number %d\n", seq_no);
	for(i = 0; i < TPSN_BUFFER_SIZE; i++)
	{
		//Found requested item
		if((s->tpsn_node[i].seq_no == seq_no) && (tpsn_ptr == NULL))
		{
			DEBUG("TPSN: Found sequence number %d in buffer\n", seq_no);
			tpsn_ptr = s->tpsn_node[i].data;
			replaced_counter = s->tpsn_node[i].counter;
			s->tpsn_node[i].free = EMPTY;
		}
	}

	//The idea is that if we found the requested item, then we need
	//to increase all the counters that were smaller than its counter
	if(tpsn_ptr!=NULL)
	{
		for(i = 0; i < TPSN_BUFFER_SIZE; i++)
		{
			if(s->tpsn_node[i].counter < replaced_counter)
			{
				s->tpsn_node[i].counter++;
			}
		}
	}
	else
	{
		DEBUG("TPSN: Sequence number %d not found in buffer\n",seq_no );
	}
	
	return tpsn_ptr;
}

#ifndef _MODULE_
mod_header_ptr tpsn_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

