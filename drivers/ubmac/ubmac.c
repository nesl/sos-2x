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
 * @brief Uncertainty-Driven BMAC (UBMAC) module
 * @author Ilias Tsigkogiannis {ilias@ucla.edu}
 */
 
#include <module.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include <CC1000Const.h>
#include <cc1k.h>
#include <cc1k_lpl.h>
#include <timesync/rats/rats.h>
#include <timesync/tpsn/tpsn.h>
#include "include/ubmac.h"

#define UBMAC_TIMER_INTERVAL	61440L // 1 minute
#define UBMAC_TID               0
#define INITIAL_TRANSMISSION_INTERVAL	1 // minute
#define NORMAL_TRANSMISSION_INTERVAL	10 //minutes
#define LEARNING_PERIOD_TOTAL_TIME		5 //minutes

#define MSG_RECEIVE_INFO (MOD_MSG_START + 2)
#define MSG_REQUEST_INFO (MOD_MSG_START + 3)
#define MSG_TIMESYNC_REPLY (MOD_MSG_START + 4)

#define MINIMUM_TIMER_RETURN_TIME 1152 //clock ticks (10 msec)


#define CLOCK_TICK_IN_8MHZ_CLK	8.682F
#define TIMER_TICK_IN_MICROSEC	976.5625F

#define TIMER_TO_CLOCK_CONVERSION 113.439F

#define ALWAYS_ON 1

// We define leverage terms to take into account unpredictability of sleeping and waking up
// These leverages are defined from the perspective of the sender side
#define LEVERAGE_PREAMBLE 24 // bytes

typedef struct
{
	uint32_t wakeup_time;
	uint32_t period;
} __attribute__ ((packed)) ubmac_transmit_t;

typedef struct ubmac_data
{
	uint16_t node_id;	
	int32_t wakeup_time;
	int32_t untranslated_wakeup_time;
	uint8_t is_wakeup_time_valid; //boolean variable
	uint32_t period;
	uint8_t ref_count;
	uint8_t timesync_mod_id;
	uint32_t sync_precision; 
	struct ubmac_data *next;
}ubmac_data_t;

/**
 * Module can define its own state
 */
typedef struct 
{
	uint8_t pid;
	ubmac_transmit_t my_info;
	ubmac_data_t *dest_info;
	uint16_t timer_counter;
	uint16_t learning_interval_counter;
} __attribute__ ((packed)) app_state_t;

static uint8_t add_info(app_state_t *s, ubmac_init_t * ubmac_init_ptr);
static void update_info(app_state_t *s, uint16_t node_id, ubmac_transmit_t *ubmac_transmit_ptr);
static ubmac_data_t * get_info(app_state_t *s, uint16_t node_id);
static void update_translated_time(app_state_t *s, uint8_t timesync_mod_id, void *data);
static uint8_t remove_info(app_state_t *s, uint16_t node_id);

/**
 * All modules should included a handler for MSG_INIT to initialize module
 * state, and a handler for MSG_FINAL to release module resources.
 */

static int8_t ubmac_msg_handler(void *start, Message *e);

/**
 * This is the only global variable one can have.
 */
static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = UBMAC_PID,
	.state_size     = sizeof(app_state_t),
    .num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(UBMAC_PID),
	.module_handler = ubmac_msg_handler,
};


