/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver Support for the SHT11 on TMote
 * 
 */

#include <sys_module.h>
#include <sensor_driver.h>

#include "sht1x_sensor.h"

#define LED_DEBUG
#include <led_dbg.h>

/*
 * This is a special sensor driver because it does not
 * use regular hardware communication protocols like I2C etc.
 * Instead it uses a proprietary 2-wire serial protocol.
 * Thus, it deviates at certain places from the standard SOS template for
 * sensor drivers and directly interacts with the hardware.
 * But it does conform to the standard communication protocol
 * with the sensing API.
 */

#define SHT1x_NUM_SENSORS		2

typedef struct 
{
	func_cb_ptr sht1x_get_data;
	func_cb_ptr sht1x_stop_data;
	uint8_t state;
	sensor_channel_map_t map[SHT1x_NUM_SENSORS];
} sht1x_sensor_state_t;

// control function registered with sensor API
static int8_t sht1x_sensor_control(func_cb_ptr cb, sensor_driver_command_t command,
				sos_pid_t app_id, sensor_id_t sensor, sample_context_t *param, void *context);
// data ready callback used by hardware driver
static int8_t sht1x_sensor_data_ready_cb(func_cb_ptr cb, sht1x_feedback_t fb, sos_pid_t app_id, 
										uint16_t channels, sensor_data_msg_t* buf);
// Sensor control feedback from the hardware driver
static int8_t sht1x_sensor_feedback(func_cb_ptr cb, sensor_driver_command_t command, 
									uint16_t channel, void *context);

static int8_t hardware_error(func_cb_ptr cb);

static int8_t sht1x_msg_handler(void *start, Message *e);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = SHT1x_SENSOR_PID,
    .code_id        = ehtons(SHT1x_SENSOR_PID),
    .state_size     = sizeof(sht1x_sensor_state_t),
    .num_sub_func   = 2,
    .num_prov_func  = 3,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .module_handler = sht1x_msg_handler,
    funct : {
		{hardware_error, "cyy5", SHT1x_COMM_PID, SHT1x_GET_DATA_FID},
		{hardware_error, "cyS2", SHT1x_COMM_PID, SHT1x_STOP_DATA_FID},
		{sht1x_sensor_control, "cCw2", SHT1x_SENSOR_PID, SENSOR_CONTROL_FID},
		{sht1x_sensor_data_ready_cb, "cCS3", SHT1x_SENSOR_PID, SHT1x_DATA_READY_FID},
		{sht1x_sensor_feedback, "cCw2", SHT1x_SENSOR_PID, SHT1x_FEEDBACK_FID},
	},
};

static int8_t sht1x_sensor_control(func_cb_ptr cb, sensor_driver_command_t command,
				sos_pid_t app_id, sensor_id_t sensor, sample_context_t *param, void *context) {
	sht1x_sensor_state_t *s = (sht1x_sensor_state_t *)sys_get_state();
	int8_t ret = SOS_OK;

	// Return if driver is in error state.
	if (s->state == DRIVER_ERROR) return -EINVAL;

	// Get sensor <-> channel mapping for requested sensor
	uint16_t channel = get_channel(sensor, s->map, SHT1x_NUM_SENSORS); 

	// Return if requested sensor is not supported by this driver.
	if (channel == 0) return -EINVAL;

	switch(command) {
		case SENSOR_REGISTER_REQUEST_COMMAND: {
			if (param == NULL) return -EINVAL;
			ret = SOS_CALL(s->sht1x_get_data, sht1x_get_data_func_t, SHT1x_REGISTER_REQUEST, 
							app_id, channel, param, context);
			break;
		}
		case SENSOR_GET_DATA_COMMAND: {
			if (param == NULL) return -EINVAL;
			ret = SOS_CALL(s->sht1x_get_data, sht1x_get_data_func_t, SHT1x_GET_DATA, 
							app_id, channel, param, context);
			break;
		}
		case SENSOR_STOP_DATA_COMMAND: {
			ret = SOS_CALL(s->sht1x_stop_data, sht1x_stop_data_func_t, app_id, channel);
			break;
		}
		default: ret = -EINVAL;
	}

	return ret;
}


