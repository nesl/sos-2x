/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <sys_module.h>
 */
#include <sys_module.h>
#include <led.h>
#include <timesync/tpsn_net/tpsn_net.h>

#define TRANSMIT_TIMER 0

#define TRANSMIT_INTERVAL 1024

#define USERINT_FID 0

#define MSG_GLOBAL_TIME_SEND (MSG_GLOBAL_TIME_REPLY + 10)

typedef struct
{
    uint16_t addr;
    uint32_t time;
    uint32_t refreshed;
} msg_global_time_send_t;

/**
 * Module can define its own state
 */
typedef struct 
{
	sos_pid_t pid;
} app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t test_tpsn_net_module_handler(void *state, Message *e);
static void userint();

/**
 * This is the only global variable one can have.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = 
{
	.mod_id          = DFLT_APP_ID1,
	.state_size      = sizeof(app_state_t),
	.num_sub_func    = 0,
	.num_prov_func   = 1,
    .platform_type   = HW_TYPE,
    .processor_type  = MCU_TYPE,
    .code_id         = ehtons(DFLT_APP_ID1),
	.module_handler  = test_tpsn_net_module_handler,	
    .funct           = {
        { userint, "vvv0", DFLT_APP_ID1, USERINT_FID},
    },
};


static int8_t test_tpsn_net_module_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;
	//MsgParam *p = (MsgParam*)(msg->data);
	
	/**
	 * Switch to the correct message handler
	 */
	switch (msg->type)
	{
		case MSG_INIT:
		{
			s->pid = msg->did;
            sys_register_isr(0, USERINT_FID);
            sys_led(LED_RED_OFF);
            sys_led(LED_GREEN_OFF);
            sys_led(LED_YELLOW_OFF);
			return SOS_OK;
		}
    	case MSG_GLOBAL_TIME_REPLY:
		{
            msg_global_time_send_t* msg_global_time_send = (msg_global_time_send_t*)sys_malloc(sizeof(msg_global_time_send_t));
            msg_global_time_t* msg_global_time = (msg_global_time_t*)msg->data;

            sys_led(LED_GREEN_TOGGLE);
            msg_global_time_send->addr = sys_id();
            msg_global_time_send->time = msg_global_time->time;
            msg_global_time_send->refreshed = msg_global_time->refreshed;
            sys_post_net(s->pid, MSG_GLOBAL_TIME_SEND, sizeof(msg_global_time_send_t), msg_global_time_send, SOS_MSG_RELEASE, 0);
            //sys_post_uart(s->pid, MSG_GLOBAL_TIME_SEND, sizeof(msg_global_time_send_t), msg_global_time_send, SOS_MSG_RELEASE, BCAST_ADDRESS);
            break;
		}

        case MSG_GLOBAL_TIME_SEND:
        {
            msg_global_time_send_t* datamsg = (msg_global_time_send_t*) sys_msg_take_data(msg);
            //sys_led(LED_YELLOW_TOGGLE);
            sys_post_uart(s->pid, MSG_GLOBAL_TIME_SEND, sizeof(msg_global_time_send_t), datamsg, SOS_MSG_RELEASE, BCAST_ADDRESS);
            break;

        }
		
		case MSG_FINAL:
		{
			sys_timer_stop(TRANSMIT_TIMER);
            sys_deregister_isr(0);
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

static void userint(){
    //uint16_t i;
    uint32_t timestamped = sys_time32();
    /*
    for (i=0; i < sys_rand()%10000; i++) {
      asm volatile("nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   "nop\n\t"
                   ::);
    }
    */
    //sys_led(LED_YELLOW_TOGGLE);
    msg_global_time_t* msg_global_time = (msg_global_time_t*)sys_malloc(sizeof(msg_global_time_t));
    msg_global_time->time = timestamped;
    sys_post(TPSN_NET_PID, MSG_GET_GLOBAL_TIME, sizeof(msg_global_time_t), msg_global_time, SOS_MSG_RELEASE);
    //sys_post(DFLT_APP_ID1, MSG_GLOBAL_TIME_REPLY, sizeof(msg_global_time_t), msg_global_time, SOS_MSG_RELEASE);
    }

#ifndef _MODULE_
mod_header_ptr test_tpsn_net_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif

