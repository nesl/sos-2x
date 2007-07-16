/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */


#include <sys_module.h>
#include <sensor.h>

#include <mts310sb.h>

#define PHOTO_0_SENSOR_ID 0

static int8_t sounder_control(func_cb_ptr cb, uint8_t cmd, uint8_t options);

static int8_t sounder_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : BUZZER_SENSOR_PID,
  state_size : 0,
  num_timers : 0,
  num_sub_func : 0,
  num_prov_func : 1,
  module_handler : sounder_msg_handler,
	funct : {
		{sounder_control, "cCC2", BUZZER_SENSOR_PID, SENSOR_CONTROL_FID},
	},
};


/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 * why does this currently do nothing?
 */
int8_t sounder_data_ready_cb(uint8_t port, uint16_t value, uint8_t status) {
	return -EINVAL;
}


static inline void buzzer_on() {
	SET_SOUNDER_EN();
	SET_SOUNDER_EN_DD_OUT();
}
static inline void photo_off() {
	SET_SOUNDER_EN_DD_IN();
	CLR_SOUNDER_EN();
}


static int8_t sounder_control(func_cb_ptr cb, uint8_t cmd, void* data) {

	uint8_t options = *(uint8_t*) data;

	switch (cmd) {
		case SENSOR_GET_DATA_CMD:
			// get ready to read sounder sensor
			return sys_adc_proc_getData(MTS310_PHOTO_SID, PHOTO_0_SENSOR_ID);
			break;

		case SENSOR_ENABLE_CMD:
			if (options & PHOTO_SENSOR_FLAG) {
			  // FIXME: these functions don't exist, what should they do?
				photo_on();
			} else {
				temp_on();
			}
			break;

		case SENSOR_DISABLE_CMD:
			if (options & PHOTO_SENSOR_FLAG) {
				photo_off();
			} else {
				temp_off();
			}
			break;

		default:
			return -EINVAL;
	}
	return SOS_OK;
}


int8_t sounder_msg_handler(void *state, Message *msg)
{
  switch (msg->type) {

		case MSG_INIT:
			// bind adc channel and register callback pointer
			sys_adc_proc_bindPort(MTS310_PHOTO_SID, MTS310_PHOTO_ADC, BUZZER_SENSOR_PID, &sounder_data_ready_cb);
			// register with kernel sensor interface
			sys_sensor_register(BUZZER_SENSOR_PID, MTS310_PHOTO_SID, SENSOR_CONTROL_FID, PHOTO_SENSOR_FLAG);
			sys_sensor_register(BUZZER_SENSOR_PID, MTS310_TEMP_SID, SENSOR_CONTROL_FID, TEMP_SENSOR_FLAG);
			break;

		case MSG_FINAL:
			// shutdown sensor
			photo_off();
			temp_off();
			//  unregister ADC port
			sys_adc_proc_unbindPort(BUZZER_SENSOR_PID, MTS310_PHOTO_SID);
			// unregister sensor
			sys_sensor_deregister(BUZZER_SENSOR_PID, MTS310_PHOTO_SID);
			sys_sensor_deregister(BUZZER_SENSOR_PID, MTS310_TEMP_SID);
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

