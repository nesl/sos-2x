/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <module.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <mts310sb.h>

#define PHOTOTEMP_TEST_APP_TID 0
#define PHOTOTEMP_TEST_APP_INTERVAL 1024

#define PHOTOTEMP_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3

enum {
	PHOTOTEMP_TEST_APP_INIT=0,
	PHOTOTEMP_TEST_APP_IDLE,
	PHOTOTEMP_TEST_APP_PHOTO,
	PHOTOTEMP_TEST_APP_PHOTO_BUSY,
	PHOTOTEMP_TEST_APP_TEMP,
	PHOTOTEMP_TEST_APP_TEMP_BUSY,
};

typedef struct {
	uint8_t pid;
	uint8_t state;
} app_state_t;


static int8_t phototemp_test_msg_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = PHOTOTEMP_TEST_PID,
	.state_size     = sizeof(app_state_t),
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type = HW_TYPE,
	.processor_type = MCU_TYPE,
	.code_id = ehtons(PHOTOTEMP_TEST_PID),
	.module_handler = phototemp_test_msg_handler,
};


static int8_t phototemp_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

		case MSG_INIT:
			s->state = PHOTOTEMP_TEST_APP_INIT;
			s->pid = msg->did;
			ker_timer_init(s->pid, PHOTOTEMP_TEST_APP_TID, TIMER_REPEAT);
			ker_timer_start(s->pid, PHOTOTEMP_TEST_APP_TID, PHOTOTEMP_TEST_APP_INTERVAL);
			break;

		case MSG_FINAL:
			ker_sensor_disable(s->pid, MTS310_PHOTO_SID);
			ker_sensor_disable(s->pid, MTS310_TEMP_SID);
			break;

		case MSG_TIMER_TIMEOUT:
			{
				LED_DBG(LED_YELLOW_TOGGLE);
				switch (s->state) {
					case PHOTOTEMP_TEST_APP_INIT:
						// do any necessary init here
						s->state = PHOTOTEMP_TEST_APP_IDLE;
						break;

					case PHOTOTEMP_TEST_APP_IDLE:
						s->state = PHOTOTEMP_TEST_APP_TEMP;
						break;

					case PHOTOTEMP_TEST_APP_PHOTO:
						LED_DBG(LED_GREEN_TOGGLE);
						ker_sensor_enable(s->pid, MTS310_PHOTO_SID);
						s->state = PHOTOTEMP_TEST_APP_PHOTO_BUSY;
						ker_sensor_get_data(s->pid, MTS310_PHOTO_SID);
						break;

					case PHOTOTEMP_TEST_APP_PHOTO_BUSY:
						s->state = PHOTOTEMP_TEST_APP_TEMP;
						break;
						
					case PHOTOTEMP_TEST_APP_TEMP:
						ker_sensor_enable(s->pid, MTS310_TEMP_SID);
						s->state = PHOTOTEMP_TEST_APP_TEMP_BUSY;
						ker_sensor_get_data(s->pid, MTS310_TEMP_SID);
						break;

					case PHOTOTEMP_TEST_APP_TEMP_BUSY:
						s->state = PHOTOTEMP_TEST_APP_PHOTO;
						break;

					default:
						LED_DBG(LED_RED_TOGGLE);
						s->state = PHOTOTEMP_TEST_APP_INIT;
						break;
				}
			}
			break;

		case MSG_DATA_READY:
			{
				uint8_t *data_msg;

				LED_DBG(LED_GREEN_TOGGLE);
				data_msg =  ker_malloc ( UART_MSG_LEN, s->pid );
				if ( data_msg ) {
				
					data_msg[0] = msg->data[0];
					data_msg[1] = msg->data[2];
					data_msg[2] = msg->data[1];

					post_uart ( s->pid,
							s->pid,
							MSG_DATA_READY,
							UART_MSG_LEN,
							data_msg,
							SOS_MSG_RELEASE,
							UART_ADDRESS);
				}
			
				if (s->state == PHOTOTEMP_TEST_APP_TEMP_BUSY) {
					ker_sensor_disable(s->pid, MTS310_PHOTO_SID);
					s->state = PHOTOTEMP_TEST_APP_PHOTO;
				} else {
					ker_sensor_disable(s->pid, MTS310_PHOTO_SID);
					s->state = PHOTOTEMP_TEST_APP_TEMP;
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
mod_header_ptr phototemp_test_app_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

