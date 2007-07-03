/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>
#include <string.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <tmote_sensors.h>

#define TEMP_TEST_APP_TID 0
#define TEMP_TEST_APP_INTERVAL 512

#define TEMP_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3

enum {
	TEMP_TEST_APP_INIT=0,
	TEMP_TEST_APP_IDLE,
	TEMP_TEST_APP_TEMP,
	TEMP_TEST_APP_TEMP_BUSY,
};

typedef struct {
	uint8_t pid;
	uint8_t state;
} app_state_t;


static int8_t temp_test_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = TEMP_TEST_PID,
	.state_size     = sizeof(app_state_t),
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type = HW_TYPE,
	.processor_type = MCU_TYPE,
	.code_id = ehtons(TEMP_TEST_PID),
	.module_handler = temp_test_msg_handler,
};


static int8_t temp_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

		case MSG_INIT:
			s->state = TEMP_TEST_APP_INIT;
			s->pid = msg->did;
			sys_timer_start(TEMP_TEST_APP_TID, TEMP_TEST_APP_INTERVAL, TIMER_REPEAT);
			if(sys_sensor_enable(TEMP_SID) != SOS_OK) {
				LED_DBG(LED_RED_ON);
				sys_timer_stop(TEMP_TEST_APP_TID);
			}
			break;

		case MSG_FINAL:
			sys_sensor_disable( TEMP_SID);
			sys_timer_stop(TEMP_TEST_APP_TID);
			break;

		case MSG_TIMER_TIMEOUT:
			{
				LED_DBG(LED_YELLOW_TOGGLE);
				switch (s->state) {
					case TEMP_TEST_APP_INIT:
						// do any necessary init here
						s->state = TEMP_TEST_APP_IDLE;
						break;

					case TEMP_TEST_APP_IDLE:
						s->state = TEMP_TEST_APP_TEMP;
						break;

					case TEMP_TEST_APP_TEMP:
						s->state = TEMP_TEST_APP_TEMP_BUSY;
						sys_sensor_get_data( TEMP_SID);
						break;

					case TEMP_TEST_APP_TEMP_BUSY:
						break;
						
					default:
						LED_DBG(LED_RED_TOGGLE);
						s->state = TEMP_TEST_APP_INIT;
						break;
				}
			}
			break;

		case MSG_DATA_READY:
			{
				uint8_t *data_msg;

				LED_DBG(LED_GREEN_TOGGLE);

				data_msg = sys_malloc ( sizeof(MsgParam));
				if ( data_msg ) {
          LED_DBG(LED_RED_TOGGLE);
					memcpy((void*)data_msg, (void*)msg->data, sizeof(MsgParam));

					sys_post_uart ( s->pid,
							MSG_DATA_READY,
							sizeof(MsgParam),
							data_msg,
							SOS_MSG_RELEASE,
						  BCAST_ADDRESS);
				}
				switch(s->state) {
					case TEMP_TEST_APP_TEMP_BUSY:
						s->state = TEMP_TEST_APP_TEMP;
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
mod_header_ptr temp_test_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

