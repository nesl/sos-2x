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

#define NUM_SENSORS				1

typedef struct {
	sample_context_t ctx;
	sensor_id_t sensor[NUM_SENSORS];
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
			// Set sensor array consisting of sensor ID's
			// that need to be sampled
			s->sensor[0] = ACCEL_X_SENSOR;
			// Set sampling rate to 250 Hz.
			// Period = 128 in ticks from a 32kHz clock
			s->ctx.period = 128;
			// Set continuous non-stop sampling
			s->ctx.samples = 0;
			// Set the buffer size to 32 samples
			s->ctx.event_samples = 32;
			// No filtering is required right now
			s->filter = FILTER_NONE;
			// Module starts sampling when the user button is pressed
			sys_register_isr(PORT2_INTERRUPT, USER_INT_FID);
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
			// Stopping sampling for even one sensor stops
			// sampling for all other sensors too.
			sys_sensor_stop_sampling(s->sensor[0]);
			// Deregister interrupt handler
			sys_deregister_isr(PORT2_INTERRUPT);
			break;
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}

static void user_isr() {
	test_sensor_state_t *s = (test_sensor_state_t*)sys_get_state();

	LED_DBG(LED_RED_TOGGLE);
	if (s->user_pin == 0) {
		s->user_pin = 1;
		sys_sensor_start_sampling(s->sensor, NUM_SENSORS, &(s->ctx), &(s->filter));
	} else {
		s->user_pin = 0;
		sys_sensor_stop_sampling(s->sensor[0]);
	}
}

#ifndef _MODULE_
mod_header_ptr test_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