static int8_t ubmac_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t*)state;
	switch (msg->type)
	{
		case MSG_INIT:
		{
			s->pid = msg->did;
			
			s->my_info.wakeup_time = 0;
			s->my_info.period = ALWAYS_ON; //it will remain ALWAYS_ON, even if notifyUbmac is never called

			s->dest_info = NULL;
			s->timer_counter = INITIAL_TRANSMISSION_INTERVAL;
			s->learning_interval_counter = LEARNING_PERIOD_TOTAL_TIME;
			ker_timer_init(s->pid, UBMAC_TID, TIMER_REPEAT);
			ker_timer_start(s->pid, UBMAC_TID, UBMAC_TIMER_INTERVAL);
			break;
		}
		case MSG_TIMER_TIMEOUT:
		{
			if(--s->timer_counter == 0)
			{
				post_net(s->pid, s->pid, MSG_RECEIVE_INFO, sizeof(ubmac_transmit_t), &s->my_info, 0, BCAST_ADDRESS);
				if(s->learning_interval_counter > 0)
				{
					s->learning_interval_counter--;
					s->timer_counter = INITIAL_TRANSMISSION_INTERVAL;
				}
				else
					s->timer_counter = NORMAL_TRANSMISSION_INTERVAL;
			}
			break;
		}
		case MSG_START_TIMESYNC:
		{
			ubmac_init_t * ubmac_init_ptr = (ubmac_init_t *)msg->data;
			
			if(add_info(s, ubmac_init_ptr) == TRUE)
			{
				switch(ubmac_init_ptr->timesync_mod_id)
				{
					case RATS_TIMESYNC_PID:
					{
						post_short(RATS_TIMESYNC_PID, s->pid, MSG_RATS_CLIENT_START, ubmac_init_ptr->sync_precision, ubmac_init_ptr->node_id, 0);			
						break;						
					}
					case TPSN_TIMESYNC_PID:
					{
						break;
					}
					default:
					{
						break;
					}
				}
				post_net(s->pid, s->pid, MSG_REQUEST_INFO, 0, NULL, 0, ubmac_init_ptr->node_id);				
			}
			break;
		}
		case MSG_REQUEST_INFO:
		{
			post_net(s->pid, s->pid, MSG_RECEIVE_INFO, sizeof(ubmac_transmit_t), &s->my_info, 0, msg->saddr);	
			break;		
		}		
		case MSG_RECEIVE_INFO:
		{
			ubmac_transmit_t *ubmac_transmit_ptr = (ubmac_transmit_t *)msg->data;
			update_info(s, msg->saddr, ubmac_transmit_ptr);
			break;
		}
		case MSG_TIMESYNC_REPLY:
		{
			update_translated_time(s, msg->sid, msg->data);
			break;
		}
		case MSG_STOP_TIMESYNC:
		{
			MsgParam *p  = (MsgParam *)msg->data;
			if(remove_info(s, p->word) == TRUE)
			{
				post_short(RATS_TIMESYNC_PID, s->pid, MSG_RATS_CLIENT_STOP, 0, p->word, 0);
			}
			break;
		}
		case MSG_FINAL:
		{
			ker_timer_stop(s->pid, UBMAC_TID);
			break;
		}
		default:
			return -EINVAL;
	}

	return SOS_OK;
}

//Returns TRUE if new entry is created or sync precision is updated
static uint8_t add_info(app_state_t *s, ubmac_init_t * ubmac_init_ptr)
{
	ubmac_data_t *ubmac_data_ptr;
	
	// Case 1: Empty list
	if(s->dest_info == NULL)
	{
		s->dest_info = (ubmac_data_t *)ker_malloc(sizeof(ubmac_data_t), s->pid);
		s->dest_info->node_id = ubmac_init_ptr->node_id;
		s->dest_info->wakeup_time = 0;
		s->dest_info->is_wakeup_time_valid = FALSE;
		s->dest_info->period = 0;
		s->dest_info->ref_count = 1;
		s->dest_info->sync_precision = ubmac_init_ptr->sync_precision;
		s->dest_info->timesync_mod_id = ubmac_init_ptr->timesync_mod_id;
		s->dest_info->next = NULL;
		return TRUE;
	}
	
	// Case 2: Entry found
	ubmac_data_ptr = s->dest_info;
	while(ubmac_data_ptr)
	{
		if(ubmac_data_ptr->node_id == ubmac_init_ptr->node_id)
		{
			ubmac_data_ptr->ref_count++;
			if(ubmac_data_ptr->sync_precision  > ubmac_init_ptr->sync_precision)
			{
				ubmac_data_ptr->sync_precision = ubmac_init_ptr->sync_precision;
				return TRUE;
			}
			return FALSE;
		}
		
		if(ubmac_data_ptr->next != NULL)
			ubmac_data_ptr = ubmac_data_ptr->next;
		else
			break;
	}
	
	// Case 3: Entry not found
	ubmac_data_ptr->next = (ubmac_data_t *)ker_malloc(sizeof(ubmac_data_t), s->pid);
	ubmac_data_ptr->next->node_id = ubmac_init_ptr->node_id;
	ubmac_data_ptr->next->wakeup_time = 0;
	ubmac_data_ptr->next->is_wakeup_time_valid = FALSE;	
	ubmac_data_ptr->next->period = 0;
	ubmac_data_ptr->next->ref_count = 1;
	ubmac_data_ptr->next->sync_precision = ubmac_init_ptr->sync_precision;
	ubmac_data_ptr->next->timesync_mod_id = ubmac_init_ptr->timesync_mod_id;
	ubmac_data_ptr->next->next = NULL;
	return TRUE;
}

