/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
//written by: Martin Moreno

#include <sys_module.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include <sensor.h>

#include <mts310sb.h>
#include <i2c_system.h>

/**
 * private conguration options for this driver
 */
#define MAG_1_SENSOR_ID (1<<6)
#define MAG_0_SENSOR_ID (0)
#define MIDDLE_ADC_VALUE 0x0200
#define MIDDLE_ADC_OFFSET 0x0010
#define I2C_POTEN_ADDR 0x2C
#define STEADY_NUM 3
#define CALLIBRATE_MAX_TIME 2048

typedef struct magnet_sensor_state {
	uint8_t magnet_0_state;
	uint8_t magnet_1_state;
	uint8_t callibrate_mode;
	uint8_t step_size;
	uint8_t i2c_data_value;
	uint16_t *i2c_data_ptr;
	uint8_t current_axis;
	uint8_t high_or_low;
	uint8_t steady_count;
	uint8_t callibrate_timeout;
} magnet_sensor_state_t;


// function registered with kernel sensor component
static int8_t magnet_control(func_cb_ptr cb, uint8_t cmd, void *data);
// data ready callback registered with adc driver
int8_t magnet_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags);

static int8_t magnet_msg_handler(void *state, Message *msg);
static inline int8_t mag_callibrate(uint8_t x_or_y);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
  mod_id : MAG_SENSOR_PID,
  state_size : sizeof(magnet_sensor_state_t),
  num_timers : 2,
  num_sub_func : 0,
  num_prov_func : 2,
	platform_type : HW_TYPE,
	processor_type : MCU_TYPE,
	code_id : ehtons(MAG_SENSOR_PID),
  module_handler : magnet_msg_handler,
	funct : {
		{magnet_control, "cCw2", MAG_SENSOR_PID, SENSOR_CONTROL_FID},
		{magnet_data_ready_cb, "cCS3", MAG_SENSOR_PID, SENSOR_DATA_READY_FID},
	},
};


/**
 * adc call back
 * not a one to one mapping so not SOS_CALL
 */
int8_t magnet_data_ready_cb(func_cb_ptr cb, uint8_t port, uint16_t value, uint8_t flags) {

	// get the current state of the sensor
	magnet_sensor_state_t *s = (magnet_sensor_state_t*) sys_get_state();

	if(s->callibrate_mode == 0) { // we are NOT calibrating, this is real data
		LED_DBG(LED_GREEN_TOGGLE);
		// post data ready message here
		if (port == MTS310_MAG_0_SID) {
			sys_sensor_data_ready(MTS310_MAG_0_SID, value, flags);
		} else {
			sys_sensor_data_ready(MTS310_MAG_1_SID, value, flags);
		}

	}
	else // we are in callibrate mode
		{
			uint8_t poten_register;

			// check what port we are callibrating at set the appropriate register on the potentiometer
			if (port == MTS310_MAG_0_SID)
				poten_register = 0x00;
			else
				poten_register = 0x80;

			s->i2c_data_ptr = sys_malloc(sizeof(uint16_t));

			// DEBUG PURPOSES
			uint32_t temp1, temp2;

			temp1 = port;
			temp1 = temp1<<24;
			temp2 = s->i2c_data_value;
			temp2 = temp2<<16;
			uint32_t *uart_data = sys_malloc(sizeof(uint32_t));
			*uart_data = ((value & (0x00FF))<<8)|(value >> 8)|temp1|temp2;
			sys_post_uart(MAG_SENSOR_PID, MSG_DATA_READY, sizeof(uint32_t), (void*)uart_data, SOS_MSG_RELEASE, UART_ADDRESS);

			 
			// check the ADC value that was previously read
		  if(value > (MIDDLE_ADC_VALUE + MIDDLE_ADC_OFFSET)) { //we are too high
				
				if(s->high_or_low == 0) { // if we were too low last time

					if(s->step_size != 1) { // make sure we dont change the stepsize to zero
						s->step_size = (s->step_size)>>1;
					}

					s->i2c_data_value += s->step_size;
				}
				else {
					s->i2c_data_value += s->step_size;
				}

				s->high_or_low = 1;
				s->steady_count = 0;

			}
			
			else if(value < (MIDDLE_ADC_VALUE - MIDDLE_ADC_OFFSET)) {	// we are too low
				
				if(s->high_or_low == 1) { // if we were too high last time

					if(s->step_size != 1) {
						s->step_size = (s->step_size)>>1;
					}

					s->i2c_data_value -= s->step_size;
				}
				else {
					s->i2c_data_value -= s->step_size;
				}

				s->high_or_low = 0;
				s->steady_count = 0;

			}

			else { // we are in the intended middle range

				if(s->steady_count < STEADY_NUM) {// make sure we are steadily in this range
					(s->steady_count)++;
				}
				else {
					s->callibrate_mode = 0; // turn off callibrate mode
					return SOS_OK;
					
				}
			}


			*(s->i2c_data_ptr) = poten_register | ((s->i2c_data_value)<<8);
			sys_i2c_reserve_bus(MAG_SENSOR_PID, I2C_ADDRESS, I2C_SYS_MASTER_FLAG|I2C_SYS_TX_FLAG);
			sys_i2c_send_data(I2C_POTEN_ADDR, (uint8_t*)(s->i2c_data_ptr), 2, MAG_SENSOR_PID);
		}

	return SOS_OK;
}


static inline void magnet_on() {
	SET_MAG_EN();
	SET_MAG_EN_DD_OUT();
}
static inline void magnet_off() {
	SET_MAG_EN_DD_IN();
	CLR_MAG_EN();
}


