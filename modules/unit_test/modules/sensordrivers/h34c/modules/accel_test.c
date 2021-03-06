/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>
#include <string.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <h34c.h>

#define ACCEL_TEST_APP_TID 0
#define ACCEL_TEST_APP_INTERVAL 20

#define ACCEL_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3

enum {
	ACCEL_TEST_APP_INIT=0,
	ACCEL_TEST_APP_IDLE,
	ACCEL_TEST_APP_ACCEL_0,
	ACCEL_TEST_APP_ACCEL_0_BUSY,
	ACCEL_TEST_APP_ACCEL_1,
	ACCEL_TEST_APP_ACCEL_1_BUSY,
	ACCEL_TEST_APP_ACCEL_2,
	ACCEL_TEST_APP_ACCEL_2_BUSY,
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
			sys_led(LED_RED_OFF);
			sys_led(LED_GREEN_OFF);
			sys_led(LED_YELLOW_OFF);

			s->state = ACCEL_TEST_APP_INIT;
			s->pid = msg->did;
			sys_timer_start(ACCEL_TEST_APP_TID, ACCEL_TEST_APP_INTERVAL, SLOW_TIMER_REPEAT);
			if(sys_sensor_enable(H34C_ACCEL_0_SID) != SOS_OK) {
				sys_led(LED_RED_ON);
				sys_timer_stop( ACCEL_TEST_APP_TID);
			}
			break;

		case MSG_FINAL:
			sys_sensor_disable(H34C_ACCEL_0_SID);
			sys_timer_stop( ACCEL_TEST_APP_TID);
			break;

		case MSG_TIMER_TIMEOUT:
			{
				//sys_led(LED_YELLOW_TOGGLE);
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
						sys_sensor_get_data(H34C_ACCEL_0_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_0_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_1;
						break;
						
					case ACCEL_TEST_APP_ACCEL_1:
						s->state = ACCEL_TEST_APP_ACCEL_1_BUSY;
						sys_sensor_get_data(H34C_ACCEL_1_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_1_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_0;
						break;

					case ACCEL_TEST_APP_ACCEL_2:
						s->state = ACCEL_TEST_APP_ACCEL_2_BUSY;
						sys_sensor_get_data(H34C_ACCEL_2_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_2_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_0;
						break;

					default:
						//sys_led(LED_RED_TOGGLE);
						s->state = ACCEL_TEST_APP_INIT;
						break;
				}
			}
			break;

		case MSG_DATA_READY:
			{
				uint8_t *data_msg;


				data_msg = sys_malloc ( UART_MSG_LEN );
				if ( data_msg ) {
				  sys_led(LED_GREEN_TOGGLE);
					memcpy((void*)data_msg, (void*)msg->data, UART_MSG_LEN);

					if (sys_id() == 0){
						sys_post_uart ( 
								s->pid,
								MSG_DATA_READY,
								UART_MSG_LEN,
								data_msg,
								SOS_MSG_RELEASE,
								BCAST_ADDRESS);
					} else {
					  sys_post_net(
								s->pid,
								MSG_DATA_READY,
								UART_MSG_LEN,
								data_msg,
								SOS_MSG_RELEASE,
								BCAST_ADDRESS);
					}
				}
				switch(s->state) {
					case ACCEL_TEST_APP_ACCEL_0_BUSY:
						s->state = ACCEL_TEST_APP_ACCEL_1;
						break;
					case ACCEL_TEST_APP_ACCEL_1_BUSY:
						s->state = ACCEL_TEST_APP_ACCEL_2;
						break;
					case ACCEL_TEST_APP_ACCEL_2_BUSY:
						s->state = ACCEL_TEST_APP_ACCEL_0;
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
mod_header_ptr accel_test_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