static void update_info(app_state_t *s, uint16_t node_id, ubmac_transmit_t *ubmac_transmit_ptr)
{
	ubmac_data_t *ubmac_data_ptr;
	
	// Case 1: Empty list
	if(s->dest_info == NULL)
		return;
	
	// Case 2: Entry found
	ubmac_data_ptr = s->dest_info;
	while(ubmac_data_ptr)
	{
		if(ubmac_data_ptr->node_id == node_id)
		{
			ubmac_data_ptr->period = ubmac_transmit_ptr->period;
			switch(ubmac_data_ptr->timesync_mod_id)
			{
				case RATS_TIMESYNC_PID:
				{
					//In case of RATS, we only need to store the untranslated value
					ubmac_data_ptr->untranslated_wakeup_time = ubmac_transmit_ptr->wakeup_time;
					ubmac_data_ptr->is_wakeup_time_valid = TRUE;
					break;						
				}
				case TPSN_TIMESYNC_PID:
				{
					ubmac_data_ptr->untranslated_wakeup_time = ubmac_transmit_ptr->wakeup_time;
					
					//Ask TPSN to do the translation (find the offset)
					tpsn_t *tpsn_ptr = (tpsn_t *)ker_malloc(sizeof(tpsn_t), s->pid);
					tpsn_ptr->mod_id = s->pid;
					tpsn_ptr->msg_type = MSG_TIMESYNC_REPLY;
					tpsn_ptr->node_id = node_id;

					post_long(TPSN_TIMESYNC_PID, s->pid, MSG_GET_INSTANT_TIMESYNC, sizeof(tpsn_t), tpsn_ptr, 0);	
					break;
				}
				default:
				{
					break;
				}
			}
			return;
		}
		
		if(ubmac_data_ptr->next != NULL)
			ubmac_data_ptr = ubmac_data_ptr->next;
		else
			break;
	}
	
	// Case 3: Entry not found
	return;
}

