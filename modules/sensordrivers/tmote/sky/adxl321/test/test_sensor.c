/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>

// Sensor board specific header file for 
// configuration options.
// The path to this file should be given in
// the Makefile.
#include <adxl321_sensor.h>

#define LED_DEBUG
#include <led_dbg.h>

#define TEST_SENSOR_PID			DFLT_APP_ID0
#define USER_INT_FID			0

typedef struct {
	sample_context_t ctx;
	filter_type_t filter;
	uint8_t user_pin;
} test_sensor_state_t;

static int8_t test_sensor_msg_handler(void *state, Message *msg);
static void user_isr();

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	mod_id : TEST_SENSOR_PID,
	code_id : ehtons(TEST_SENSOR_PID),
  	state_size : sizeof(test_sensor_state_t),
	num_sub_func : 0,
	num_prov_func : 1,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	module_handler : test_sensor_msg_handler,
	funct : { 
		{user_isr, "vvv0", TEST_SENSOR_PID, USER_INT_FID},
	},
};

int8_t test_sensor_msg_handler(void *state, Message *msg) {
	test_sensor_state_t *s = (test_sensor_state_t *)state;

	switch (msg->type) {
		case MSG_INIT: {
			LED_DBG(LED_RED_OFF);
			LED_DBG(LED_GREEN_OFF);
			LED_DBG(LED_YELLOW_OFF);
			s->user_pin = 0;
      		s->ctx.delay = 0;
			s->ctx.period = 64; // this is in ticks from a 32kHz clock
			s->ctx.samples = 0;
			s->ctx.event_samples = 32;
			s->filter = FILTER_NONE;
			// Module starts sampling when the user button is pressed
			if (sys_register_isr(PORT2_INTERRUPT, USER_INT_FID) == 0) {
				LED_DBG(LED_RED_TOGGLE);
			}
			break;
		}
		case MSG_DATA_READY: {
			size_t len = msg->len;
			sensor_data_msg_t *b = (sensor_data_msg_t *)sys_msg_take_data(msg);
			sys_post_uart(TEST_SENSOR_PID, 0x81, len, b, SOS_MSG_RELEASE, BCAST_ADDRESS);
			LED_DBG(LED_YELLOW_TOGGLE);
			break;
		}
		case MSG_FINAL: {
			sys_sensor_stop_sampling(ACCEL_X_SENSOR);
			sys_sensor_stop_sampling(ACCEL_Y_SENSOR);
			sys_deregister_isr(PORT2_INTERRUPT);
			break;
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}

static void user_isr() {
	test_sensor_state_t *s = (test_sensor_state_t*)sys_get_state();
	sensor_id_t sensor[1] = {ACCEL_X_SENSOR};

	LED_DBG(LED_RED_TOGGLE);
	if (s->user_pin == 0) {
		s->user_pin = 1;
		sys_sensor_start_sampling(sensor, 1, &(s->ctx), &(s->filter));
	} else {
		s->user_pin = 0;
		sys_sensor_stop_sampling(ACCEL_X_SENSOR);
	}
}

#ifndef _MODULE_
mod_header_ptr test_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

