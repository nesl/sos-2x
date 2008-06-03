/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>

#define LED_DEBUG
#include <led_dbg.h>

#define TEST_SENSOR_PID			DFLT_APP_ID0
#define USER_INT_FID			0

typedef struct {
	sample_context_t ctx[2];
	uint16_t sequence;

	uint16_t num_samples;
	sos_pid_t app_id;
	uint16_t channels;
	sensor_status_t status;
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
			s->sequence = 0;
			s->ctx[0].delay = 0;
			// The sensors connected to the internal ADC use the ticks from the 32KHz clock
			s->ctx[0].period = 32768; // this is in ticks from a 32kHz clock
			s->ctx[0].samples = 0; // samples=0 means collect an infinite number of samples
			s->ctx[0].event_samples = 1; //event_samples=1 means send out one sample per buffer
			s->ctx[1].delay = 200; // the temperature sensor needs 200ms of warmup
			// The SHT1x sensors use the ticks from the 1024Hz SOS timer
			s->ctx[1].period = 512; // this is in ticks from the sos timer, e.g. 1024=1second
			s->ctx[1].samples = 64;
			s->ctx[1].event_samples = 32;
			s->ctx[2].delay = 0;
			// Module starts sampling when the user button is pressed
			if (sys_register_isr(PORT2_INTERRUPT, USER_INT_FID) == 0) {
				LED_DBG(LED_RED_TOGGLE);
			}
			break;
		}
		case MSG_TIMER_TIMEOUT: {
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
			sys_sensor_stop_sampling(LIGHT_PAR_SENSOR);
			sys_sensor_stop_sampling(TEMPERATURE_SENSOR);
			sys_deregister_isr(PORT2_INTERRUPT);
			//sys_timer_stop(0);
			break;
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}

static void user_isr() {
	test_sensor_state_t *s = (test_sensor_state_t*)sys_get_state();
	sensor_id_t sensor[4] = {LIGHT_AMBIENT_SENSOR, LIGHT_PAR_SENSOR, INTERNAL_TEMPERATURE_SENSOR, INTERNAL_VOLTAGE_SENSOR, };

	LED_DBG(LED_RED_TOGGLE);
	sys_sensor_start_sampling(sensor, 4, &(s->ctx[0]), NULL);
	sensor[0] = TEMPERATURE_SENSOR;
	sensor[1] = HUMIDITY_SENSOR;
	sys_sensor_start_sampling(sensor, 2, &(s->ctx[1]), NULL);
}

#ifndef _MODULE_
mod_header_ptr test_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

