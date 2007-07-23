/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */


#include <sys_module.h>
#include <sensor.h>

#define LED_DEBUG
#include <led_dbg.h>

#include <mts310sb.h>

#define SOUNDER_SENSOR_ID 0

typedef struct{
	uint8_t sounder_state;
} sounder_state_t;

static int8_t sounder_control(func_cb_ptr cb, uint8_t cmd, void* data);

static int8_t sounder_data_ready_cb(uint8_t port, uint16_t value, uint8_t status);

static int8_t sounder_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : SOUNDER_PID,
  state_size : sizeof(sounder_state_t),
  num_timers : 0,
  num_sub_func : 0,
  num_prov_func : 2,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	code_id : ehtons(SOUNDER_PID),
  module_handler : sounder_msg_handler,
	funct : {
		{sounder_control, "cCC2", SOUNDER_PID, SENSOR_CONTROL_FID},
		{sounder_data_ready_cb, "cCS3", SOUNDER_PID, SENSOR_DATA_READY_FID},
	},
};


/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 * why does this currently do nothing?
 * A: there is no sensor to get data from for the sounder
 */
int8_t sounder_data_ready_cb(uint8_t port, uint16_t value, uint8_t status) {
	return -EINVAL;
}


static inline void buzzer_on() {
	SET_SOUNDER_EN();
	SET_SOUNDER_EN_DD_OUT();
}

static inline void buzzer_off() {
	SET_SOUNDER_EN_DD_IN();
	CLR_SOUNDER_EN();
}

static int8_t sounder_control(func_cb_ptr cb, uint8_t cmd, void* data) {


	uint8_t options = *(uint8_t*) data;

	switch (cmd) {
		case SENSOR_GET_DATA_CMD:
			// get ready to read sounder sensor
			// but there is no sounder sensor
			break;

		case SENSOR_ENABLE_CMD:
			buzzer_on();
			break;

		case SENSOR_DISABLE_CMD:
			buzzer_off();
			break;

		default:
			return -EINVAL;
	}
	return SOS_OK;
}


int8_t sounder_msg_handler(void *state, Message *msg)
{
  sounder_state_t *s = (sounder_state_t *) state;
	
  switch (msg->type) {

		case MSG_INIT:
			s->sounder_state = SOUNDER_SENSOR_ID;
      sys_sensor_register(SOUNDER_PID, MTS310_SOUNDER_SID, SENSOR_CONTROL_FID, (void*)(&s->sounder_state));
			break;

		case MSG_FINAL:
			sys_sensor_deregister(SOUNDER_PID, MTS310_SOUNDER_SID);
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr soundersensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

