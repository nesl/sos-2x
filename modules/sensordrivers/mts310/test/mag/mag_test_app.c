/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <module.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <mts310sb.h>

#define MAG_TEST_APP_TID 0
#define MAG_TEST_APP_INTERVAL 4096

#define MAG_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3

enum {
	MAG_TEST_APP_INIT_0=0,
	MAG_TEST_APP_INIT_1,
	MAG_TEST_APP_IDLE_0,
	MAG_TEST_APP_IDLE_1,
	MAG_TEST_APP_MAG_0,
	MAG_TEST_APP_MAG_0_BUSY,
	MAG_TEST_APP_MAG_1,
	MAG_TEST_APP_MAG_1_BUSY,
};

typedef struct {
	uint8_t pid;
	uint8_t state;
} app_state_t;


static int8_t mag_test_msg_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = MAG_TEST_PID,
	.state_size     = sizeof(app_state_t),
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type = HW_TYPE,
	.processor_type = MCU_TYPE,
	.code_id = ehtons(MAG_TEST_PID),
	.module_handler = mag_test_msg_handler,
};


static int8_t mag_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

	case MSG_INIT:
		s->state = MAG_TEST_APP_INIT_0;
		s->pid = msg->did;
		ker_timer_init(s->pid, MAG_TEST_APP_TID, TIMER_REPEAT);
		ker_timer_start(s->pid, MAG_TEST_APP_TID, MAG_TEST_APP_INTERVAL);
		ker_sensor_enable(s->pid, MTS310_MAG_0_SID);
		ker_sensor_enable(s->pid, MTS310_MAG_1_SID);
		break;

	case MSG_FINAL:
		ker_sensor_disable(s->pid, MTS310_MAG_0_SID);
		ker_sensor_disable(s->pid, MTS310_MAG_1_SID);
		break;

	case MSG_TIMER_TIMEOUT:
		{
			LED_DBG(LED_YELLOW_TOGGLE);

			uint8_t *temp;

			switch (s->state) {
			case MAG_TEST_APP_INIT_0:
				// callibrate magnetometers
				temp = ker_malloc(sizeof(uint8_t), MAG_TEST_PID);
				*temp = 0;
				ker_sensor_control(s->pid, MTS310_MAG_0_SID, temp);	
				s->state = MAG_TEST_APP_IDLE_0;
				break;
			case MAG_TEST_APP_IDLE_0:
				s->state = MAG_TEST_APP_INIT_1;
				break;
			case MAG_TEST_APP_INIT_1:
				temp = ker_malloc(sizeof(uint8_t), MAG_TEST_PID);
				*temp = 1;
				ker_sensor_control(s->pid, MTS310_MAG_1_SID, temp);	
				s->state = MAG_TEST_APP_IDLE_1;
				break;
			case MAG_TEST_APP_IDLE_1:
				s->state = MAG_TEST_APP_MAG_0;
				break;
			case MAG_TEST_APP_MAG_0:
				s->state = MAG_TEST_APP_MAG_0_BUSY;
				ker_sensor_get_data(s->pid, MTS310_MAG_0_SID);
				break;
			case MAG_TEST_APP_MAG_0_BUSY:
				s->state = MAG_TEST_APP_MAG_1;
				break;
			case MAG_TEST_APP_MAG_1:
				s->state = MAG_TEST_APP_MAG_1_BUSY;
				ker_sensor_get_data(s->pid, MTS310_MAG_1_SID);
				break;
			case MAG_TEST_APP_MAG_1_BUSY:
				s->state = MAG_TEST_APP_MAG_0;
				break;
			default:
				//LED_DBG(LED_RED_TOGGLE);
				s->state = MAG_TEST_APP_INIT_0;
				break;
			}
		}
		break;

	case MSG_DATA_READY:
		{
			uint8_t *data_msg;

			//LED_DBG(LED_GREEN_TOGGLE);

			data_msg = ker_malloc ( UART_MSG_LEN, s->pid );
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
			if (s->state == MAG_TEST_APP_MAG_1_BUSY) {
				s->state = MAG_TEST_APP_MAG_0;
			} else {
				s->state = MAG_TEST_APP_MAG_1;
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
mod_header_ptr mag_test_app_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