static inline int8_t mag_callibrate(uint8_t x_or_y) {

	magnet_sensor_state_t *s = (magnet_sensor_state_t*) sys_get_state();
	
	s->callibrate_mode = 1; // we are now in calibrate mode
	s->step_size = 0x40;
	s->i2c_data_value = 0x00;
	s->high_or_low = 1;
	s->steady_count = 0;
	s->callibrate_timeout = 0;

	(s->i2c_data_ptr) = sys_malloc(sizeof(uint16_t));

	// setup the i2c on the corresponding sensor
	if(x_or_y == 0) {
		s->current_axis = 0;
		*(s->i2c_data_ptr) = 0x0000 | ((s->i2c_data_value)<<8);
	}
	else if(x_or_y == 1){
		s->current_axis = 1;
		*(s->i2c_data_ptr) = 0x0080 | ((s->i2c_data_value)<<8);
	}
	else {
		return -EINVAL;
	}

	if(SOS_OK == sys_i2c_reserve_bus(MAG_SENSOR_PID, I2C_ADDRESS, I2C_SYS_MASTER_FLAG|I2C_SYS_TX_FLAG)) {
		sys_i2c_send_data(I2C_POTEN_ADDR, (uint8_t*)(s->i2c_data_ptr), 2, MAG_SENSOR_PID);
	}
	else {
		return -EBUSY;
	}		
	
	// FIXME: is the timer type correct for this sensor? 
	sys_timer_start(0, CALLIBRATE_MAX_TIME, TIMER_ONE_SHOT);
	
	return SOS_OK;
}


static int8_t magnet_control(func_cb_ptr cb, uint8_t cmd, void* data) {

	uint8_t ctx = *(uint8_t*)data;
	
	switch (cmd) {
	case SENSOR_GET_DATA_CMD:
		// get ready to read accel sensor
		if ((ctx & 0xC0) == MAG_0_SENSOR_ID) {
			return sys_adc_proc_getData(MTS310_MAG_0_SID, MAG_0_SENSOR_ID);
		} else {
			return sys_adc_proc_getData(MTS310_MAG_1_SID, MAG_1_SENSOR_ID);
		}
		break;

	case SENSOR_ENABLE_CMD:
		magnet_on();
		break;

	case SENSOR_DISABLE_CMD:
		magnet_off();
		break;

	case SENSOR_CONFIG_CMD:
		// no configuation
		if (data != NULL) {
			sys_free(data);
		}

		// POSSIBLE ERROR IN sensor.c,  line 234
		LED_DBG(LED_RED_TOGGLE);

		// callibrate sensor
		return mag_callibrate(ctx);
	
		break;

	default:
		return -EINVAL;
		break;
	}
	return SOS_OK;
}


int8_t magnet_msg_handler(void *state, Message *msg)
{
	
	magnet_sensor_state_t *s = (magnet_sensor_state_t*)state;
  
	switch (msg->type) {

	case MSG_INIT:
		// bind adc channel and register callback pointer

		sys_adc_proc_bindPort(MTS310_MAG_0_SID, MTS310_MAG_0_HW_CH, MAG_SENSOR_PID, SENSOR_DATA_READY_FID);
		sys_adc_proc_bindPort(MTS310_MAG_1_SID, MTS310_MAG_1_HW_CH, MAG_SENSOR_PID, SENSOR_DATA_READY_FID);
		// register with kernel sensor interface
		s->magnet_0_state = MAG_0_SENSOR_ID;
		sys_sensor_register(MAG_SENSOR_PID, MTS310_MAG_0_SID, SENSOR_CONTROL_FID, (void*)(&s->magnet_0_state));
		s->magnet_1_state = MAG_1_SENSOR_ID;
		sys_sensor_register(MAG_SENSOR_PID, MTS310_MAG_1_SID, SENSOR_CONTROL_FID, (void*)(&s->magnet_1_state));

		LED_DBG(LED_YELLOW_OFF);
		LED_DBG(LED_GREEN_OFF);
		LED_DBG(LED_RED_OFF);
		LED_DBG(LED_RED_ON);

		s->callibrate_mode = 0;

		break;

	case MSG_FINAL:
		// shutdown sensor
		magnet_off();
		//  unregister ADC port
		sys_adc_proc_unbindPort(MAG_SENSOR_PID, MTS310_MAG_0_SID);
		sys_adc_proc_unbindPort(MAG_SENSOR_PID, MTS310_MAG_1_SID);
		// unregister sensor
	  sys_sensor_deregister(MAG_SENSOR_PID, MTS310_MAG_0_SID);
		sys_sensor_deregister(MAG_SENSOR_PID, MTS310_MAG_1_SID);
		break;

	case MSG_I2C_SEND_DONE:


		sys_free(s->i2c_data_ptr);

		sys_i2c_release_bus(MAG_SENSOR_PID);
			
		if(s->callibrate_timeout == 0) {// only continue getting data if we have not reached to callibration timeout
			// FIXME: again, is this the right timer type?
			sys_timer_start(1, 50, TIMER_ONE_SHOT);
		}
		else {
			s->callibrate_mode = 0;
		}
		break;
		
	case MSG_TIMER_TIMEOUT:
	
		switch (*(msg->data)) {
		case 0: // we have reached the max callibration time
			s->callibrate_timeout = 1;
			break;
			
		case 1:
			if (s->current_axis == 0)
				sys_adc_proc_getData(MTS310_MAG_0_SID, MAG_0_SENSOR_ID);
			else
				sys_adc_proc_getData(MTS310_MAG_1_SID, MAG_1_SENSOR_ID);
			
			break;
			
		default:
			return -EINVAL;
			break;
		}
		
		break;
		
	default:
		return -EINVAL;
		break;
	}
	return SOS_OK;
}


#ifndef _MODULE_
mod_header_ptr mag_sensor_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

