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
 * @brief Rate Adaptive Time Synchronization (RATS) module
 * @author Ilias Tsigkogiannis {ilias@ucla.edu}
 */
 
#include <module.h>
#include <sys_module.h>
#include <string.h> // for memcpy
#include <systime.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include "rats.h"

//#define UART_DEBUG
#define USE_PANIC_PACKETS

#define ROOT_NODE 0

#define TRANSMIT_TIMER 0
#define PANIC_TIMER 1
#define VALIDATION_TIMER 2

#define TOTAL_VALIDATION_RETRANSMISSIONS (UNICAST_VALIDATION_RETRANSMISSIONS + BROADCAST_VALIDATION_RETRANSMISSIONS)

#define NO_REQUEST_CREATED 0
#define CREATED_FIRST_REQUEST 1
#define CREATED_NEW_REQUEST 2

#define UART_TIMESTAMP 0
#define UART_PERIOD_CHANGE 1
#define UART_PANIC 2
#define UART_FORWARD_EXT 3
#define UART_LINEAR_DBG 4
#define UART_BUFFER 5
#define UART_CALC_DEBUG 6
#define UART_PERIOD_REQUEST 7

#define MSG_RATS_SERVER_START (MOD_MSG_START + 3)
#define MSG_RATS_SERVER_STOP (MOD_MSG_START + 4)
#define MSG_PERIOD_REQUEST (MOD_MSG_START + 5)
#define MSG_PERIOD_REPLY (MOD_MSG_START + 6)
#define MSG_PERIOD_CHANGE (MOD_MSG_START + 7)
#define MSG_PANIC (MOD_MSG_START + 8)
#define MSG_INVALIDATE_ENTRY (MOD_MSG_START + 9)

enum {NORMAL_PACKET, TEST_PACKET};

typedef struct
{
	uint32_t time[2];	//must always be the first field of the struct (filled by the network stack)
	uint8_t type;		//must always be second, in order to discriminate between normal and test packets
	float a;
	float b;	
	uint16_t sampling_period;
	uint16_t node_id;
	float est_error;
} __attribute__ ((packed)) test_packet_t;

typedef struct
{
	uint32_t time[2];	//must always be the first field of the struct (filled by the network stack)
	uint8_t type;		//must always be second, in order to discriminate between normal and test packets
	uint16_t node_id;
} __attribute__ ((packed)) ext_packet_t;


typedef struct
{
	float time[2];
	uint8_t type;
	uint32_t int_parent_time;	
	uint16_t node_id;
} __attribute__ ((packed)) debug_packet_t;


typedef struct
{
  uint16_t saddr;
	uint16_t old_period;
	uint16_t new_period;
} __attribute__ ((packed)) period_packet_t;

typedef struct
{
	uint32_t time[2];	//must always be the first field of the struct (filled by the network stack)
	uint8_t type;		//must always be second, in order to discriminate between normal and test packets
	uint16_t transmission_period;
	uint16_t min_period_node_id;
} __attribute__ ((packed)) ts_packet_t;

typedef struct timesync
{
	uint16_t node_id;
	uint32_t *timestamps;
	uint32_t *my_time;
	float a;
	float b;
	uint16_t sampling_period;
	uint8_t sync_precision;
	uint8_t packet_count;
	uint8_t window_size;
	uint16_t panic_timer_counter; //used to count how many times the panic counter has fired
	uint8_t panic_timer_retransmissions;
	uint8_t ref_counter;
	struct timesync *next;
} __attribute__ ((packed)) timesync_t;

/**
 * Module can define its own state
 */
typedef struct {
	sos_pid_t pid;
	ts_packet_t ts_packet;
	timesync_t *ts_list;
	uint16_t validation_node_id;	
	uint16_t validation_period;
	uint8_t validation_timer_retransmissions;
	uint16_t validation_timer_counter; //used to count how many times the validation counter has fired
	uint16_t transmit_timer_counter; //used to count how many times the transmission counter has fired	
} __attribute__ ((packed)) app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t module(void *start, Message *e);
static uint8_t add_request(app_state_t *s, uint16_t node_id, uint8_t sync_precision);
static uint8_t add_values(app_state_t *s, Message *msg);
static timesync_t * get_timesync_ptr(app_state_t *s, uint16_t node_id);
void getRegression(uint32_t* pTSParentArray, uint32_t* pTSMyArray, uint8_t max_window, uint8_t size,
				uint8_t slot, float* alpha, float* beta);
float getError(uint32_t* pTSParentArray, uint32_t* pTSMyArray, uint8_t max_window, uint8_t size, 
				uint8_t slot, float* alpha, float* beta, uint16_t period /* sec */, uint8_t should_invert);

//The following functions are used to convert from clock ticks to milliseconds
//and the opposite. The used clock frequency is supposed to be 115.2KHz.
static inline float ticks_to_msec_float(int32_t ticks) 
{
	return ticks/115.2;
}

static inline float msec_to_ticks_float(int32_t msec) 
{
	return msec*115.2;
}				
				
//The following functions are only used for debugging using the UART
#ifdef UART_DEBUG
static void send_debug_packet(app_state_t *s, ts_packet_t *ts_packet_ptr, float est_error);
static void send_buffer(app_state_t *s, uint16_t root_node_id);
static void send_buffer2(app_state_t *s, uint16_t root_node_id);
#endif //UART_DEBUG				
																
static mod_header_t mod_header SOS_MODULE_HEADER = 
{
	mod_id : RATS_TIMESYNC_PID,
	state_size : sizeof(app_state_t),
	num_timers : 3,
	num_sub_func : 0,
	num_prov_func : 0,
	platform_type  : HW_TYPE /* or PLATFORM_ANY */,
	processor_type : MCU_TYPE,
	code_id        : ehtons(RATS_TIMESYNC_PID),
	module_handler : module,	
};


static int8_t module(void *state, Message *msg)
{
    app_state_t *s = (app_state_t *) state;
	MsgParam *p = (MsgParam*)(msg->data);
	
    /**
     * Switch to the correct message handler
     */
    switch (msg->type){

        case MSG_INIT:
		{
			DEBUG("RATS: node %d initializing\n", ker_id());
			s->pid = msg->did;
			s->ts_list = NULL;
			s->ts_packet.type = NORMAL_PACKET;

			//Notify neighbors that RATS is starting (in case node rebooted while it was
			//synchronizing with another node
			post_net(s->pid, s->pid, MSG_INVALIDATE_ENTRY, 0, NULL, 0, BCAST_ADDRESS);
			return SOS_OK;
		}
		case MSG_RATS_CLIENT_START:
		{
			MsgParam *p  = (MsgParam *)msg->data;
			DEBUG("RATS: Received MSG_RATS_CLIENT_START for node %d\n", p->word);
			
			uint8_t request_status = add_request(s, p->word, p->byte);

			//If a new request was created, then send packet to parent
			if(request_status != NO_REQUEST_CREATED)
			{
				DEBUG("RATS: Transmitting request to node %d\n", p->word);
				LED_DBG(LED_RED_TOGGLE);
				//If the current node is the parent of the target node, then the target node will
				//reply by informing the parent, who will add the target to its list of children.
				post_net(s->pid, s->pid, MSG_RATS_SERVER_START, 0, NULL, 0, p->word);
			}
			else
			{
				//Request already exists
				DEBUG("RATS: Request already exists\n");
			}
			
			//If this was the first request that was created, we need to start the panic timer
			if(request_status == CREATED_FIRST_REQUEST)
			{
				DEBUG("RATS: PANIC_TIMER started\n");
				
				#ifdef USE_PANIC_PACKETS
				sys_timer_start(PANIC_TIMER, MIN_SAMPLING_PERIOD*1024, TIMER_REPEAT);
				#endif //USE_PANIC_PACKETS
			}			

			return SOS_OK;
		}
		case MSG_RATS_SERVER_START:
		{
			timesync_t * temp_ts_ptr = get_timesync_ptr(s, msg->saddr);

			DEBUG("RATS: Received request from node %d\n", msg->saddr);
			
			if(temp_ts_ptr == NULL)
			{
				DEBUG("RATS: Starting timesync with node %d\n", msg->saddr);
				LED_DBG(LED_RED_TOGGLE);
				//If request is coming from node, with whom the node is not synchronizing, then
				//synchronization is starting
				sys_timer_stop(TRANSMIT_TIMER);
				sys_timer_stop(VALIDATION_TIMER);				
				s->ts_packet.transmission_period = INITIAL_TRANSMISSION_PERIOD;	
				s->ts_packet.min_period_node_id = msg->saddr;	
				s->transmit_timer_counter = 1; //s->ts_packet.transmission_period/INITIAL_TRANSMISSION_PERIOD;
				s->validation_timer_counter = 5; //s->transmit_timer_counter + 4;
				s->validation_timer_retransmissions = TOTAL_VALIDATION_RETRANSMISSIONS;
				sys_timer_start(TRANSMIT_TIMER, MIN_SAMPLING_PERIOD*1024, TIMER_REPEAT);	
				sys_timer_start(VALIDATION_TIMER, MIN_SAMPLING_PERIOD*1024, TIMER_REPEAT);				
			}

			return SOS_OK;
		}
		case MSG_RATS_GET_TIME:
		{
			//If the module passed a NULL pointer or if the data size is wrong, then discard
			if( (msg->data == NULL) 
			#ifndef PC_PLATFORM
			 || (msg->len != sizeof(rats_t) ) 
			#endif //PC_PLATFORM
			 )
			{
				DEBUG("RATS: Invalid data received in MSG_RATS_GET_TIME\n");
				break;
			}
			rats_t * rats_ptr = (rats_t *)sys_msg_take_data(msg);
			DEBUG("RATS: Received MSG_RATS_GET_TIME (mod_id=%d node=%d)\n", rats_ptr->mod_id, msg->saddr);			
			
			if(rats_ptr->source_node_id == ker_id())
			{
				timesync_t * temp_ts_ptr = get_timesync_ptr(s, rats_ptr->target_node_id);
				if(temp_ts_ptr == NULL)
				{
					DEBUG("RATS: Target node %d is not time synced\n", rats_ptr->target_node_id);
					sys_free(rats_ptr);
					break;
				}
				else
				{
					DEBUG("RATS: Calculating time for target node %d locally\n", rats_ptr->target_node_id);
					if(temp_ts_ptr->packet_count < BUFFER_SIZE) // learning state
					{
						rats_ptr->time_at_target_node = 0;
						rats_ptr->error = 0;
					}
					else
					{
						rats_ptr->time_at_target_node = convert_from_mine_to_parent_time(rats_ptr->time_at_source_node, rats_ptr->target_node_id);
						rats_ptr->error	= getError(&temp_ts_ptr->timestamps[0], &temp_ts_ptr->my_time[0], BUFFER_SIZE,
							temp_ts_ptr->window_size, BUFFER_SIZE - temp_ts_ptr->window_size, &temp_ts_ptr->a, &temp_ts_ptr->b, temp_ts_ptr->sampling_period, FALSE);
					}
				}
			}
			else if (rats_ptr->target_node_id == ker_id())
			{
				timesync_t * temp_ts_ptr = get_timesync_ptr(s, rats_ptr->source_node_id);
				if(temp_ts_ptr == NULL)
				{
					DEBUG("RATS: Source node %d is not time synced\n", rats_ptr->source_node_id);
					sys_free(rats_ptr);
					break;
				}
				else
				{
					DEBUG("RATS: Calculating time for source node %d locally\n", rats_ptr->source_node_id);
					if(temp_ts_ptr->packet_count < BUFFER_SIZE) // learning state
					{
						rats_ptr->time_at_target_node = 0;
						rats_ptr->error = 0;
					}
					else
					{
						rats_ptr->time_at_target_node = convert_from_parent_to_my_time(rats_ptr->time_at_source_node, rats_ptr->source_node_id);
						rats_ptr->error	= getError(&temp_ts_ptr->timestamps[0], &temp_ts_ptr->my_time[0], BUFFER_SIZE,
							temp_ts_ptr->window_size, BUFFER_SIZE - temp_ts_ptr->window_size, &temp_ts_ptr->a, &temp_ts_ptr->b, temp_ts_ptr->sampling_period, TRUE);
					}
				}
				
			}
			else
			{
				DEBUG("RATS: Invalid request (source = %d, target - %d)\n", rats_ptr->source_node_id, rats_ptr->target_node_id);
				sys_free(rats_ptr);
				break;
			}

			DEBUG("RATS: Sending reply to module %d\n", rats_ptr->mod_id);
			post_long(rats_ptr->mod_id, s->pid, rats_ptr->msg_type, sizeof(rats_t), rats_ptr, SOS_MSG_RELEASE);
			break;
		}
		case MSG_RATS_CLIENT_STOP:
		{
			MsgParam *p  = (MsgParam *)msg->data;
			uint16_t node_id = p->word;
			
			//First we need to remove node from list of parents
			/* Select node at head of list */
			timesync_t * ts_list_ptr = s->ts_list;
			timesync_t * ts_delete_list_ptr;
			timesync_t * ts_previous_list_ptr = s->ts_list;
		
			/* Loop until we've reached the end of the list */
			while( ts_list_ptr != NULL )
			{
				if(ts_list_ptr->node_id == node_id)
				{
					if(--ts_list_ptr->ref_counter > 0)
						return SOS_OK;
					
					DEBUG("RATS: Removing node %d from list of parents. Sending MSG_RATS_SERVER_STOP.\n", node_id);
					post_net(s->pid, s->pid, MSG_RATS_SERVER_STOP, 0, NULL, 0, node_id);

					/* Found the item to be deleted,
		     		 re-link the list around it */
					if( ts_list_ptr == s->ts_list )
						/* We're deleting the head */
						s->ts_list = ts_list_ptr->next;
					else
						ts_previous_list_ptr->next = ts_list_ptr->next;
		
					ts_delete_list_ptr = ts_list_ptr;
					ts_list_ptr = ts_list_ptr->next;
					
					/* Free the node */
					sys_free( ts_delete_list_ptr );
					
					//If the parent list is empty, then we're stopping the panic timer
					if(s->ts_list == NULL)
					{
						DEBUG("RATS: Parent list is empty. Stopping panic timer\n");
						
						#ifdef USE_PANIC_PACKETS
						sys_timer_stop(PANIC_TIMER);
						#endif //USE_PANIC_PACKETS
					}					
					
					return SOS_OK;
				}
				ts_previous_list_ptr = ts_list_ptr;
				ts_list_ptr = ts_list_ptr->next;
			}
			
			DEBUG("RATS: Requested parent %d was not found\n", node_id);
			
			break;
		}
		case MSG_RATS_SERVER_STOP:
		{
			DEBUG("RATS: Received MSG_RATS_SERVER_STOP from %d\n", msg->saddr);
			//If node has minimum period, then go to validation protocol
			if(msg->saddr == s->ts_packet.min_period_node_id)
			{
				DEBUG("RATS: Going to validation protocol\n");
				s->validation_timer_counter = 1;
				s->validation_timer_retransmissions = BROADCAST_VALIDATION_RETRANSMISSIONS;
				s->validation_node_id = ker_id();
			}

			break;
		}	
		case MSG_TIMER_TIMEOUT:
		{
			switch(p->byte)
			{
				case TRANSMIT_TIMER:
				{
					if( (--(s->transmit_timer_counter)) == 0)
					{
						DEBUG("RATS: Broadcasting MSG_TIMESTAMP packet\n");
						LED_DBG(LED_GREEN_TOGGLE);
						post_net(s->pid, s->pid, MSG_TIMESTAMP, sizeof(ts_packet_t), &s->ts_packet, 0, BCAST_ADDRESS);
						
						#ifdef UART_DEBUG
						post_uart(s->pid, s->pid, UART_TIMESTAMP, sizeof(ts_packet_t), &s->ts_packet, 0, BCAST_ADDRESS);
						#endif //UART_DEBUG
						
						s->transmit_timer_counter = (uint16_t)(s->ts_packet.transmission_period / MIN_SAMPLING_PERIOD);
					}
					break;	
				}
				case VALIDATION_TIMER:
				{
					if( (--(s->validation_timer_counter)) == 0)
					{
						s->validation_timer_counter = 1;
						//Send up to MSG_PERIOD_REQUEST packets (UNICAST_VALIDATION_RETRANSMISSIONS times) to node with minimum period.
						//If the node doesn't respond until then, then broadcast BROADCAST_VALIDATION_RETRANSMISSIONS times
						//After the transmitting BROADCAST_VALIDATION_RETRANSMISSIONS packets, use the minimum period that
						//was sent during that interval
						if( s->validation_timer_retransmissions > BROADCAST_VALIDATION_RETRANSMISSIONS )
						{
							--s->validation_timer_retransmissions;							
							DEBUG("RATS: Transmitting MSG_PERIOD_REQUEST (retries left = %d) to node %d\n", s->validation_timer_retransmissions, s->ts_packet.min_period_node_id);
							post_net(s->pid, s->pid, MSG_PERIOD_REQUEST, 0, NULL, 0, s->ts_packet.min_period_node_id);
							
							#ifdef UART_DEBUG
							post_uart(s->pid, s->pid, UART_PERIOD_REQUEST, 0, NULL, 0, s->ts_packet.min_period_node_id);
							#endif //UART_DEBUG
						}
						else if( s->validation_timer_retransmissions > 0)
						{
							--s->validation_timer_retransmissions;							
							DEBUG("RATS: Broadcasting MSG_PERIOD_REQUEST (retries left = %d)\n", s->validation_timer_retransmissions);
							//Invalidate node with minimum period
							s->validation_node_id = ker_id();
							post_net(s->pid, s->pid, MSG_PERIOD_REQUEST, 0, NULL, 0, BCAST_ADDRESS);
							
							#ifdef UART_DEBUG
							post_uart(s->pid, s->pid, UART_PERIOD_REQUEST, 0, NULL, 0, BCAST_ADDRESS);
							#endif //UART_DEBUG
						}
						else //s->validation_timer_retransmissions == 0
						{
							sys_timer_stop(TRANSMIT_TIMER);
							sys_timer_stop(VALIDATION_TIMER);
							
							//Restart normal procedure only if there was a reply
							if(ker_id() != s->validation_node_id)
							{
								DEBUG("RATS: Setting node %d as the one with min period (%d)\n", 
								s->validation_node_id, s->validation_period);
								s->ts_packet.min_period_node_id = s->validation_node_id;
								s->ts_packet.transmission_period = s->validation_period;
								s->transmit_timer_counter = s->ts_packet.transmission_period/INITIAL_TRANSMISSION_PERIOD;
								s->validation_timer_counter = s->transmit_timer_counter + 4;
								s->validation_timer_retransmissions = TOTAL_VALIDATION_RETRANSMISSIONS;
								sys_timer_start(TRANSMIT_TIMER, MIN_SAMPLING_PERIOD*1024, TIMER_REPEAT);
								sys_timer_start(VALIDATION_TIMER, MIN_SAMPLING_PERIOD*1024, TIMER_REPEAT);
							}
							else
							{
								DEBUG("RATS: Validation timer expired, without receiving any packets\n");
								sys_timer_stop(TRANSMIT_TIMER);
								sys_timer_stop(VALIDATION_TIMER);								
							}							
						}
					}
					break;
				}
				case PANIC_TIMER:
				{
					//There is a fixed number of retransmissions. If the corresponding counter
					//reaches zero, then the child is removed from the list
					
					/* Select node at head of list */
					timesync_t * ts_list_ptr = s->ts_list;
					timesync_t * ts_delete_list_ptr;
					timesync_t * ts_previous_list_ptr = s->ts_list;

					/* Loop until we've reached the end of the list */
					while( ts_list_ptr != NULL )
					{
						if(--ts_list_ptr->panic_timer_counter == 0)
						{
							if(ts_list_ptr->panic_timer_retransmissions > 0)
							{
								//Transmit the packet
								--ts_list_ptr->panic_timer_retransmissions;								
								DEBUG("RATS: Sending panic packet to node %d (retries=%d)\n", ts_list_ptr->node_id, ts_list_ptr->panic_timer_retransmissions);
								post_net(s->pid, s->pid, MSG_PANIC, 0, NULL, 0, ts_list_ptr->node_id);
								
								//The retransmission period should be INITIAL_TRANSMISSION_PERIOD 
								ts_list_ptr->panic_timer_counter = 1; 
							}
							else
							{
								DEBUG("RATS: Removing node %d from list of parents\n", ts_list_ptr->node_id);
								/* Found the item to be deleted,
	     		 				re-link the list around it */
								if( ts_list_ptr == s->ts_list )
									/* We're deleting the head */
									s->ts_list = ts_list_ptr->next;
								else
									ts_previous_list_ptr->next = ts_list_ptr->next;
	
								ts_delete_list_ptr = ts_list_ptr;
								ts_list_ptr = ts_list_ptr->next;
				
								/* Free the node */
								sys_free( ts_delete_list_ptr );
								continue;
							}
						}						
						ts_previous_list_ptr = ts_list_ptr;
						ts_list_ptr = ts_list_ptr->next;						
					}
					
					//If the parent list is empty, then we're stopping the panic timer
					if(s->ts_list == NULL)
					{
						DEBUG("RATS: Parent list is empty. Stopping panic timer\n");
						#ifdef USE_PANIC_PACKETS
						sys_timer_stop(PANIC_TIMER);
						#endif //USE_PANIC_PACKETS
					}
					
					break;
				}
				default:
					break;
			}
			return SOS_OK;
		}
		case MSG_PERIOD_CHANGE:
		{
			uint16_t temp_transmission_period;
			DEBUG("RATS: Received packet for period change from %d\n", msg->saddr);
			LED_DBG(LED_YELLOW_TOGGLE);
			if((msg->data == NULL) || (msg->len != sizeof(uint16_t)) )
			{
				DEBUG("RATS: Invalid parameters in MSG_PERIOD_CHANGE\n");
				break;
			}
			
			temp_transmission_period = (* (uint16_t*)(msg->data));

			//Change period if:
			//a)received period is smaller than period in use
			//b)node that sent period is the one that has the current smallest period
			//c)I am currently using myself as the node with the smallest period (used in the beginning and in transitive modes)
			if((temp_transmission_period < s->ts_packet.transmission_period) 
				|| (s->ts_packet.min_period_node_id == msg->saddr) 
				|| (s->ts_packet.min_period_node_id == ker_id()) )
			{
				DEBUG("RATS: Changing period (new_period=%d new_node=%d). Sending to UART\n", temp_transmission_period, msg->saddr);
				sys_timer_stop(TRANSMIT_TIMER);
			    sys_timer_stop(VALIDATION_TIMER);
				
				#ifdef UART_DEBUG
				period_packet_t * period_packet_ptr = sys_malloc(sizeof(period_packet_t));
				period_packet_ptr->saddr = msg->saddr;
				period_packet_ptr->old_period = s->ts_packet.transmission_period;
				period_packet_ptr->new_period = temp_transmission_period;
				post_uart(s->pid, s->pid, UART_PERIOD_CHANGE, sizeof(period_packet_t), period_packet_ptr, SOS_MSG_RELEASE, UART_ADDRESS);
				#endif //UART_DEBUG
				
				s->ts_packet.transmission_period = temp_transmission_period;
				s->ts_packet.min_period_node_id = msg->saddr;
				s->transmit_timer_counter = (uint16_t)(s->ts_packet.transmission_period / MIN_SAMPLING_PERIOD);
				s->validation_timer_counter = s->transmit_timer_counter + 4;
				s->validation_timer_retransmissions = TOTAL_VALIDATION_RETRANSMISSIONS;
				sys_timer_start(TRANSMIT_TIMER, INITIAL_TRANSMISSION_PERIOD*1024, TIMER_REPEAT);
				sys_timer_start(VALIDATION_TIMER, INITIAL_TRANSMISSION_PERIOD*1024, TIMER_REPEAT);
			}
			return SOS_OK;	
		}
		case MSG_TIMESTAMP:
		{
			ts_packet_t *ts_packet_ptr = (ts_packet_t *)msg->data;
			DEBUG("RATS: MSG_TIMESTAMP with type = %d\n", ts_packet_ptr->type);
			if(ts_packet_ptr->type == NORMAL_PACKET)
			{
				DEBUG("RATS: Receiving timestamp data from node %d\n", msg->saddr);
				if( add_values(s, msg) == TRUE)
				{
					LED_DBG(LED_GREEN_TOGGLE);
					DEBUG("RATS: Accessed internal structure\n");
				}
				else
				{
					DEBUG("RATS: Discarding MSG_TIMESTAMP from node %d\n", msg->saddr);
				}
			}
			else // TEST_PACKET
			{
				if(ker_id() == ROOT_NODE)
				{
					DEBUG("RATS: Receiving test data from node %d. Sending to UART\n", msg->saddr);
					#ifdef UART_DEBUG
					ext_packet_t * ext_packet_ptr = (ext_packet_t *)msg->data;
					debug_packet_t * debug_packet_ptr = (debug_packet_t *)sys_malloc(sizeof(debug_packet_t));
					debug_packet_ptr->time[0] = ticks_to_msec_float(ext_packet_ptr->time[0]);
					debug_packet_ptr->time[1] = ticks_to_msec_float(ext_packet_ptr->time[1]);
					debug_packet_ptr->node_id = ker_id();
					debug_packet_ptr->int_parent_time = ext_packet_ptr->time[1];
					post_uart(s->pid, s->pid, UART_FORWARD_EXT, sizeof(debug_packet_t), debug_packet_ptr, SOS_MSG_RELEASE, UART_ADDRESS);
					#endif //UART_DEBUG
				}
				else
				{
					DEBUG("RATS: Receiving test data from node %d. Sending to parent\n", msg->saddr);
					#ifdef UART_DEBUG
					ext_packet_t * ext_packet_ptr = (ext_packet_t *)msg->data;
					uint32_t parent_time = convert_from_mine_to_parent_time(ext_packet_ptr->time[1], ROOT_NODE);
					
					//Break if the parent is not found in the timestamping list
					if(parent_time == 0)
					{
						break;
					}
					
					debug_packet_t * debug_packet_ptr = (debug_packet_t *)sys_malloc(sizeof(debug_packet_t));
					debug_packet_ptr->time[0] = ticks_to_msec_float(ext_packet_ptr->time[0]);
					debug_packet_ptr->time[1] = ticks_to_msec_float(parent_time);
					debug_packet_ptr->int_parent_time = parent_time;
					debug_packet_ptr->node_id = ker_id();					
					post_uart(s->pid, s->pid, UART_FORWARD_EXT, sizeof(debug_packet_t), debug_packet_ptr, SOS_MSG_RELEASE, UART_ADDRESS);
					#endif //UART_DEBUG
				}
			}
			break;
		}
		case MSG_PERIOD_REQUEST:
		{
			DEBUG("RATS: Received MSG_PERIOD_REQUEST packet from node %d\n", msg->saddr);
			timesync_t * temp_ts_ptr = get_timesync_ptr(s, msg->saddr);
			if(temp_ts_ptr == NULL)
			{
				DEBUG("RATS: Discarding MSG_PERIOD_REQUEST\n");
				break;
			}

			uint16_t *sampling_period = (uint16_t *)sys_malloc(sizeof(uint16_t));
			if(sampling_period != NULL)
			{
				*sampling_period = temp_ts_ptr->sampling_period;
				DEBUG("RATS: Sending MSG_PERIOD_REPLY packet (period=%d) to node %d\n", *sampling_period, msg->saddr);
				post_net(s->pid, s->pid, MSG_PERIOD_REPLY, sizeof(uint16_t), sampling_period, SOS_MSG_RELEASE, msg->saddr);
			}
			break;
		}
		case MSG_PERIOD_REPLY:
		{
			uint16_t transmission_period;
			DEBUG("RATS: Received MSG_PERIOD_REPLY packet from node %d\n", msg->saddr);
			memcpy(&transmission_period, &msg->data[0], sizeof(transmission_period));
			s->validation_timer_counter = s->transmit_timer_counter + 4;
			s->validation_timer_retransmissions = TOTAL_VALIDATION_RETRANSMISSIONS;
			if((transmission_period < s->validation_period) || (s->validation_node_id == ker_id() ) )
			{
				DEBUG("RATS: Changing VALIDATION period (new_period=%d new_node=%d)\n", transmission_period, msg->saddr);
				s->validation_period = transmission_period;
				s->validation_node_id = msg->saddr;
			}
			break;
		}
		case MSG_PANIC:
		{
			//Transmit MSG_TIMESTAMP, restart timer, recalculate value for transmit_timer_counter
			sys_timer_stop(TRANSMIT_TIMER);
			sys_timer_stop(VALIDATION_TIMER);
			
			#ifdef UART_DEBUG
			uint16_t *data = (uint16_t *)sys_malloc(sizeof(uint16_t));
			*data = msg->saddr;
			post_uart(s->pid, s->pid, UART_PANIC, sizeof(uint16_t), data, SOS_MSG_RELEASE, UART_ADDRESS);
			#endif //UART_DEBUG
			
			post_net(s->pid, s->pid, MSG_TIMESTAMP, sizeof(ts_packet_t), &s->ts_packet, 0, BCAST_ADDRESS);
			s->transmit_timer_counter = (uint16_t)(s->ts_packet.transmission_period / MIN_SAMPLING_PERIOD);
			s->validation_timer_counter = s->transmit_timer_counter + 4;
			s->validation_timer_retransmissions = TOTAL_VALIDATION_RETRANSMISSIONS;
			sys_timer_start(TRANSMIT_TIMER, INITIAL_TRANSMISSION_PERIOD*1024, TIMER_REPEAT);
			sys_timer_start(VALIDATION_TIMER, INITIAL_TRANSMISSION_PERIOD*1024, TIMER_REPEAT);
			break;
		}
		case MSG_INVALIDATE_ENTRY:
		{
			DEBUG("RATS: Received invalidation message from node %d\n", msg->saddr);
			timesync_t * temp_ts_ptr = get_timesync_ptr(s, msg->saddr);
			if(temp_ts_ptr == NULL)
			{
				DEBUG("RATS: Discarding MSG_INVALIDATE_ENTRY\n");
				break;
			}
			
			DEBUG("RATS: Invalidation entry for node %d\n", msg->saddr);
			temp_ts_ptr->packet_count = 0;
			temp_ts_ptr->a = 0;
			temp_ts_ptr->b = 0;		
			temp_ts_ptr->sampling_period = INITIAL_TRANSMISSION_PERIOD;
			temp_ts_ptr->window_size = (uint8_t)BUFFER_SIZE;
			temp_ts_ptr->panic_timer_counter = 5; //(s->ts_list->sampling_period / INITIAL_TRANSMISSION_PERIOD) + 4;
			temp_ts_ptr->panic_timer_retransmissions = PANIC_TIMER_RETRANSMISSIONS;			
			memset(temp_ts_ptr->timestamps, 0, BUFFER_SIZE*sizeof(uint32_t));
			memset(temp_ts_ptr->my_time, 0, BUFFER_SIZE*sizeof(uint32_t));


			//Notify node to start procedure from beginning
			post_net(s->pid, s->pid, MSG_RATS_SERVER_START, 0, NULL, 0, msg->saddr);
			break;
		}
        case MSG_FINAL:
        {
			sys_timer_stop(TRANSMIT_TIMER);
			sys_timer_stop(VALIDATION_TIMER);
			
			#ifdef USE_PANIC_PACKETS
			sys_timer_stop(PANIC_TIMER);
			#endif //USE_PANIC_PACKETS
			
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

static uint8_t add_request(app_state_t *s, uint16_t node_id, uint8_t sync_precision)
{
	timesync_t *ts_list_ptr;
	
	// Case 1: Empty list
	if(s->ts_list == NULL)
	{
		DEBUG("RATS: Adding first request for node %d\n", node_id);
		s->ts_list = (timesync_t *)sys_malloc(sizeof(timesync_t));
		memset(s->ts_list, 0, sizeof(timesync_t));
		s->ts_list->node_id = node_id;
		s->ts_list->timestamps = (uint32_t *)sys_malloc(BUFFER_SIZE*sizeof(uint32_t));
		s->ts_list->my_time = (uint32_t *)sys_malloc(BUFFER_SIZE*sizeof(uint32_t));
		memset(s->ts_list->timestamps, 0, BUFFER_SIZE*sizeof(uint32_t));
		memset(s->ts_list->my_time, 0, BUFFER_SIZE*sizeof(uint32_t));	
		s->ts_list->a = 0;
		s->ts_list->b = 0;		
		s->ts_list->sampling_period = INITIAL_TRANSMISSION_PERIOD;
		s->ts_list->sync_precision = sync_precision;		
		s->ts_list->packet_count = 0;		
		s->ts_list->window_size = (uint8_t)BUFFER_SIZE;
		s->ts_list->panic_timer_counter = 5; //(s->ts_list->sampling_period / INITIAL_TRANSMISSION_PERIOD) + 4;
		s->ts_list->panic_timer_retransmissions = PANIC_TIMER_RETRANSMISSIONS;			
		s->ts_list->ref_counter = 1;
		s->ts_list->next = NULL;
	
		return CREATED_FIRST_REQUEST;
	}
	
	// Case 2: Entry found
	ts_list_ptr = s->ts_list;
	while(ts_list_ptr)
	{
		//If the new requested sync precision is better than the old one, then
		//keep better sync precision. Update reference counter
		if(ts_list_ptr->node_id == node_id)
		{
			DEBUG("RATS: Found existing request for node %d\n", node_id);
			if(ts_list_ptr->sync_precision > sync_precision)
				ts_list_ptr->sync_precision = sync_precision;
			ts_list_ptr->ref_counter++;
			return NO_REQUEST_CREATED;
		}
		
		if(ts_list_ptr->next != NULL)
			ts_list_ptr = ts_list_ptr->next;
		else
			break;
	}

	// Case 3: Entry not found
	DEBUG("RATS: Adding new request for node %d\n", node_id);
	ts_list_ptr->next = (timesync_t *)sys_malloc(sizeof(timesync_t));
	ts_list_ptr = ts_list_ptr->next;
	memset(ts_list_ptr, 0, sizeof(timesync_t));
	ts_list_ptr->node_id = node_id;
	ts_list_ptr->timestamps = (uint32_t *)sys_malloc(BUFFER_SIZE*sizeof(uint32_t));
	ts_list_ptr->my_time = (uint32_t *)sys_malloc(BUFFER_SIZE*sizeof(uint32_t));
	memset(ts_list_ptr->timestamps, 0, BUFFER_SIZE*sizeof(uint32_t));
	memset(ts_list_ptr->my_time, 0, BUFFER_SIZE*sizeof(uint32_t));	
	ts_list_ptr->a = 0;
	ts_list_ptr->b = 0;	
	ts_list_ptr->sampling_period = INITIAL_TRANSMISSION_PERIOD;	
	ts_list_ptr->sync_precision = sync_precision;
	ts_list_ptr->packet_count = 0;
	ts_list_ptr->window_size = (uint8_t)BUFFER_SIZE;
	ts_list_ptr->panic_timer_counter = 5; //(ts_list_ptr->sampling_period / INITIAL_TRANSMISSION_PERIOD) + 4;
	ts_list_ptr->panic_timer_retransmissions = PANIC_TIMER_RETRANSMISSIONS;			
	ts_list_ptr->ref_counter = 1;	
	ts_list_ptr->next = NULL;		

	return CREATED_NEW_REQUEST;
}

uint8_t add_values(app_state_t *s, Message *msg)
{
	timesync_t *ts_list_ptr;
	ts_packet_t *ts_packet_ptr = (ts_packet_t *)msg->data;
	float est_error = 0;	
	
	// Case 1: Empty list
	if(s->ts_list == NULL)
	{
		return FALSE;
	}
	
	// Case 2: Entry found
	ts_list_ptr = s->ts_list;
	while(ts_list_ptr)
	{
		if(ts_list_ptr->node_id == msg->saddr)
		{
			uint8_t i;
			DEBUG("RATS: Found entry for node %d\n", msg->saddr);
			//first we have to move the old data one position downwards
			for(i = 0; i< BUFFER_SIZE-1 ; i++)
			{
					ts_list_ptr->timestamps[i] = ts_list_ptr->timestamps[i+1];
					ts_list_ptr->my_time[i] = ts_list_ptr->my_time[i+1];
			}
			//afterwards we write the new data in the last position
			ts_list_ptr->timestamps[BUFFER_SIZE-1] = ts_packet_ptr->time[0];
			ts_list_ptr->my_time[BUFFER_SIZE-1] = ts_packet_ptr->time[1];

			//calculate error and new window size
			if(ts_list_ptr->packet_count < BUFFER_SIZE) // learning state
			{
				ts_list_ptr->packet_count++;
				DEBUG("RATS: Learning state: %d packets received\n", ts_list_ptr->packet_count);
			}
			else
			{
				
				getRegression(&ts_list_ptr->timestamps[0], &ts_list_ptr->my_time[0], BUFFER_SIZE,
					ts_list_ptr->window_size, BUFFER_SIZE - ts_list_ptr->window_size, &ts_list_ptr->a, &ts_list_ptr->b);
				
				DEBUG("RATS: est_error * SCALING_FACTOR = %f\n", est_error * SCALING_FACTOR);
				DEBUG("RATS: LOWER_THRESHOLD * sync_precision = %f\n", LOWER_THRESHOLD * ts_list_ptr->sync_precision);

				est_error = getError(&ts_list_ptr->timestamps[0], &ts_list_ptr->my_time[0], BUFFER_SIZE,
					ts_list_ptr->window_size, BUFFER_SIZE - ts_list_ptr->window_size, &ts_list_ptr->a, &ts_list_ptr->b, ts_list_ptr->sampling_period, FALSE);
				if( (est_error) < (LOWER_THRESHOLD * ts_list_ptr->sync_precision))
				{
					if( (ts_list_ptr->sampling_period * 2) <= MAX_SAMPLING_PERIOD)
						ts_list_ptr->sampling_period *= 2;
					else
						ts_list_ptr->sampling_period = MAX_SAMPLING_PERIOD;
					
					DEBUG("RATS: New period (doubled): %d\n", ts_list_ptr->sampling_period);
				}

				else if((est_error) > (HIGHER_THRESHOLD * ts_list_ptr->sync_precision))
				{
					if( (ts_list_ptr->sampling_period / 2) >= MIN_SAMPLING_PERIOD)
						ts_list_ptr->sampling_period /= 2;
					else
						ts_list_ptr->sampling_period = MIN_SAMPLING_PERIOD;
					
					DEBUG("RATS: New period (divided by 2): %d\n", ts_list_ptr->sampling_period);
				}
				else 
				{
					DEBUG("RATS: Period remains constant: %d\n", ts_list_ptr->sampling_period);
				}

				// window size has to be in the limits of [2, BUFFER_SIZE]
				uint8_t temp = (uint8_t)(TIME_CONSTANT/ts_list_ptr->sampling_period);
				if((temp >= 2)&& (temp <= BUFFER_SIZE))
				{
					ts_list_ptr->window_size = temp;
				}
				else if (temp < 2)
					ts_list_ptr->window_size = (uint8_t)2;

				DEBUG("RATS: Current window size : %d\n", ts_list_ptr->window_size);
			}
			
			// send packet only if calculated period is less than the one used by the parent
			// or if I am the node that has the minimum period or if transmitter has set
			// his id equal to the id with the minimum period (used in transitive modes)
			if((ts_list_ptr->sampling_period < ts_packet_ptr->transmission_period) 
				|| (ts_packet_ptr->min_period_node_id == ker_id())
				|| (ts_packet_ptr->min_period_node_id == msg->saddr) )
			{

				DEBUG("RATS: new_period=min OR I am min_period_node => transmit new_period\n");
				post_net(s->pid, s->pid, MSG_PERIOD_CHANGE, sizeof(uint16_t),
					&ts_list_ptr->sampling_period, 0, msg->saddr);
			}				
			
			//Update fields for panic packets
			ts_list_ptr->panic_timer_retransmissions = PANIC_TIMER_RETRANSMISSIONS;
			ts_list_ptr->panic_timer_counter = (ts_list_ptr->sampling_period / INITIAL_TRANSMISSION_PERIOD) + 4;
			LED_DBG(LED_YELLOW_TOGGLE);
			
			#ifdef UART_DEBUG
			send_debug_packet(s, ts_packet_ptr, est_error);		
			#endif //UART_DEBUG
			
			return TRUE;
		}
		
		if(ts_list_ptr->next != NULL)
			ts_list_ptr = ts_list_ptr->next;
		else
			break;
	}
	
	// Case 3: Entry not found
	return FALSE;
}

timesync_t * get_timesync_ptr(app_state_t *s, uint16_t node_id)
{
	timesync_t * ts_list_ptr = s->ts_list;
	while(ts_list_ptr)
	{
		if(ts_list_ptr->node_id == node_id)
			return ts_list_ptr;
			
		ts_list_ptr = ts_list_ptr->next;
	}
	return NULL;
}

uint32_t convert_from_mine_to_parent_time(uint32_t time, uint16_t parent_node_id)
{
	app_state_t *s = (app_state_t *)ker_get_module_state(RATS_TIMESYNC_PID);
	
	timesync_t * temp_ts_ptr = get_timesync_ptr(s, parent_node_id);
	float parent_time;
	uint8_t i;
	
	if((temp_ts_ptr == NULL) ||
		(temp_ts_ptr->packet_count < BUFFER_SIZE) ) // learning state
		return 0;

	//If the current value is bigger than last one, then there was an overflow
	if( (temp_ts_ptr->my_time[BUFFER_SIZE-1]) > time )
	{
		time += INT_MAX_GTIME;
	}
	else
	{	
		//if any value in the buffer is smaller than its previous value, then there
		//was an overflow
		for(i = (BUFFER_SIZE - temp_ts_ptr->window_size); i < BUFFER_SIZE-1; i++)
		{
			if( temp_ts_ptr->my_time[i] > temp_ts_ptr->my_time[i+1] )
			{
				time += INT_MAX_GTIME;
				break;
			}
		} 
	} 
		
	parent_time = msec_to_ticks_float(temp_ts_ptr->a) + (temp_ts_ptr->b * time) + AVG_PROPAGATION_DELAY;
	
	if(parent_time > INT_MAX_GTIME)
		parent_time -= INT_MAX_GTIME;
	else if(parent_time < 0)
		parent_time += INT_MAX_GTIME;
	
	if(parent_time == 0.0) //0.0 should be returned only in error cases
		return 1;
	else
		return (uint32_t)parent_time;
}

uint32_t convert_from_parent_to_my_time(uint32_t time, uint16_t child_node_id)
{
	app_state_t *s = (app_state_t *)ker_get_module_state(RATS_TIMESYNC_PID);
	
	timesync_t * temp_ts_ptr = get_timesync_ptr(s, child_node_id);
	float child_time;
	uint8_t i;
	
	if((temp_ts_ptr == NULL) ||
		(temp_ts_ptr->packet_count < BUFFER_SIZE) || // learning state
		(temp_ts_ptr->b == 0) ) //sanity check to avoid division by zero
		return 0;
		
	//If the current value is bigger than last one, then there was an overflow
	if( (temp_ts_ptr->timestamps[BUFFER_SIZE-1]) > time )
	{
		time += INT_MAX_GTIME;
	}
	else
	{	
		//if any value in the buffer is smaller than its previous value, then there
		//was an overflow
		for(i = (BUFFER_SIZE - temp_ts_ptr->window_size); i < BUFFER_SIZE-1; i++)
		{
			if( temp_ts_ptr->timestamps[i] > temp_ts_ptr->timestamps[i+1] )
			{
				time += INT_MAX_GTIME;
				break;
			}
		} 
	} 
		
	child_time = (time - msec_to_ticks_float(temp_ts_ptr->a) - AVG_PROPAGATION_DELAY)/(temp_ts_ptr->b);

	if(child_time > INT_MAX_GTIME)
		child_time -= INT_MAX_GTIME;
	else if(child_time < 0)
		child_time += INT_MAX_GTIME;
	
	if(child_time == 0.0) //0.0 should be returned only in error cases
		return 1;
	else
		return (uint32_t)child_time;
}

//The following functions are only used for debugging using the UART
#ifdef UART_DEBUG
void send_buffer(app_state_t *s, uint16_t root_node_id)
{
/*	uint8_t i;
	timesync_t * temp_ts_ptr = get_timesync_ptr(s, root_node_id);
	if(temp_ts_ptr == NULL)
		return;
	
	uint32_t *buffer = (uint32_t *)sys_malloc(BUFFER_SIZE*sizeof(uint32_t));

	for(i=0; i<BUFFER_SIZE; i++)
	{
		buffer[i] = temp_ts_ptr->my_time[i];
	}

	post_uart(s->pid, s->pid, UART_BUFFER, BUFFER_SIZE*sizeof(uint32_t), (uint8_t*)buffer, SOS_MSG_RELEASE, UART_ADDRESS);*/
	
	return;
}

void send_buffer2(app_state_t *s, uint16_t root_node_id)
{
/*	uint8_t i;
	timesync_t * temp_ts_ptr = get_timesync_ptr(s, root_node_id);
	if(temp_ts_ptr == NULL)
		return;
	
	uint32_t *buffer = (uint32_t *)sys_malloc(BUFFER_SIZE*sizeof(uint32_t));

	for(i=0; i<BUFFER_SIZE; i++)
	{
		buffer[i] = temp_ts_ptr->timestamps[i];
	}
	
	post_uart(s->pid, s->pid, UART_BUFFER, BUFFER_SIZE*sizeof(uint32_t), (uint8_t*)buffer, SOS_MSG_RELEASE, UART_ADDRESS);*/
	
	return;
}


void send_debug_packet(app_state_t *s, ts_packet_t *ts_packet_ptr, float est_error)
{
	timesync_t * temp_ts_ptr = get_timesync_ptr(s, ROOT_NODE);
	test_packet_t *test_packet_ptr = (test_packet_t *)sys_malloc(sizeof(test_packet_t));
	if( temp_ts_ptr == NULL )
	{
		test_packet_ptr->a = 0;
		test_packet_ptr->b = 0; 
		test_packet_ptr->sampling_period = 2;
	}
	else
	{
		test_packet_ptr->a = temp_ts_ptr->a;
		test_packet_ptr->b = temp_ts_ptr->b; 
		test_packet_ptr->sampling_period = temp_ts_ptr->sampling_period;
	}
	test_packet_ptr->time[0] = ts_packet_ptr->time[0];
	test_packet_ptr->time[1] = ts_packet_ptr->time[1];
					
	test_packet_ptr->type = TEST_PACKET;
	test_packet_ptr->node_id = ker_id();
	test_packet_ptr->est_error = est_error;

	post_uart(s->pid, s->pid, UART_CALC_DEBUG, sizeof(test_packet_t), test_packet_ptr, SOS_MSG_RELEASE, UART_ADDRESS);		

	send_buffer(s, ROOT_NODE);
	send_buffer2(s, ROOT_NODE);
}

#endif //UART_DEBUG

#ifndef _MODULE_
mod_header_ptr rats_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif //_MODULE_
