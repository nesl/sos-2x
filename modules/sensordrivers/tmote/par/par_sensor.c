/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>

#include <sensor.h>
#include <adc_api.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include <tmote_sensors.h>

typedef struct par_sensor_state {
	uint8_t state;
} par_sensor_state_t;


// function registered with kernel sensor component
static int8_t par_sensor_control(func_cb_ptr cb, uint8_t cmd, void *data);
// data ready callback registered with adc driver
int8_t par_sensor_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags);

static int8_t par_sensor_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : LITEPOT_PID,
  state_size : sizeof(par_sensor_state_t),
  num_timers : 0,
  num_sub_func : 0,
  num_prov_func : 2,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	code_id : ehtons(LITEPOT_PID),
  module_handler : par_sensor_msg_handler,
	funct : {
		{par_sensor_control, "cCw2", LITEPOT_PID, SENSOR_CONTROL_FID},
		{par_sensor_data_ready_cb, "cCS3", LITEPOT_PID, SENSOR_DATA_READY_FID},
	},
};


/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 */
int8_t par_sensor_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags) {

	// post data ready message here
	switch(port) {
		case PAR_SID:
      SYS_LED_DBG(LED_RED_TOGGLE);
			ker_sensor_data_ready(PAR_SID, value, flags);
			break;
		default:
			return -EINVAL;
	}
	return SOS_OK;
}


static int8_t par_sensor_control(func_cb_ptr cb, uint8_t cmd, void* data) {\

	//uint8_t ctx = *(uint8_t*)data;
	
	switch (cmd) {
		case SENSOR_GET_DATA_CMD:
			// get ready to read accel sensor
			return sys_adc_get_data(PAR_SID, 0);

		case SENSOR_ENABLE_CMD:
			break;

		case SENSOR_DISABLE_CMD:
			break;

		case SENSOR_CONFIG_CMD:
			// no configuation
			if (data != NULL) {
				sys_free(data);
			}
			break;

		default:
			return -EINVAL;
	}
	return SOS_OK;
}


int8_t par_sensor_msg_handler(void *state, Message *msg)
{
	
	par_sensor_state_t *s = (par_sensor_state_t*)state;
  
	switch (msg->type) {

		case MSG_INIT:
			// bind adc channel and register callback pointer

		  sys_adc_bind_port(PAR_SID, PAR_HW_CH, LITEPOT_PID,  SENSOR_DATA_READY_FID);
			// register with kernel sensor interface
			ker_sensor_register(LITEPOT_PID, PAR_SID, SENSOR_CONTROL_FID, (void*)(&s->state));
			break;

		case MSG_FINAL:
			//  unregister ADC port
			sys_adc_unbind_port(LITEPOT_PID, PAR_SID);
			// unregister sensor
			ker_sensor_deregister(LITEPOT_PID, PAR_SID);
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr par_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

