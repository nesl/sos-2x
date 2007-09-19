/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <sys_module.h>
 */
#include <sys_module.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include <systime.h> // needed for msec_to_ticks
#include <string.h>
#include "tpsn_net.h"

// max time before we need a refresh
#define REFRESH_INTERVAL (msec_to_ticks(30*1024))

enum 
{ 
    ADV_TIMER_ID,
    SYNC_TIMER_ID,
};

enum {TPSN_REQUEST, TPSN_REPLY};

typedef enum {
    INIT,
    SYNCED,
    SYNCING,
    INIT_SYNCING,
} sync_state_t;

/**
 * Module can define its own state
 */
typedef struct 
{
	sos_pid_t pid;
    int8_t level; // which level in the sync tree are we?
    uint16_t parent_id; // indicates our time sync parent
    uint32_t clock_drift;
    uint32_t last_refresh;
    uint8_t current_seq_no;
    sync_state_t sync_state;
} app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t tpsn_net_module_handler(void *state, Message *e);
static void start_sync();
static uint32_t get_global_time(func_cb_ptr p, uint32_t time);

/**
 * This is the only global variable one can have.
 * TODO: change mod_id to correct value.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id          = TPSN_NET_PID,
	.state_size      = sizeof(app_state_t),
	.num_sub_func    = 0,
	.num_prov_func   = 1,
    .platform_type   = HW_TYPE,
    .processor_type  = MCU_TYPE,
    .code_id         = ehtons(TPSN_NET_PID),
	.module_handler  = tpsn_net_module_handler,	
    .funct           = {
        {get_global_time, "IIz1", TPSN_NET_PID, 0}
    },
};


static int8_t tpsn_net_module_handler(void *state, Message *msg)
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

            msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)sys_malloc(sizeof(msg_adv_level_t));
			DEBUG("TPSN NET: Started\n");

			s->pid = msg->did;
            s->level = -1;
            s->parent_id = 0;
            s->last_refresh = 0;
            s->sync_state = INIT;
            s->current_seq_no = 0;

            // try to join the sync tree
            msg_adv_level->level = s->level;
            sys_post_net(s->pid, MSG_ADV_LEVEL, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, BCAST_ADDRESS);
            sys_timer_start(ADV_TIMER_ID, 5*1024, TIMER_REPEAT);
			return SOS_OK;
		}

        case MSG_GET_GLOBAL_TIME:
        {
            DEBUG("TPSN_NET: state: %d\n", s->sync_state);
            msg_global_time_t* time_msg = (msg_global_time_t*)msg->data;
            msg_global_time_t* time_reply_msg = (msg_global_time_t*)sys_malloc(sizeof(msg_global_time_t));

            if(s->sync_state == SYNCED || s->sync_state == SYNCING){
                // we are synced, and thus had at least one time sync exchange
                // if we are level 1, then just return our time
                if(s->level == 1){
                    time_reply_msg->time = time_msg->time;
                    time_reply_msg->refreshed = 0;
                } else {
                    uint32_t delta_refresh = 0;
                    uint32_t cur_time = sys_time32();
                    // check for overflow
                    if(cur_time < s->last_refresh){
                        cur_time += 0x7F000000;
                    }
                    delta_refresh = cur_time - s->last_refresh;
                    // only try to refresh if we are not already syncing.
                    if (s->sync_state == SYNCED && delta_refresh > REFRESH_INTERVAL){
                        DEBUG("TPSN_NET: Refresh needed refresh: %d\n", delta_refresh);
                        s->sync_state = SYNCING;

                        start_sync();
                   }
                    // even though we might be syncing, reply with the current estimate.
                    time_reply_msg->time = time_msg->time + s->clock_drift;
                    time_reply_msg->refreshed = delta_refresh;
                }
            } else {
                time_reply_msg->time = NOT_SYNCED;
                time_reply_msg->refreshed = NOT_SYNCED;
            }
            DEBUG("TPSN_NET: converted time for module %d, drift %d, refreshed %d, global time %d, sync state %d\n", msg->sid, s->clock_drift, time_reply_msg->refreshed, time_reply_msg->time, s->sync_state);
            sys_post(msg->sid, MSG_GLOBAL_TIME_REPLY, sizeof(msg_global_time_t), time_reply_msg, SOS_MSG_RELEASE);

            return SOS_OK;
        }

        case MSG_TIMESTAMP:
        {
            DEBUG("TPSN_NET: state: %d\n", s->sync_state);
            LED_DBG(LED_RED_TOGGLE);
            tpsn_req_t *tpsn_req_ptr = (tpsn_req_t *)msg->data;
            switch(tpsn_req_ptr->type){
                case TPSN_REQUEST:
                {
                    DEBUG("TPSN_NET: Received TPSN_REQUEST (seq_no=%d) from node %d\n", tpsn_req_ptr->seq_no, msg->saddr);
                    DEBUG("TPSN_NET: Transmitting TPSN_REPLY to node %d at time %d\n", msg->saddr, sys_time32());
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
                    if(s->sync_state == SYNCING || s->sync_state == INIT_SYNCING){
                        //LED_DBG(LED_YELLOW_TOGGLE);

                        DEBUG("TPSN: Received TPSN_REPLY from node %d\n", msg->saddr);
                        tpsn_reply_t *tpsn_reply_ptr = (tpsn_reply_t *)msg->data;
                        if (tpsn_reply_ptr->seq_no == s->current_seq_no){
                            sys_timer_stop(SYNC_TIMER_ID);
                            s->current_seq_no++;

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

                            s->clock_drift = ( ((int32_t)tpsn_reply_ptr->previous_time[1] - (int32_t)tpsn_reply_ptr->previous_time[0]) -
                                    ((int32_t)tpsn_reply_ptr->time[1] - (int32_t)tpsn_reply_ptr->time[0]) )/2;
                            s->last_refresh = sys_time32();
                            s->sync_state = SYNCED;
                            DEBUG("TPSN: The clock offset for node %d is %d\n", msg->saddr, s->clock_drift);
                        }
                    }
                    break;
                }
                default:
                {
                    DEBUG("Received unknown packet\n");
                    break;
                }
            }
        }

        case MSG_TIMER_TIMEOUT:
        {
            DEBUG("TPSN_NET: state: %d\n", s->sync_state);
            switch(p->byte)
            {
                case ADV_TIMER_ID:
                    {
                        if( s->sync_state == INIT){
                            // try to join the sync tree
                            DEBUG("TPSN_NET: Trying to join sync tree. Sending out ADV\n");
                            msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)sys_malloc(sizeof(msg_adv_level_t));
                            msg_adv_level->level = s->level;
                            sys_post_net(s->pid, MSG_ADV_LEVEL, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, BCAST_ADDRESS);
                        }
                        break;
                    }
                case SYNC_TIMER_ID:
                    {
                        if(s->sync_state == SYNCING || s->sync_state == INIT_SYNCING)
                        {
                            // didn't receive the timestamp in time. Try to resend
                            DEBUG("TPSN_NET: SYNC_TIMER fired. Trying to sync again.\n");
                            start_sync();
                        }
						break;
                    }
                default:
                    break;
            }
            return SOS_OK;
        }

        case MSG_ADV_LEVEL:
        {
            // FIXME: only allow level 1 to reply for now!
            if ( s->sync_state == SYNCED && s->level < 2){
                msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)sys_malloc(sizeof(msg_adv_level_t));
                msg_adv_level->level = s->level;
                sys_post_net(s->pid, MSG_ADV_REPLY, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, msg->saddr);
        
            }
            return SOS_OK;
        }

        case MSG_ADV_REPLY:
        {
            DEBUG("TPSN_NET: state: %d\n", s->sync_state);
            msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)msg->data;
            if ( (s->sync_state == INIT) || msg_adv_level->level < s->level) {
                DEBUG("TPSN_NET: received new level %d from %d\n", msg_adv_level->level, msg->saddr);
                sys_timer_stop(ADV_TIMER_ID);
                s->level = msg_adv_level->level+1;
                s->parent_id = msg->saddr;

                if(s->level == 1){
                    // special case for root.
                    msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)sys_malloc(sizeof(msg_adv_level_t));
                    msg_adv_level->level = s->level;
                    sys_post_net(s->pid, MSG_ADV_REPLY, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, msg->saddr);

                    s->sync_state = SYNCED;
                } else {
                    s->sync_state = INIT_SYNCING;
                    start_sync();
                }
           }
            return SOS_OK;
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

static void start_sync(){
    app_state_t* s = (app_state_t*)sys_get_state();

    LED_DBG(LED_YELLOW_TOGGLE);

    tpsn_req_t* tpsn_req_ptr = (tpsn_req_t *) sys_malloc(sizeof(tpsn_req_t));
    tpsn_req_ptr->type = TPSN_REQUEST;
    tpsn_req_ptr->seq_no = s->current_seq_no;
    DEBUG("TPSN_NET: Transmitting TIMESYNC packet to node %d with seq_no=%d\n", s->parent_id, tpsn_req_ptr->seq_no);
    sys_post_net(s->pid, MSG_TIMESTAMP, sizeof(tpsn_req_t), tpsn_req_ptr, SOS_MSG_RELEASE, s->parent_id);

    sys_timer_start(SYNC_TIMER_ID, 50, TIMER_ONE_SHOT);

}

static uint32_t get_global_time(func_cb_ptr p, uint32_t time){
    app_state_t* s = (app_state_t*)sys_get_state();

    if(s->sync_state == SYNCED || s->sync_state == SYNCING){
        // we are synced, and thus had at least one time sync exchange
        // if we are level 1, then just return our time
        if(s->level == 1){
            return time;
        } else {
            uint32_t delta_refresh = 0;
            uint32_t cur_time = sys_time32();
            // check for overflow
            if(cur_time < s->last_refresh){
                cur_time += 0x7F000000;
            }
            delta_refresh = cur_time - s->last_refresh;
            // only try to refresh if we are not already syncing.
            if (s->sync_state == SYNCED && delta_refresh > REFRESH_INTERVAL){
                DEBUG("TPSN_NET: Refresh needed refresh: %d\n", delta_refresh);
                s->sync_state = SYNCING;

                start_sync();
            }
            // even though we might be syncing, reply with the current estimate.
            return time + s->clock_drift;
        }
    } else {
        return NOT_SYNCED;
    }

}

#ifndef _MODULE_
mod_header_ptr tpsn_net_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