static void update_translated_time(app_state_t *s, uint8_t timesync_mod_id, void *data)
{
	ubmac_data_t *ubmac_data_ptr;
	tpsn_t * tpsn_ptr = NULL;
	uint16_t node_id = 0;
	
	switch(timesync_mod_id)
	{
		case RATS_TIMESYNC_PID:
		{	
			//We should never come here
			return;
		}
		case TPSN_TIMESYNC_PID:
		{
			tpsn_ptr = (tpsn_t *)data;
			node_id = tpsn_ptr->node_id;
			break;
		}
		default:
			return;
	}
			
		
	// Case 1: Empty list
	if(s->dest_info == NULL)
		return;
	
	// Case 2: Entry found
	ubmac_data_ptr = s->dest_info;
	while(ubmac_data_ptr)
	{
		if(ubmac_data_ptr->node_id == node_id)
		{
			switch(ubmac_data_ptr->timesync_mod_id)
			{
				case RATS_TIMESYNC_PID:
				{	
					//We should never come here
					return;
				}
				case TPSN_TIMESYNC_PID:
				{
					//Sanity check
					if(tpsn_ptr == NULL)
						return;

					ubmac_data_ptr->wakeup_time = ubmac_data_ptr->untranslated_wakeup_time - tpsn_ptr->clock_drift;
					ubmac_data_ptr->is_wakeup_time_valid = TRUE;
					ker_free(tpsn_ptr);
					return;
				}
				default:
					break;
			}
		}
		
		if(ubmac_data_ptr->next != NULL)
			ubmac_data_ptr = ubmac_data_ptr->next;
		else
			break;
	}
	
	// Case 3: Entry not found
	return;	
}

static uint8_t remove_info(app_state_t *s, uint16_t node_id)
{
	ubmac_data_t *ubmac_data_ptr = s->dest_info;
	ubmac_data_t *ubmac_delete_ptr;
	ubmac_data_t *ubmac_previous_ptr = s->dest_info;
	
	/* Loop until we've reached the end of the list */
	while( ubmac_data_ptr != NULL )
	{
		if(ubmac_data_ptr->node_id == node_id)
		{
			if(--ubmac_data_ptr->ref_count > 0)
				return FALSE;
			
			DEBUG("UBMAC: Removing node %d from list\n", node_id);
			
			/* Found the item to be deleted,
     		 re-link the list around it */
			if( ubmac_data_ptr == s->dest_info )
				/* We're deleting the head */
				s->dest_info = ubmac_data_ptr->next;
			else
				ubmac_previous_ptr->next = ubmac_data_ptr->next;

			ubmac_delete_ptr = ubmac_data_ptr;
			ubmac_data_ptr = ubmac_data_ptr->next;
					
			/* Free the node */
			ker_free( ubmac_delete_ptr );
			
			return TRUE;
		}
		ubmac_previous_ptr = ubmac_data_ptr;
		ubmac_data_ptr = ubmac_data_ptr->next;
	}
			
	DEBUG("UBMAC: Node %d was not found in list\n", node_id);
			
	return FALSE;
}

static ubmac_data_t * get_info(app_state_t *s, uint16_t node_id)
{
	ubmac_data_t *ubmac_data_ptr = s->dest_info;
	while(ubmac_data_ptr)
	{
		if(ubmac_data_ptr->node_id == node_id)
			return ubmac_data_ptr;
		ubmac_data_ptr = ubmac_data_ptr->next;
	}
	
	return NULL;
}

