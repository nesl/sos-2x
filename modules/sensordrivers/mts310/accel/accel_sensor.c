/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>

#include <sensor.h>
//#include <adc_proc.h>


#include <sensordrivers/mts310/include/mts310sb.h>

/**
 * private conguration options for this driver
 */
#define ACCEL_1_SENSOR_ID (1<<6)
#define ACCEL_0_SENSOR_ID (0)

typedef struct accel_sensor_state {
	uint8_t accel_0_state;
	uint8_t accel_1_state;
	uint8_t options;
	uint8_t state;
} accel_sensor_state_t;


// function registered with kernel sensor component
static int8_t accel_control(func_cb_ptr cb, uint8_t cmd, void *data);
// data ready callback registered with adc driver
int8_t accel_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags);

static int8_t accel_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
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
	if (port == MTS310_ACCEL_0_SID) {
		sys_sensor_data_ready(MTS310_ACCEL_0_SID, value, flags);
	} else {
		sys_sensor_data_ready(MTS310_ACCEL_1_SID, value, flags);
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
			if ((ctx & 0xC0) == ACCEL_0_SENSOR_ID) {
				return sys_adc_proc_getData(MTS310_ACCEL_0_SID, ACCEL_0_SENSOR_ID);
			} else {
				return sys_adc_proc_getData(MTS310_ACCEL_1_SID, ACCEL_1_SENSOR_ID);
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

			sys_adc_proc_bindPort(MTS310_ACCEL_0_SID, MTS310_ACCEL_0_HW_CH, ACCEL_SENSOR_PID,  SENSOR_DATA_READY_FID);
			sys_adc_proc_bindPort(MTS310_ACCEL_1_SID, MTS310_ACCEL_1_HW_CH, ACCEL_SENSOR_PID,  SENSOR_DATA_READY_FID);
			// register with kernel sensor interface
			s->accel_0_state = ACCEL_0_SENSOR_ID;
			sys_sensor_register(ACCEL_SENSOR_PID, MTS310_ACCEL_0_SID, SENSOR_CONTROL_FID, (void*)(&s->accel_0_state));
			s->accel_1_state = ACCEL_1_SENSOR_ID;
			sys_sensor_register(ACCEL_SENSOR_PID, MTS310_ACCEL_1_SID, SENSOR_CONTROL_FID, (void*)(&s->accel_1_state));
			break;

		case MSG_FINAL:
			// shutdown sensor
			accel_off();
			//  unregister ADC port
			sys_adc_proc_unbindPort(ACCEL_SENSOR_PID, MTS310_ACCEL_0_SID);
			sys_adc_proc_unbindPort(ACCEL_SENSOR_PID, MTS310_ACCEL_1_SID);
			// unregister sensor
			sys_sensor_deregister(ACCEL_SENSOR_PID, MTS310_ACCEL_0_SID);
			sys_sensor_deregister(ACCEL_SENSOR_PID, MTS310_ACCEL_1_SID);
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