static int8_t sht1x_sensor_feedback(func_cb_ptr cb, sensor_driver_command_t command, 
							uint16_t channel, void *context) {
	sht1x_sensor_state_t *s = (sht1x_sensor_state_t *)sys_get_state();

	// Return if driver is in error state.
	if (s->state == DRIVER_ERROR) return -EINVAL;

	// Get sensor <-> channel mapping for requested sensor
	sensor_id_t sensor = get_sensor(channel, s->map, SHT1x_NUM_SENSORS); 

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
                	make_enable_output();
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


static int8_t sht1x_sensor_data_ready_cb(func_cb_ptr cb, sht1x_feedback_t fb, 
		sos_pid_t app_id, uint16_t channels, sensor_data_msg_t* sht1x_buf) {
	sht1x_sensor_state_t *s = (sht1x_sensor_state_t *)sys_get_state();
	sensor_data_msg_t *b = sht1x_buf;
	
	// Get sensor ID from sensor <-> channel mapping
	sensor_id_t sensor = get_sensor(channels, s->map, SHT1x_NUM_SENSORS);
	if (sensor == MAX_NUM_SENSORS) {
		if (b != NULL) sys_free(b);
		return -EINVAL;
	}
	
	switch(fb) {
		case SHT1x_SENSOR_SEND_DATA: {
			// Sanity check: Verify if there is any buffer to send.
			if (b == NULL) return -EINVAL;
			b->status = SENSOR_DATA;
			b->sensor = sensor;
			break;
		}
		case SHT1x_SENSOR_CHANNEL_UNBOUND: {
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
		case SHT1x_SENSOR_SAMPLING_DONE: {
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
		case SHT1x_SENSOR_ERROR: {
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

static int8_t hardware_error(func_cb_ptr cb) {
	return -EINVAL;
}

////
// Code, glorious code!
////

static int8_t sht1x_msg_handler(void *state, Message *msg) { 

    sht1x_sensor_state_t *s = (sht1x_sensor_state_t *)state;

    /** <h2>Types of messages handled by the SHT11 module</h2>
    */
    switch(msg->type) {
        /**
         * \par MSG_INIT
         * Initialize the driver parameters. Do not enable it right now. 
         */

        case MSG_INIT: { 
			// Setup the sensor <-> channel mapping.
			s->map[0].sensor = TEMPERATURE_SENSOR;
			s->map[0].channel = SHT1x_TEMPERATURE_CH;
			s->map[1].sensor = HUMIDITY_SENSOR;
			s->map[1].channel = SHT1x_HUMIDITY_CH;

			// Try to register the sensor driver with sensor API for all 
			// the sensors it supports
			int8_t ret = sys_sensor_driver_register(TEMPERATURE_SENSOR, SENSOR_CONTROL_FID);
			if (ret < 0) {
				s->state = DRIVER_ERROR;
				return -EINVAL;
			}
			ret = sys_sensor_driver_register(HUMIDITY_SENSOR, SENSOR_CONTROL_FID);
			if (ret < 0) {
				sys_sensor_driver_deregister(TEMPERATURE_SENSOR);
				s->state = DRIVER_ERROR;
				return -EINVAL;
			}
		
			// Sensor is disabled by default. Do not turn it ON here.
			s->state = DRIVER_DISABLE;

            break; 
		}
	   /** 
		* \par MSG_FINAL
		* Stick a fork in it, cause this module is done.
		*/    
        case MSG_FINAL: { 
			sys_sensor_driver_deregister(TEMPERATURE_SENSOR);
			sys_sensor_driver_deregister(HUMIDITY_SENSOR);
			break; 
		}   
        default: return -EINVAL;
    }

    return SOS_OK;

}

#ifndef _MODULE_
mod_header_ptr sht1x_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif



