/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

// Sensor driver module (loadable)
// So only include sensor_driver.h and sys_module.h
// Since this sensor driver uses ADC, no need to add
// hardware specific header file as that is already included
// in sys_module.h
#include <sys_module.h>
#include <sensor_driver.h>

#define LED_DEBUG
#include <led_dbg.h>

// Number of sensors handled by this driver.
#define NUM_SENSORS			2

typedef struct {
	uint8_t state;
	sensor_config_t config;
	sensor_channel_map_t map[NUM_SENSORS];
} accel_sensor_state_t;

// data ready callback registered with adc driver
static int8_t accel_sensor_data_ready_cb(func_cb_ptr cb, adc_feedback_t fb, sos_pid_t app_id, 
							uint16_t channels, sensor_data_msg_t* buf);
// control function registered with sensor API
static int8_t accel_sensor_control(func_cb_ptr cb, sensor_driver_command_t command, 
			sos_pid_t app_id, sensor_id_t sensor, sample_context_t *param, void *context);
// Sensor control feedback from the ADC driver
static int8_t accel_sensor_feedback(func_cb_ptr cb, sensor_driver_command_t command, 
						uint16_t channel, void *context);

static int8_t accel_sensor_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
	mod_id : ACCEL_SENSOR_PID,
	code_id : ehtons(ACCEL_SENSOR_PID),
  	state_size : sizeof(accel_sensor_state_t),
	num_timers : 0,
	num_sub_func : 0,
	num_prov_func : 3,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	module_handler : accel_sensor_msg_handler,
	funct : {
		{accel_sensor_control, "cCw2", ACCEL_SENSOR_PID, SENSOR_CONTROL_FID},
		{accel_sensor_data_ready_cb, "cCS3", ACCEL_SENSOR_PID, SENSOR_DATA_READY_FID},
		{accel_sensor_feedback, "cCw2", ACCEL_SENSOR_PID, SENSOR_FEEDBACK_FID},
	},
};


static int8_t accel_sensor_control(func_cb_ptr cb, sensor_driver_command_t command, 
			sos_pid_t app_id, sensor_id_t sensor, sample_context_t *param, void *context) {
	accel_sensor_state_t *s = (accel_sensor_state_t *)sys_get_state();

	// Return if driver is in error state.
	if (s->state == DRIVER_ERROR) return -EINVAL;

	// Get sensor <-> channel mapping for requested sensor
	uint16_t channel = get_channel(sensor, s->map, NUM_SENSORS); 

	// Return if requested sensor is not supported by this driver.
	if (channel == 0) return -EINVAL;

	switch(command) {
		case SENSOR_REGISTER_REQUEST_COMMAND: {
			if (param == NULL) return -EINVAL;
			return sys_adc_get_data(ADC_REGISTER_REQUEST, app_id, channel, param, context); 
		}
		case SENSOR_GET_DATA_COMMAND: {
			if (param == NULL) return -EINVAL;
			return sys_adc_get_data(ADC_GET_DATA, app_id, channel, param, context); 
		}
		case SENSOR_STOP_DATA_COMMAND: {
			return sys_adc_stop_data(app_id, channel);
		}
		default: return -EINVAL;
	}

	return SOS_OK;
}

static int8_t accel_sensor_feedback(func_cb_ptr cb, sensor_driver_command_t command, 
						uint16_t channel, void *context) {
	accel_sensor_state_t *s = (accel_sensor_state_t *)sys_get_state();

	// Return if driver is in error state.
	if (s->state == DRIVER_ERROR) return -EINVAL;

	// Get sensor <-> channel mapping for requested sensor
	sensor_id_t sensor = get_sensor(channel, s->map, NUM_SENSORS); 

	// Return if requested sensor is not supported by this driver.
	if (sensor == MAX_NUM_SENSORS) return -EINVAL;

	switch(command) {
		case SENSOR_ENABLE_COMMAND: {
			switch(s->state) {
				case DRIVER_ENABLE: {
					// Re-configure the sensor according to passed context or
					// default settings.
					return SOS_OK;
				}
				case DRIVER_DISABLE: {
					// Turn ON the sensor, and configure it according to
					// passed context or default settings.
					s->state = DRIVER_ENABLE;
					return SOS_OK;
				}
				default: return -EINVAL;
			}
		}
		case SENSOR_DISABLE_COMMAND: {
			switch(s->state) {
				case DRIVER_DISABLE: {
					// Already disabled. Do nothing.
					return SOS_OK;
				}
				case DRIVER_ENABLE: {
					// Turn OFF the sensor.
					s->state = DRIVER_DISABLE;
					return SOS_OK;
				}
				default: return -EINVAL;
			}
			return SOS_OK;
		}
		default: return -EINVAL;
	}

	return SOS_OK;
}

static int8_t accel_sensor_data_ready_cb(func_cb_ptr cb, adc_feedback_t fb, sos_pid_t app_id, 
							uint16_t channels, sensor_data_msg_t* adc_buf) {
	accel_sensor_state_t *s = (accel_sensor_state_t *)sys_get_state();
	sensor_data_msg_t *b = adc_buf;
	
	// Get sensor ID from sensor <-> channel mapping
	sensor_id_t sensor = get_sensor(channels, s->map, NUM_SENSORS);
	if (sensor == MAX_NUM_SENSORS) {
		if (b != NULL) sys_free(b);
		return -EINVAL;
	}
	
	switch(fb) {
		case ADC_SENSOR_SEND_DATA: {
			// Sanity check: Verify if there is any buffer to send.
			if (b == NULL) return -EINVAL;
			b->status = SENSOR_DATA;
			b->sensor = sensor;
			break;
		}
		case ADC_SENSOR_CHANNEL_UNBOUND: {
			// 'b' should not point to any buffer here.
			if (b != NULL) sys_free(b);
			// Status buffer: Allocate space for 'b' with 0 samples.
			b = (sensor_data_msg_t *)sys_malloc(sizeof(sensor_data_msg_t));
			if (b == NULL) return -EINVAL;
			b->status = SENSOR_DRIVER_UNREGISTERED;
			b->sensor = sensor;
			b->num_samples = 0;
			break;
		}
		case ADC_SENSOR_SAMPLING_DONE: {
			// 'b' should not point to any buffer here.
			if (b != NULL) sys_free(b);
			// Status buffer: Allocate space for 'b' with 0 samples.
			b = (sensor_data_msg_t *)sys_malloc(sizeof(sensor_data_msg_t));
			if (b == NULL) return -EINVAL;
			b->status = SENSOR_SAMPLING_STOPPED;
			b->sensor = sensor;
			b->num_samples = 0;
			break;
		}
		case ADC_SENSOR_ERROR: {
			// 'b' should not point to any buffer here.
			if (b != NULL) sys_free(b);
			// Status buffer: Allocate space for 'b' with 0 samples.
			b = (sensor_data_msg_t *)sys_malloc(sizeof(sensor_data_msg_t));
			if (b == NULL) return -EINVAL;
			b->status = SENSOR_SAMPLING_ERROR;
			b->sensor = sensor;
			b->num_samples = 0;
			break;
		}
		default: {
			// 'b' should not point to any buffer here.
			if (b != NULL) sys_free(b);
			// Status buffer: Allocate space for 'b' with 0 samples.
			b = (sensor_data_msg_t *)sys_malloc(sizeof(sensor_data_msg_t));
			if (b == NULL) return -EINVAL;
			b->status = SENSOR_STATUS_UNKNOWN;
			b->sensor = sensor;
			b->num_samples = 0;
			break;
		}
	}

	// Post buffer to application
	sys_post(app_id, MSG_DATA_READY, sizeof(sensor_data_msg_t) + 
			(b->num_samples*sizeof(uint16_t)), b, SOS_MSG_RELEASE);

	return SOS_OK;
}

int8_t accel_sensor_msg_handler(void *state, Message *msg) { 
	accel_sensor_state_t *s = (accel_sensor_state_t *)state;

	switch (msg->type) {
		case MSG_INIT: {
			// Setup the sensor <-> channel mapping.
			s->map[0].sensor = ACCEL_X_SENSOR;
			s->map[0].channel = ADC_DRIVER_CH0;
			s->map[1].sensor = ACCEL_Y_SENSOR;
			s->map[1].channel = ADC_DRIVER_CH1;

			// Setup the channel configuration parameters.
			// For num_sensors > 1, there may be different configurations for 
			// different channels, so setup multiple configurations accordingly.
			s->config.sht0 = ADC_SAMPLE_TIME_64;
			s->config.ref2_5 = ADC_INTERNAL_REF2_5V;

			// Try to register the sensor driver with sensor API for all 
			// the sensors it supports
			int8_t ret = sys_sensor_driver_register(ACCEL_X_SENSOR, SENSOR_CONTROL_FID);
			if (ret < 0) {
				s->state = DRIVER_ERROR;
				return -EINVAL;
			}
			ret = sys_sensor_driver_register(ACCEL_Y_SENSOR, SENSOR_CONTROL_FID);
			if (ret < 0) {
				s->state = DRIVER_ERROR;
				sys_sensor_driver_deregister(ACCEL_X_SENSOR);
				return -EINVAL;
			}

			// Try to bind sensors to their respective ADC channels.
		  	ret = sys_adc_bind_channel(get_all_channels(s->map, NUM_SENSORS), 
							SENSOR_FEEDBACK_FID, SENSOR_DATA_READY_FID, &(s->config));
			if (ret < 0) {
				// Deregister the driver from sensor API
				sys_sensor_driver_deregister(ACCEL_X_SENSOR);
				sys_sensor_driver_deregister(ACCEL_Y_SENSOR);
				// Error in binding to channel. Set ERROR state.
				s->state = DRIVER_ERROR;
				return -EINVAL;
			}

			// Sensor is disabled by default. Do not turn it ON here.
			s->state = DRIVER_DISABLE;

			break;
		}
		case MSG_FINAL: {
			sys_adc_unbind_channel(get_all_channels(s->map, NUM_SENSORS));
			sys_sensor_driver_deregister(ACCEL_X_SENSOR);
			sys_sensor_driver_deregister(ACCEL_Y_SENSOR);
			break;
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr accel_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

