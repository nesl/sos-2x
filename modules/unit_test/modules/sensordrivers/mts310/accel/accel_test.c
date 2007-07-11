/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>
#include <string.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <mts310sb.h>

#define ACCEL_TEST_APP_TID 0
#define ACCEL_TEST_APP_INTERVAL 10
#define ACCEL_TEST_APP_INTERVAL_2 20

#define ACCEL_TEST_PID DFLT_APP_ID0

#define UART_MSG_LEN 3
#define CHANGE_FREQ 10000
#define CHANGE_FREQ_2 20000

#define MSG_ACCEL_DATA (MOD_MSG_START + 1)

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
	uint16_t counter;
} app_state_t;

typedef struct {
	uint8_t id;
	uint8_t change;
	uint16_t counter;
	uint8_t data[UART_MSG_LEN];
} data_msg_t;

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

static int8_t accel_change_freq(int8_t new_freq){
  sys_sensor_disable(MTS310_ACCEL_0_SID);
  sys_timer_stop( ACCEL_TEST_APP_TID );

	sys_timer_start(ACCEL_TEST_APP_TID, new_freq, SLOW_TIMER_REPEAT);
	if(sys_sensor_enable(MTS310_ACCEL_0_SID) != SOS_OK) {
		sys_led(LED_RED_ON);
		sys_timer_stop(ACCEL_TEST_APP_TID);
		return -EINVAL;
	}

	return SOS_OK;
}

static int8_t accel_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

		case MSG_INIT:
			sys_led(LED_GREEN_OFF);
			sys_led(LED_YELLOW_OFF);
			sys_led(LED_RED_OFF);

			s->state = ACCEL_TEST_APP_INIT;
			s->pid = msg->did;
			s->counter = 0;
			sys_timer_start(ACCEL_TEST_APP_TID, ACCEL_TEST_APP_INTERVAL, SLOW_TIMER_REPEAT);
			if(sys_sensor_enable(MTS310_ACCEL_0_SID) != SOS_OK) {
				sys_led(LED_RED_ON);
				sys_timer_stop(ACCEL_TEST_APP_TID);
			}
			break;

		case MSG_FINAL:
			sys_sensor_disable(MTS310_ACCEL_0_SID);
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
						sys_sensor_get_data(MTS310_ACCEL_0_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_0_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_1;
						break;
						
					case ACCEL_TEST_APP_ACCEL_1:
						s->state = ACCEL_TEST_APP_ACCEL_1_BUSY;
						sys_sensor_get_data(MTS310_ACCEL_0_SID);
						break;

					case ACCEL_TEST_APP_ACCEL_1_BUSY:
						//s->state = ACCEL_TEST_APP_ACCEL_0;
						break;

					default:
						//sys_led(LED_GREEN_TOGGLE);
						s->state = ACCEL_TEST_APP_INIT;
						break;
				}
			}
			break;

		case MSG_ACCEL_DATA:
			{
				uint8_t *payload;
				uint8_t msg_len;

				msg_len = msg->len;
				payload = sys_msg_take_data(msg);

				sys_post_uart (
						s->pid,
						MSG_ACCEL_DATA,
						msg_len,
						payload,
						SOS_MSG_RELEASE,
						BCAST_ADDRESS);
			}
			break;

		case MSG_DATA_READY:
			{
				data_msg_t *data_msg;

				data_msg = (data_msg_t*) sys_malloc (sizeof(data_msg_t));
				if ( data_msg ) {
				  sys_led(LED_GREEN_TOGGLE);

					data_msg->id = sys_id();

					if (s->counter == CHANGE_FREQ)
						data_msg->change = 2;
					else if (s->counter == CHANGE_FREQ_2)
						data_msg->change = 1;
					else
						data_msg->change = 0;

					data_msg->counter = s->counter;
					s->counter++;

					memcpy((void *)data_msg->data, (void*)msg->data, UART_MSG_LEN);

					if (sys_id() == 0){
						sys_post_uart ( 
								s->pid,
								MSG_ACCEL_DATA,
								sizeof(data_msg_t),
								data_msg,
								SOS_MSG_RELEASE,
								BCAST_ADDRESS);
					} else {
						sys_led(LED_YELLOW_TOGGLE);

						sys_post_net (
								s->pid, 
								MSG_ACCEL_DATA,
								sizeof(data_msg_t),
								data_msg,
								SOS_MSG_RELEASE,
								0);
					}
				} else
					sys_led(LED_RED_ON);
				switch(s->state) {
					case ACCEL_TEST_APP_ACCEL_0_BUSY:
						s->state = ACCEL_TEST_APP_ACCEL_1;
						break;
					case ACCEL_TEST_APP_ACCEL_1_BUSY:
						s->state = ACCEL_TEST_APP_ACCEL_0;
						break;
				}

				if (s->counter == CHANGE_FREQ){
					if (accel_change_freq(ACCEL_TEST_APP_INTERVAL_2) != SOS_OK)
            sys_led(LED_RED_ON);	
        } else if (s->counter == CHANGE_FREQ_2){
					if (accel_change_freq(ACCEL_TEST_APP_INTERVAL) != SOS_OK)
						sys_led(LED_RED_ON);
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

