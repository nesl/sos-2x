/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <sys_module.h>
 */
#include <sys_module.h>
#define LED_DEBUG
#include <led_dbg.h>
#include <timesync/tpsn/tpsn.h>
#include <systime.h> // needed for msec_to_ticks
#include "tpsn_net.h"

// max time before we need a refresh
#define REFRESH_INTERVAL (msec_to_ticks(30*1024))

#define ADV_TIMER_ID 0

typedef enum {
    INIT,
    SYNCED
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
    sync_state_t sync_state;
	tpsn_t *tpsn_ptr;
} app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t tpsn_net_module_handler(void *state, Message *e);

/**
 * This is the only global variable one can have.
 * TODO: change mod_id to correct value.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id          = TPSN_NET_PID,
	.state_size      = sizeof(app_state_t),
	.num_sub_func    = 0,
	.num_prov_func   = 0,
    .platform_type   = HW_TYPE,
    .processor_type  = MCU_TYPE,
    .code_id         = ehtons(TPSN_NET_PID),
	.module_handler  = tpsn_net_module_handler,	
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
			s->tpsn_ptr = NULL;
            s->level = -1;
            s->parent_id = 0;
            s->sync_state = INIT;

            // try to join the sync tree
            msg_adv_level->level = s->level;
            sys_post_net(s->pid, MSG_ADV_LEVEL, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, BCAST_ADDRESS);
            sys_timer_start(ADV_TIMER_ID, 5*1024, TIMER_REPEAT);
			return SOS_OK;
		}

        case MSG_GET_GLOBAL_TIME:
        {
            msg_global_time_t* time_msg = (msg_global_time_t*)msg->data;
            msg_global_time_t* time_reply_msg = (msg_global_time_t*)sys_malloc(sizeof(msg_global_time_t));

            if(s->sync_state == SYNCED){
                // we are synced, and thus had at least one time sync exchange
                uint32_t delta_refresh = 0;
                uint32_t cur_time = sys_time32();
                // if we are level 1, then just return our time
                if(s->level == 1){
                    time_reply_msg->time = time_msg->time;
                    time_reply_msg->refreshed = 0;
                } else {
                    // check for overflow
                    if(cur_time < s->last_refresh){
                        delta_refresh = (0xFFFFFFFF - s->last_refresh) + cur_time;
                    } else {
                        delta_refresh = cur_time - s->last_refresh;
                    }
                    if (delta_refresh > REFRESH_INTERVAL){
                        DEBUG("TPSN_NET: Refresh needed refresh: %d\n", delta_refresh);
                        if(s->tpsn_ptr == NULL)
                            s->tpsn_ptr = (tpsn_t*) sys_malloc(sizeof(tpsn_t));

                        s->tpsn_ptr->mod_id = s->pid;
                        s->tpsn_ptr->msg_type = MSG_TPSN_REPLY;
                        s->tpsn_ptr->node_id = s->parent_id;
                        sys_post(TPSN_TIMESYNC_PID, MSG_GET_INSTANT_TIMESYNC, sizeof(tpsn_t), s->tpsn_ptr, 0);
                    }
                    time_reply_msg->time = time_msg->time + s->clock_drift;
                    time_reply_msg->refreshed = delta_refresh;
                }
            } else {
                time_reply_msg->time = NOT_SYNCED;
                time_reply_msg->refreshed = NOT_SYNCED;
            }
            DEBUG("TPSN_NET: converted time for module %d, drift %d, refreshed %d, global time %d\n", msg->sid, s->clock_drift, time_reply_msg->refreshed, time_reply_msg->time);
            sys_post(msg->sid, MSG_GLOBAL_TIME_REPLY, sizeof(msg_global_time_t), time_reply_msg, SOS_MSG_RELEASE);

            return SOS_OK;
        }

        case MSG_TPSN_REPLY:
        {
            if((s->tpsn_ptr->msg_type != msg->type) && (s->tpsn_ptr->clock_drift == 0)){
                // something went wrong
                DEBUG("TPSN_NET: received invalid reply\n");
            } else {
                DEBUG("TPSN_NET: clock offset with node %d is %d, synctime: %d\n", s->tpsn_ptr->node_id, s->tpsn_ptr->clock_drift, sys_time32() + s->tpsn_ptr->clock_drift);
            }
            s->clock_drift = s->tpsn_ptr->clock_drift;
            s->last_refresh = sys_time32();

            // the pointer needs both to be freed and set to NULL, since sys_free doesn't do the later
            sys_free(s->tpsn_ptr);
            s->tpsn_ptr = NULL;

            return SOS_OK;
        }

        case MSG_TIMER_TIMEOUT:
        {
            switch(p->byte)
            {
                case ADV_TIMER_ID:
                    {
                        msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)sys_malloc(sizeof(msg_adv_level_t));
                        if( s->sync_state == INIT){
                            // try to join the sync tree
                            msg_adv_level->level = s->level;
                            sys_post_net(s->pid, MSG_ADV_LEVEL, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, BCAST_ADDRESS);
                        }
                    }
                default:
                    break;
            }
            return SOS_OK;
        }

        case MSG_ADV_LEVEL:
        {
            if ( s->sync_state == SYNCED ){
                msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)sys_malloc(sizeof(msg_adv_level_t));
                msg_adv_level->level = s->level;
                sys_post_net(s->pid, MSG_ADV_REPLY, sizeof(msg_adv_level_t), msg_adv_level, SOS_MSG_RELEASE, msg->saddr);
        
            }
            return SOS_OK;
        }

        case MSG_ADV_REPLY:
        {
            msg_adv_level_t* msg_adv_level = (msg_adv_level_t*)msg->data;
            if ( (s->sync_state == INIT) || msg_adv_level->level < s->level) {
                DEBUG("TPSN_NET: received new level %d from %d\n", msg_adv_level->level, msg->saddr);
                sys_timer_stop(ADV_TIMER_ID);
                s->level = msg_adv_level->level+1;
                s->parent_id = msg->saddr;
                s->sync_state = SYNCED;
                if(s->tpsn_ptr == NULL)
                    s->tpsn_ptr = (tpsn_t*) sys_malloc(sizeof(tpsn_t));

                s->tpsn_ptr->mod_id = s->pid;
                s->tpsn_ptr->msg_type = MSG_TPSN_REPLY;
                s->tpsn_ptr->node_id = s->parent_id;
                sys_post(TPSN_TIMESYNC_PID, MSG_GET_INSTANT_TIMESYNC, sizeof(tpsn_t), s->tpsn_ptr, 0);
 
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

#ifndef _MODULE_
mod_header_ptr tpsn_net_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

