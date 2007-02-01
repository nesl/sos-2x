/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <module.h>
#include <sys_module.h>

#include <sensor.h>
#include <adc_proc.h>

#include <h34c.h>

/**
 * private conguration options for this driver. This is used to differenciate
 * the calls to the ADC and sensor registration system.
 */
#define ACCEL_0_SENSOR_ID (0)
#define ACCEL_1_SENSOR_ID (1<<6)
#define ACCEL_2_SENSOR_ID (2<<6)

typedef struct accel_sensor_state {
	uint8_t accel_0_state;
	uint8_t accel_1_state;
	uint8_t accel_2_state;
	uint8_t options;
	uint8_t state;
} accel_sensor_state_t;


// function registered with kernel sensor component
static int8_t accel_control(func_cb_ptr cb, uint8_t cmd, void *data);
// data ready callback registered with adc driver
int8_t accel_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags);

static int8_t accel_msg_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : ACCEL_SENSOR_PID,
  state_size : sizeof(accel_sensor_state_t),
  num_timers : 0,
  num_sub_func : 0,
  num_prov_func : 2,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	code_id : ehtons(ACCEL_SENSOR_PID),
  module_handler : accel_msg_handler,
	funct : {
		{accel_control, "cCw2", ACCEL_SENSOR_PID, SENSOR_CONTROL_FID},
		{accel_data_ready_cb, "cCS3", ACCEL_SENSOR_PID, SENSOR_DATA_READY_FID},
	},
};


/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 */
int8_t accel_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags) {

	// post data ready message here
	switch(port) {
		case H34C_ACCEL_0_SID:
			ker_sensor_data_ready(H34C_ACCEL_0_SID, value, flags);
			break;
		case H34C_ACCEL_1_SID:
			ker_sensor_data_ready(H34C_ACCEL_1_SID, value, flags);
			break;
		case H34C_ACCEL_2_SID:
			ker_sensor_data_ready(H34C_ACCEL_2_SID, value, flags);
			break;
		default:
			return -EINVAL;
	}
	return SOS_OK;
}


static inline void accel_on() {
	SET_ACCEL_EN();
	SET_ACCEL_EN_DD_OUT();
}
static inline void accel_off() {
	SET_ACCEL_EN_DD_IN();
	CLR_ACCEL_EN();
}

static int8_t accel_control(func_cb_ptr cb, uint8_t cmd, void* data) {\

	uint8_t ctx = *(uint8_t*)data;
	
	switch (cmd) {
		case SENSOR_GET_DATA_CMD:
			// get ready to read accel sensor
			switch(ctx & 0xC0) {
				case ACCEL_0_SENSOR_ID:
					return ker_adc_proc_getData(H34C_ACCEL_0_SID, ACCEL_0_SENSOR_ID);
				case ACCEL_1_SENSOR_ID:
					return ker_adc_proc_getData(H34C_ACCEL_1_SID, ACCEL_1_SENSOR_ID);
				case ACCEL_2_SENSOR_ID:
					return ker_adc_proc_getData(H34C_ACCEL_2_SID, ACCEL_2_SENSOR_ID);
				default:
					return -EINVAL;
			}
			break;

		case SENSOR_ENABLE_CMD:
			accel_on();
			break;

		case SENSOR_DISABLE_CMD:
			accel_off();
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


int8_t accel_msg_handler(void *state, Message *msg)
{
	
	accel_sensor_state_t *s = (accel_sensor_state_t*)state;
  
	switch (msg->type) {

		case MSG_INIT:
			// bind adc channel and register callback pointer

			ker_adc_proc_bindPort(H34C_ACCEL_0_SID, H34C_ACCEL_0_HW_CH, ACCEL_SENSOR_PID,  SENSOR_DATA_READY_FID);
			ker_adc_proc_bindPort(H34C_ACCEL_1_SID, H34C_ACCEL_1_HW_CH, ACCEL_SENSOR_PID,  SENSOR_DATA_READY_FID);
			ker_adc_proc_bindPort(H34C_ACCEL_2_SID, H34C_ACCEL_2_HW_CH, ACCEL_SENSOR_PID,  SENSOR_DATA_READY_FID);
			// register with kernel sensor interface
			s->accel_0_state = ACCEL_0_SENSOR_ID;
			ker_sensor_register(ACCEL_SENSOR_PID, H34C_ACCEL_0_SID, SENSOR_CONTROL_FID, (void*)(&s->accel_0_state));
			s->accel_1_state = ACCEL_1_SENSOR_ID;
			ker_sensor_register(ACCEL_SENSOR_PID, H34C_ACCEL_1_SID, SENSOR_CONTROL_FID, (void*)(&s->accel_1_state));
			s->accel_2_state = ACCEL_2_SENSOR_ID;
			ker_sensor_register(ACCEL_SENSOR_PID, H34C_ACCEL_2_SID, SENSOR_CONTROL_FID, (void*)(&s->accel_2_state));
			break;

		case MSG_FINAL:
			// shutdown sensor
			accel_off();
			//  unregister ADC port
			ker_adc_proc_unbindPort(ACCEL_SENSOR_PID, H34C_ACCEL_0_SID);
			ker_adc_proc_unbindPort(ACCEL_SENSOR_PID, H34C_ACCEL_1_SID);
			ker_adc_proc_unbindPort(ACCEL_SENSOR_PID, H34C_ACCEL_2_SID);
			// unregister sensor
			ker_sensor_deregister(ACCEL_SENSOR_PID, H34C_ACCEL_0_SID);
			ker_sensor_deregister(ACCEL_SENSOR_PID, H34C_ACCEL_1_SID);
			ker_sensor_deregister(ACCEL_SENSOR_PID, H34C_ACCEL_2_SID);
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr accel_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

