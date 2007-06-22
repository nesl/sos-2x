/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <module.h>
#include <sys_module.h>
#include <string.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <tmote_sensors.h>

#define TSR_TEST_APP_TID 0
#define TSR_TEST_APP_INTERVAL 512

#define TSR_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3

enum {
	TSR_TEST_APP_INIT=0,
	TSR_TEST_APP_IDLE,
	TSR_TEST_APP_TSR,
	TSR_TEST_APP_TSR_BUSY,
};

typedef struct {
	uint8_t pid;
	uint8_t state;
} app_state_t;


static int8_t tsr_test_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = TSR_TEST_PID,
	.state_size     = sizeof(app_state_t),
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type = HW_TYPE,
	.processor_type = MCU_TYPE,
	.code_id = ehtons(TSR_TEST_PID),
	.module_handler = tsr_test_msg_handler,
};


static int8_t tsr_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

		case MSG_INIT:
			s->state = TSR_TEST_APP_INIT;
			s->pid = msg->did;
			sys_timer_start(TSR_TEST_APP_TID, TSR_TEST_APP_INTERVAL, TIMER_REPEAT);
			if(ker_sensor_enable(s->pid, TSR_SID) != SOS_OK) {
				SYS_LED_DBG(LED_RED_ON);
				sys_timer_stop(TSR_TEST_APP_TID);
			}
			break;

		case MSG_FINAL:
			ker_sensor_disable(s->pid, TSR_SID);
			sys_timer_stop(TSR_TEST_APP_TID);
			break;

		case MSG_TIMER_TIMEOUT:
			{
				SYS_LED_DBG(LED_YELLOW_TOGGLE);
				switch (s->state) {
					case TSR_TEST_APP_INIT:
						// do any necessary init here
						s->state = TSR_TEST_APP_IDLE;
						break;

					case TSR_TEST_APP_IDLE:
						s->state = TSR_TEST_APP_TSR;
						break;

					case TSR_TEST_APP_TSR:
						s->state = TSR_TEST_APP_TSR_BUSY;
						ker_sensor_get_data(s->pid, TSR_SID);
						break;

					case TSR_TEST_APP_TSR_BUSY:
						break;
						
					default:
						SYS_LED_DBG(LED_RED_TOGGLE);
						s->state = TSR_TEST_APP_INIT;
						break;
				}
			}
			break;

		case MSG_DATA_READY:
			{
				uint8_t *data_msg;

				SYS_LED_DBG(LED_GREEN_TOGGLE);

				data_msg = sys_malloc ( sizeof(MsgParam));
				if ( data_msg ) {
					memcpy((void*)data_msg, (void*)msg->data, sizeof(MsgParam));

					post_uart ( s->pid,
							s->pid,
							MSG_DATA_READY,
						  sizeof(MsgParam),
							data_msg,
							SOS_MSG_RELEASE,
						  BCAST_ADDRESS);
				}
				switch(s->state) {
					case TSR_TEST_APP_TSR_BUSY:
						s->state = TSR_TEST_APP_TSR;
						break;
				}
			}
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr tsr_test_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