uint16_t ubmacGetTime(uint16_t node_id)
{
	app_state_t *s = (app_state_t *)ker_get_module_state(UBMAC_PID);
	ubmac_data_t * ubmac_data_ptr = get_info(s, node_id);
	if(ubmac_data_ptr == NULL)
		return 0;
	
	if(ubmac_data_ptr->period == ALWAYS_ON)
		return TIMER_MIN_INTERVAL; //set timer with small timeout 
	
	if(ubmac_data_ptr->period == 0) //period not set
		return 0; 
		
	//Time is untranslated (ask for node's wakeup time, 
	//in case the initial request was lost)
	if(ubmac_data_ptr->is_wakeup_time_valid == FALSE)
	{
		post_net(s->pid, s->pid, MSG_REQUEST_INFO, 0, NULL, 0, node_id);
		return 0; 
	}
	
	if(ubmac_data_ptr->timesync_mod_id == RATS_TIMESYNC_PID) // RATS
	{	
		//Find current time of target node
		float current_time_of_other_node = convert_from_mine_to_parent_time(ker_systime32(), node_id);
		
		if(current_time_of_other_node == 0) //learning state
			return 0;
		
		float untranslated_wakeup_time = (float)ubmac_data_ptr->untranslated_wakeup_time;
	
		//Take care of overflow in target node's current time
		if( (untranslated_wakeup_time > 3*(INT_MAX_GTIME/4)) && (current_time_of_other_node < (INT_MAX_GTIME/4)) )
		{
			current_time_of_other_node += INT_MAX_GTIME;
		}
	
		//Find next wakeup time of target node (expressed in the target node's clock)
		while( (untranslated_wakeup_time - (CSMA_TIMEOUT + MINIMUM_TIMER_RETURN_TIME) < current_time_of_other_node ) )
			untranslated_wakeup_time += ubmac_data_ptr->period;
	
		//Convert target node's wakeup time to local time
		uint32_t transmission_time = convert_from_parent_to_my_time(untranslated_wakeup_time, node_id);
		
		if(transmission_time == 0) // learning state
			return 0;
		
		//Take care of overflow in wakeup time
		if(untranslated_wakeup_time > INT_MAX_GTIME)
			untranslated_wakeup_time -= INT_MAX_GTIME;
		
		ubmac_data_ptr->untranslated_wakeup_time = untranslated_wakeup_time;
	
		//Take care of overflow in transmission time
		if( (transmission_time < (INT_MAX_GTIME/4)) && (ker_systime32() > 3*(INT_MAX_GTIME/4)) )
		{
			transmission_time += INT_MAX_GTIME;
		}
		
		//If final result is valid, send with UBMAC, otherwise send with BMAC
		if((uint16_t)((transmission_time - ker_systime32() - CSMA_TIMEOUT)/TIMER_TO_CLOCK_CONVERSION) > TIMER_MIN_INTERVAL)
			return (uint16_t)((transmission_time - ker_systime32() - CSMA_TIMEOUT)/TIMER_TO_CLOCK_CONVERSION); 
		else
			return 0;
	}	
	else if(ubmac_data_ptr->timesync_mod_id == TPSN_TIMESYNC_PID) //TPSN
	{
		uint32_t wakeup_time = ubmac_data_ptr->wakeup_time;
		uint32_t current_time = ker_systime32();
		
		//Take care of overflow in current time
		if( (wakeup_time > 3*(INT_MAX_GTIME/4)) && (current_time < (INT_MAX_GTIME/4)) )
			current_time += INT_MAX_GTIME;
		
		while(wakeup_time - (CSMA_TIMEOUT + MINIMUM_TIMER_RETURN_TIME) < current_time )
			wakeup_time += ubmac_data_ptr->period;

		//Take care of overflow
		if(wakeup_time >= INT_MAX_GTIME)
			ubmac_data_ptr->wakeup_time = wakeup_time - INT_MAX_GTIME;
		else
			ubmac_data_ptr->wakeup_time = wakeup_time;
			
		return (uint16_t)((wakeup_time - current_time - CSMA_TIMEOUT)/TIMER_TO_CLOCK_CONVERSION); 
	}
	
	//Default return value: send with BMAC
	return 0;
			
}

uint32_t ubmacGetPreamble(uint16_t node_id)
{
	app_state_t *s = (app_state_t *)ker_get_module_state(UBMAC_PID);
	ubmac_data_t * ubmac_data_ptr = get_info(s, node_id);
	if(ubmac_data_ptr == NULL)
		return 0;
		
	//4 + (uint32_t)(1000*(2*node_precision_in_milisec)/416)		
	return (4+(uint32_t)(1000*((2*ubmac_data_ptr->sync_precision))/416)+LEVERAGE_PREAMBLE);
}

void notifyUbmac(uint32_t wakeup_time)
{
	app_state_t *s = (app_state_t *)ker_get_module_state(UBMAC_PID);
	s->my_info.period = wakeup_time - s->my_info.wakeup_time;
	s->my_info.wakeup_time = wakeup_time;
} 

#ifndef _MODULE_
mod_header_ptr ubmac_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif


