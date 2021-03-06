/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <mts310sb.h>

#define ACCEL_TEST_APP_TID 0
#define ACCEL_TEST_APP_INTERVAL 1024

#define ACCEL_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3

enum {
	ACCEL_TEST_APP_INIT=0,
	ACCEL_TEST_APP_IDLE,
	ACCEL_TEST_APP_ACCEL_0,
	ACCEL_TEST_APP_ACCEL_0_BUSY,
	ACCEL_TEST_APP_ACCEL_1,
	ACCEL_TEST_APP_ACCEL_1_BUSY,
};

typedef struct {
	uint8_t pid;
	uint8_t state;
} app_state_t;


static int8_t accel_test_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = ACCEL_TEST_PID,
	.state_size     = sizeof(app_state_t),
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type = HW_TYPE,
	.processor_type = MCU_TYPE,
	.code_id = ehtons(ACCEL_TEST_PID),
	.module_handler = accel_test_msg_handler,
};


static int8_t accel_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

		case MSG_INIT:
			s->state = ACCEL_TEST_APP_INIT;
			s->pid = msg->did;
			sys_timer_start(ACCEL_TEST_APP_TID, ACCEL_TEST_APP_INTERVAL, TIMER_REPEAT);
			sys_sensor_enable(MTS310_ACCEL_0_SID);
			break;

		case MSG_FINAL:
			sys_sensor_disable(MTS310_ACCEL_0_SID);
			break;

		case MSG_TIMER_TIMEOUT:
			{
				LED_DBG(LED_YELLOW_TOGGLE);
				switch (s->state) {
					case ACCEL_TEST_APP_INIT:
						// do any necessary init here
						s->state = ACCEL_TEST_APP_IDLE;
						break;

					case ACCEL_TEST_APP_IDLE:
						s->state = ACCEL_TEST_APP_ACCEL_0;
						break;

					case ACCEL_TEST_APP_ACCEL_0:
						s->state = ACCEL_TEST_APP_ACCEL_0_BUSY;
						sys_sensor_get_data(MTS310_ACCEL_0_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_0_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_1;
						break;
						
					case ACCEL_TEST_APP_ACCEL_1:
						s->state = ACCEL_TEST_APP_ACCEL_1_BUSY;
						sys_sensor_get_data(MTS310_ACCEL_1_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_1_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_0;
						break;

					default:
						LED_DBG(LED_RED_TOGGLE);
						s->state = ACCEL_TEST_APP_INIT;
						break;
				}
			}
			break;

		case MSG_DATA_READY:
			{
				uint8_t *data_msg;

				LED_DBG(LED_GREEN_TOGGLE);

				data_msg = sys_malloc ( UART_MSG_LEN );
				if ( data_msg ) {
					data_msg[0] = msg->data[0];
					data_msg[1] = msg->data[2];
					data_msg[2] = msg->data[1];

					sys_post_uart ( s->pid,
							MSG_DATA_READY,
							UART_MSG_LEN,
							data_msg,
							SOS_MSG_RELEASE,
							UART_ADDRESS);
				}
				if (s->state == ACCEL_TEST_APP_ACCEL_1_BUSY) {
					s->state = ACCEL_TEST_APP_ACCEL_0;
				} else {
					s->state = ACCEL_TEST_APP_ACCEL_1;
				}
			}
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}

mod_header_ptr accel_test_app_get_header() {
	return sos_get_header_address(mod_header);
}


