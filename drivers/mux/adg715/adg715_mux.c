/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief ADC module for the ADS8341
 * @author Naim Busek <ndbusek@gmail.com>
 */

/** 
 * Module needs to include <module.h>
 */
#include <sos.h>
#include <hardware.h>

#include "adg715_mux.h"

#ifndef ADG715_MUX_ADDR
#error Platform MUST define ADG715_MUX_ADDR
#endif

enum {
	ADG715_MUX_INIT=0,
	ADG715_MUX_IDLE,
	ADG715_MUX_BUSY_WR,
	ADG715_MUX_BUSY_RD,
	ADG715_MUX_ERROR,
};

typedef struct adg715_mux_state {
	uint8_t state;
	uint8_t calling_mod_id;
	uint8_t cmd;  // also current adg715_mux state
	uint8_t flags;
} adg715_mux_state_t;

static adg715_mux_state_t s;

static int8_t adg715_mux_msg_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = ADG715_MUX_PID,
	.state_size     = 0,
	.num_timers     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = adg715_mux_msg_handler,
};

int8_t adg715_mux_init() {
	HAS_CRITICAL_SECTION;
	
	ENTER_CRITICAL_SECTION();
	s.state = ADG715_MUX_INIT;
	s.flags = 0;
	LEAVE_CRITICAL_SECTION();

	ker_register_module(sos_get_header_address(mod_header));
	
	return SOS_OK;
}

int8_t adg715_mux_msg_handler(void *state, Message *msg) {
	HAS_CRITICAL_SECTION;

	switch (msg->type){
		case MSG_INIT:
			s.cmd=ADG715_MUX_OFF;
			s.calling_mod_id = NULL_PID;
			s.state = ADG715_MUX_IDLE;
			break;

		case MSG_FINAL:
			s.cmd=ADG715_MUX_OFF;
			ker_adg715_set_mux(ADG715_MUX_PID, s.cmd);
			break;

		case MSG_I2C_SEND_DONE:
			ker_i2c_release_bus(ADG715_MUX_PID);
			ENTER_CRITICAL_SECTION();
			if ((s.calling_mod_id != NULL_PID) && (s.calling_mod_id != ADG715_MUX_PID)) {
				post_short(s.calling_mod_id, ADG715_MUX_PID, MSG_MUX_SET_DONE, 0, 0,  SOS_MSG_HIGH_PRIORITY);
			}
			s.calling_mod_id = NULL_PID;
			s.state = ADG715_MUX_IDLE;
			LEAVE_CRITICAL_SECTION();
			break;

		case MSG_I2C_READ_DONE:
			{
				ker_i2c_release_bus(ADG715_MUX_PID);
				ENTER_CRITICAL_SECTION();
				if ((s.calling_mod_id != NULL_PID) && (s.calling_mod_id != ADG715_MUX_PID)) {
					post_short(s.calling_mod_id, ADG715_MUX_PID, MSG_MUX_READ_DONE, msg->data[0], 0,  SOS_MSG_HIGH_PRIORITY);
				}
				s.calling_mod_id = NULL_PID;
				s.state = ADG715_MUX_IDLE;
				LEAVE_CRITICAL_SECTION();
				ker_free(msg->data);
			}
			break;

		case MSG_ERROR:
			ker_i2c_release_bus(ADG715_MUX_PID);
			s.calling_mod_id = NULL_PID;
			s.state = ADG715_MUX_IDLE;
			s.cmd = ADG715_MUX_OFF;
			if (s.calling_mod_id != NULL_PID) {
				post_short(s.calling_mod_id, ADG715_MUX_PID, MSG_ERROR, 0, 0,  SOS_MSG_HIGH_PRIORITY);
			}
			break;

		default:
			return -EINVAL;
	}

	return SOS_OK;
}

int8_t ker_adg715_set_mux(uint8_t calling_id, uint8_t channel) {
	HAS_CRITICAL_SECTION;

	if ((calling_id != ADG715_MUX_PID) && (s.state != ADG715_MUX_IDLE)){
		return -EBUSY;
	}

	if (ker_i2c_reserve_bus(ADG715_MUX_PID, I2C_ADDRESS, I2C_SYS_MASTER_FLAG|I2C_SYS_TX_FLAG) != SOS_OK) {
		ker_i2c_release_bus(ADG715_MUX_PID);
		return -EBUSY;
	}
	
	ENTER_CRITICAL_SECTION();
	s.state = ADG715_MUX_BUSY_WR;
	switch (channel) {
		case ADG715_MUX_CH0:
		case ADG715_MUX_CH1:
		case ADG715_MUX_CH2:
		case ADG715_MUX_CH3:
			s.cmd = channel;
			break;									 
		default:
			s.cmd = ADG715_MUX_OFF;
			break;
	}
	s.calling_mod_id = calling_id;
	LEAVE_CRITICAL_SECTION();

	if (ker_i2c_send_data(ADG715_I2C_ADDR_BASE|ADG715_MUX_ADDR, &s.cmd, ADG715_WR_LEN, ADG715_MUX_PID) != SOS_OK) {
		ker_i2c_release_bus(ADG715_MUX_PID);
		return -EBUSY;
	}

	return SOS_OK;
}

int8_t ker_adg715_read_mux(uint8_t calling_id) {
	HAS_CRITICAL_SECTION;

	if ((calling_id != ADG715_MUX_PID) && (s.state != ADG715_MUX_IDLE)){
		return -EBUSY;
	}

	if (ker_i2c_reserve_bus(ADG715_MUX_PID, I2C_ADDRESS, I2C_SYS_MASTER_FLAG) != SOS_OK) {
		ker_i2c_release_bus(ADG715_MUX_PID);
		return -EBUSY;
	}
	
	ENTER_CRITICAL_SECTION();
	s.state = ADG715_MUX_BUSY_RD;
	s.calling_mod_id = calling_id;
	LEAVE_CRITICAL_SECTION();

	if (ker_i2c_read_data(ADG715_I2C_ADDR_BASE|ADG715_MUX_ADDR, ADG715_RD_LEN, ADG715_MUX_PID) != SOS_OK) {
		ker_i2c_release_bus(ADG715_MUX_PID);
		return -EBUSY;
	}

	return SOS_OK;
}
