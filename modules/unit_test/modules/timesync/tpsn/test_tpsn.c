/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <sys_module.h>
 */
#include <sys_module.h>
#define LED_DEBUG
#include <led_dbg.h>
#include <timesync/tpsn/tpsn.h>

#define TRANSMIT_TIMER 0

#define TRANSMIT_INTERVAL 1024

#define MSG_TPSN_REPLY (MOD_MSG_START + 0)

#define ROOT_NODE_ID 0
#define TARGET_NODE_ID 1

/**
 * Module can define its own state
 */
typedef struct 
{
	sos_pid_t pid;
	tpsn_t *tpsn_ptr;
} app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t module(void *state, Message *e);

/**
 * This is the only global variable one can have.
 * TODO: change mod_id to correct value.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id          = DFLT_APP_ID0,
	.state_size      = sizeof(app_state_t),
	.num_sub_func    = 0,
	.num_prov_func   = 0,
    .platform_type   = HW_TYPE,
    .processor_type  = MCU_TYPE,
    .code_id         = ehtons(DFLT_APP_ID0),
	.module_handler  = module,	
};


static int8_t module(void *state, Message *msg)
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
			DEBUG("TPSN tester: Started\n");
			s->pid = msg->did;
			s->tpsn_ptr = NULL;
			if(ROOT_NODE_ID == sys_id()) // transmitter
			{
				DEBUG("TPSN tester: Transmitter starting\n");
				sys_timer_start(TRANSMIT_TIMER, TRANSMIT_INTERVAL, TIMER_REPEAT);
			}
			else
			{
				DEBUG("TPSN tester: Receiver starting\n");
			}
			return SOS_OK;
		}
		case MSG_TIMER_TIMEOUT:
		{
			switch(p->byte)
			{
				case TRANSMIT_TIMER:
				{
					DEBUG("TPSN tester: Sending packet to TPSN module\n");
                    LED_DBG(LED_RED_TOGGLE);
					//If the the pointer that was previously passed to TPSN is now NULL, then allocate
					//memory. Otherwise use the same buffer
					if(s->tpsn_ptr == NULL)
						s->tpsn_ptr = (tpsn_t *)sys_malloc(sizeof(tpsn_t));
						
					s->tpsn_ptr->mod_id = s->pid;
					s->tpsn_ptr->msg_type = MSG_TPSN_REPLY;
					s->tpsn_ptr->node_id = TARGET_NODE_ID;
					sys_post(TPSN_TIMESYNC_PID, MSG_GET_INSTANT_TIMESYNC, sizeof(tpsn_t), s->tpsn_ptr, 0);
					break;
				}
				default:
					break;
			}
			break;
		}
		case MSG_TPSN_REPLY:
		{
            LED_DBG(LED_GREEN_TOGGLE);
			if((s->tpsn_ptr->msg_type != msg->type) && (s->tpsn_ptr->clock_drift == 0))
			{
				DEBUG("TPSN tester: received invalid reply\n");
			}
			else
			{
				DEBUG("TPSN tester: Clock offset with node %d is %d, synctime: %d\n", s->tpsn_ptr->node_id, s->tpsn_ptr->clock_drift, ker_systime32()+s->tpsn_ptr->clock_drift);
			}
			sys_post_uart(s->pid, MSG_TPSN_REPLY, sizeof(s->tpsn_ptr->clock_drift), &s->tpsn_ptr->clock_drift, 0, BCAST_ADDRESS);
			
			//The pointer needs both to be freed and to be set to NULL, since sys_free doesn't do the latter
			sys_free(s->tpsn_ptr);
			s->tpsn_ptr = NULL;
			break;
		}
		
		case MSG_FINAL:
		{
			sys_timer_stop(TRANSMIT_TIMER);
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
mod_header_ptr test_tpsn_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

