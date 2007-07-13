/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * ADS7828 Driver Module
 *
 * Gets readings from the ADS7828
 * 
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \author Mike Buchanan
 * \author Ben Jencks
 */

#include <sys_module.h>
#include <sensor.h>
// High level names of the functionality provided by the ADS7828
#include "ads7828_internal.h"
#include <led_dbg.h>

#include <ads7828.h>
#include <i2c_system.h>

////
// Module specific state and header
////

typedef struct ads7828_state ads7828_state_t;

/**
 * Sensor state.
 */
typedef struct ads7828_sensor_state {
	ads7828_state_t *mod_state;
	// Yes, it's redundant to have both sensor_id and command.
	uint8_t sensor_id;
	uint8_t command;
} ads7828_sensor_state_t;

/**
 * Module state.  Yippie skippie.
 */
struct ads7828_state {
	ads7828_sensor_state_t sensors[ADS7828_NUM_SENSORS];
	uint8_t cur_sensor;
};


/**
 * Special module message handler
 */
static int8_t ads7828_msg_handler(void *state, Message *msg);
static int8_t ads7828_control(func_cb_ptr cb, uint8_t cmd, void* sensor_state);

static uint16_t inline byteswaps(uint16_t word);

/**
 * Module header.  Um, yay.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : ADS7828_SENSOR_PID,
  state_size : sizeof(ads7828_state_t),
  num_timers : 0,
  num_sub_func : 0,
  num_prov_func : 1,
  platform_type : HW_TYPE,
  processor_type : MCU_TYPE,
  code_id : ehtons(ADS7828_SENSOR_PID),
  module_handler : ads7828_msg_handler,
  funct : {
    {ads7828_control, "cCw2", ADS7828_SENSOR_PID, SENSOR_CONTROL_FID},
  },
};


////
// Local function prototypes
////


////
// Code.  Finally, code.
////

/** Sensor control function.
 */
static int8_t ads7828_control(func_cb_ptr cb, uint8_t cmd, void* sensor_state) {
	int power_mask = THERMISTOR;
	ads7828_sensor_state_t *s = (ads7828_sensor_state_t*)sensor_state;
	HAS_CRITICAL_SECTION;
	switch (cmd) {
		case SENSOR_GET_DATA_CMD:
		// Try to reserve the I2C bus
			if (sys_i2c_reserve_bus(ADS7828_SENSOR_PID, I2C_ADDRESS, 
							I2C_SYS_MASTER_FLAG | I2C_SYS_TX_FLAG) != SOS_OK) {
				return -EBUSY;
			}
		  
			if (sys_i2c_send_data(ADS7828_I2C_ADDR,
								 &(s->command),
								 1,
								 ADS7828_SENSOR_PID) != SOS_OK) {
				sys_i2c_release_bus(ADS7828_SENSOR_PID);
				return -EBUSY;
			}
			s->mod_state->cur_sensor = s->sensor_id;
			break;

		case SENSOR_ENABLE_CMD:
			// Temporary code.
		    ENTER_CRITICAL_SECTION();
		    ADS7828_DIRECTION = ADS7828_DIRECTION | power_mask;
		    ADS7828_PORT = ADS7828_PORT | power_mask;
		    LEAVE_CRITICAL_SECTION();
			break;
			
		case SENSOR_DISABLE_CMD:
			// Can we turn them off?
			break;
			
		case SENSOR_CONFIG_CMD:
			// For now, do nothing. Perhaps we can switch to differential
			// measurements?
			// Apparently we have to free it, according to the drivers I'm
			// copying from.
			if (sensor_state != NULL) {
				sys_free(sensor_state);
			}
			break;

		default:
			return -EINVAL;
	}
	return SOS_OK;
}

/** Main message handler for ADS7828
 */
static int8_t ads7828_msg_handler(void *state, Message *msg) { 
 
    ads7828_state_t *s = (ads7828_state_t *)state;
	int i;

    /**
     * <h2>Types of messages handled by the ADS7828 module</h2>
     */
    switch(msg->type) {

        /**
         * \par MSG_INIT:
		 * Register all our sensors, and make states for them. Each state
		 * tells the sensor code which sensor to poll, and the command to send
		 * over I2C.
         */
		case MSG_INIT:
			// Set up a state for each sensor, giving it the appropriate
			// command, then register the sensor.
			// Yes, it's a bit hackish to iterate over an enumeration, but
			// it's better than having eight copies of the same code.
			for (i = 0; i < ADS7828_NUM_SENSORS; i++) {
				s->sensors[i].mod_state = s;
				s->sensors[i].sensor_id = i+1;

				// Rather than index into an array (which is apparently broken)
				// just use a switch.
				// s->sensors[i].command = ADS7828_SENSOR_CHANNELS[i];
				switch (i) {
					case 0:
						s->sensors[i].command = ADS7828_CH0;
						break;
					case 1:
						s->sensors[i].command = ADS7828_CH1;
						break;
					case 2:
						s->sensors[i].command = ADS7828_CH2;
						break;
					case 3:
						s->sensors[i].command = ADS7828_CH3;
						break;
					case 4:
						s->sensors[i].command = ADS7828_CH4;
						break;
					case 5:
						s->sensors[i].command = ADS7828_CH5;
						break;
					case 6:
						s->sensors[i].command = ADS7828_CH6;
						break;
					case 7:
						s->sensors[i].command = ADS7828_CH7;
						break;
				}
				// 1 1 -- Leave everything on.
				s->sensors[i].command |= (ADS7828_PD0 | ADS7828_PD1);
				
				sys_sensor_register(ADS7828_SENSOR_PID, i+1, SENSOR_CONTROL_FID,
						(void*)(&s->sensors[i]));
			}
			break;
		case MSG_FINAL:
			// Unregister everything. All the states are in our global state,
			// which the kernel manages for us.
			for (i = 1; i <= ADS7828_NUM_SENSORS; i++) {
				sys_sensor_deregister(ADS7828_SENSOR_PID, i);
			}
			break;
		/**
		 * Turns things on
		 */
		/*case ADS_POWER_ON:
			uint8_t* power_mask = sys_msg_take_data(msg);


		    break;*/

		/**
		 * Request has been sent.  Now read the data from the device.  The
		 * device is still reserved, so we are good to go.
		 */
		case MSG_I2C_SEND_DONE:
            // Re-reserve the I2C bus in read mode
            if(sys_i2c_reserve_bus(ADS7828_SENSOR_PID, I2C_ADDRESS,
									I2C_SYS_MASTER_FLAG) != SOS_OK) {
				return -EBUSY;
            }

			if (sys_i2c_read_data(ADS7828_I2C_ADDR, 2,
								  ADS7828_SENSOR_PID) !=SOS_OK) {
				sys_i2c_release_bus(ADS7828_SENSOR_PID);
				return -EBUSY;
			}
			break;
 

		/**
		 * Read the data!
		 */
		case MSG_I2C_READ_DONE:
			// Swapping byte order here because everything else seems to
			// expect it in the opposite order.
			sys_sensor_data_ready(s->cur_sensor, byteswaps(*(uint16_t*)(msg->data)), SOS_OK);
			sys_i2c_release_bus(ADS7828_SENSOR_PID);
			break;


		default:
			return -EINVAL;
	}

    return SOS_OK;
}

/*
 * Forcibly byteswap, because entohs doesn't do anything on the motes.
 */
static uint16_t inline byteswaps(uint16_t word)
{
	word = (word << 8) ^ word;
	word = (word >> 8) ^ word;
	word = (word << 8) ^ word;
	return word;
}

#ifndef _MODULE_
mod_header_ptr ads7828_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif




