/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>

#include <sensor.h>
//#include <adc_proc.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include <sensordrivers/mts310/include/mts310sb.h>

/**
 * private conguration options for this driver
 */
#define TEMP2_SENSOR_ID (2<<6)
#define TEMP1_SENSOR_ID (1<<6)
#define TEMP_SENSOR_ID  TEMP1_SENSOR_ID
#define PHOTO_SENSOR_ID (0)


/**
 * the mts300/310 sensor boards share a single ADC channel to measure both photo and temp
 *
 * the temp sensor can either be RT1 loaded on the board or RT2 which can be
 * externaly connected.  since both temp sensors share the same excitation only
 * one can be used.  this will simply be refered to as temp.
 *
 */

typedef struct phototemp_sensor_state {
	uint8_t options;
} phototemp_sensor_state_t;

typedef struct phototemp_state {
	phototemp_sensor_state_t photo_state;
	phototemp_sensor_state_t temp_state;
	uint8_t options;
	uint8_t state;
} phototemp_state_t;

// function registered with kernel sensor component
static int8_t phototemp_control(func_cb_ptr cb, uint8_t cmd, void *data);
// data ready callback registered with adc driver
int8_t phototemp_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t status);

static int8_t phototemp_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : PHOTOTEMP_SENSOR_PID,
  state_size : sizeof(phototemp_state_t),
  num_timers : 0,
  num_sub_func : 0,
  num_prov_func : 2,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	code_id : ehtons(PHOTOTEMP_SENSOR_PID),
	module_handler : phototemp_msg_handler,
	funct : {
		{phototemp_control, "cCw2", PHOTOTEMP_SENSOR_PID, SENSOR_CONTROL_FID},
		{phototemp_data_ready_cb, "cCS3", PHOTOTEMP_SENSOR_PID, SENSOR_DATA_READY_FID},
	},
};

static inline void photo_on() {
	SET_PHOTO_EN();
	SET_PHOTO_EN_DD_OUT();
}
static inline void photo_off() {
	SET_PHOTO_EN_DD_IN();
	CLR_PHOTO_EN();
}

static inline void temp_on() {
	SET_TEMP_EN();
	SET_TEMP_EN_DD_OUT();
}
static inline void temp_off() {
	SET_TEMP_EN_DD_IN();
	CLR_TEMP_EN();
}

/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 */
int8_t phototemp_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags) {

	// post data ready message here
	if ((flags & 0xC0) == PHOTO_SENSOR_ID) {
		photo_off();
		sys_sensor_data_ready(MTS310_PHOTO_SID, value, flags);
	} else {
		temp_off();
		sys_sensor_data_ready(MTS310_TEMP_SID, value, flags);
	}
	return SOS_OK;
}


static int8_t phototemp_control(func_cb_ptr cb, uint8_t cmd, void* data) {

	phototemp_sensor_state_t *ctx = (phototemp_sensor_state_t*)data;

	switch (cmd) {
		case SENSOR_GET_DATA_CMD:
			// get ready to read phototemp sensor
			if ((ctx->options & 0xC0) == PHOTO_SENSOR_ID) {
				photo_on();
			} else {
				temp_on();
			}
			return sys_adc_proc_getData(MTS310_PHOTO_SID, (ctx->options & 0xC0));

		case SENSOR_ENABLE_CMD:
			if ((ctx->options & 0xC0) == PHOTO_SENSOR_ID) {
				photo_on();
			} else {
				temp_on();
			}
			break;

		case SENSOR_DISABLE_CMD:
			if ((ctx->options & 0xC0) == PHOTO_SENSOR_ID) {
				photo_off();
			} else {
				temp_off();
			}
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


int8_t phototemp_msg_handler(void *state, Message *msg)
{
	
	phototemp_state_t *s = (phototemp_state_t*)state;
  
	switch (msg->type) {

		case MSG_INIT:
			LED_DBG(LED_RED_OFF);
			LED_DBG(LED_GREEN_OFF);
			LED_DBG(LED_YELLOW_OFF);
			LED_DBG(LED_RED_ON);
			// bind adc channel and register callback pointer
			sys_adc_proc_bindPort(MTS310_PHOTO_SID, MTS310_PHOTO_HW_CH, PHOTOTEMP_SENSOR_PID, SENSOR_DATA_READY_FID);

			// register with kernel sensor interface
			s->photo_state.options = PHOTO_SENSOR_ID;
			sys_sensor_register(PHOTOTEMP_SENSOR_PID, MTS310_PHOTO_SID, SENSOR_CONTROL_FID, (void*)(&s->photo_state));
			s->temp_state.options = TEMP_SENSOR_ID;
			sys_sensor_register(PHOTOTEMP_SENSOR_PID, MTS310_TEMP_SID, SENSOR_CONTROL_FID, (void*)(&s->temp_state));
			break;

		case MSG_FINAL:
			// shutdown sensor
			photo_off();
			temp_off();
			//  unregister ADC port
			sys_adc_proc_unbindPort(PHOTOTEMP_SENSOR_PID, MTS310_PHOTO_SID);
			// unregister sensor
			sys_sensor_deregister(PHOTOTEMP_SENSOR_PID, MTS310_PHOTO_SID);
			sys_sensor_deregister(PHOTOTEMP_SENSOR_PID, MTS310_TEMP_SID);
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr phototemp_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

