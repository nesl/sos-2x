/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief controling variable gain preamp
 * @author Naim Busek {ndbusek@gmail.com
 */

/** 
 * Module needs to include <module.h>
 */
#include <sos.h>
#include <hardware.h>
#include <sos_timer.h>

#include "var_preamp.h"

#define WRITE_LEN 1


/**
 *  * Varrious states that the module transitions through
 *   */
enum {
	VAR_PREAMP_INIT=0,
	VAR_PREAMP_INIT_BUSY,
	VAR_PREAMP_BACKOFF,
	VAR_PREAMP_WAIT,
	VAR_PREAMP_IDLE,
	VAR_PREAMP_BUSY,
	VAR_PREAMP_ERROR,
};


/**
 * Module state
 */
typedef struct var_preamp_state {
	uint8_t state;
	uint8_t calling_mod_id;
	uint8_t gain;
	uint8_t flags;
} var_preamp_state_t;


static var_preamp_state_t s;
static spi_addr_t addr;

#define DEFAULT_GAIN GAIN_0

static int8_t var_preamp_msg_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = VAR_PREAMP_PID,
	.state_size     = 0,
	.num_timers     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = var_preamp_msg_handler,
};

int8_t var_preamp_hardware_init() {
	var_preamp_off();					// shutdown amplifier (active high)
	SET_AMP_CS();						// CS active low so we default to not selected

	SET_AMP_SHDN_DD_OUT();	// set up CS and SHDN as outputs
	SET_AMP_CS_DD_OUT();

	return SOS_OK;
}


int8_t var_preamp_init() {
	HAS_CRITICAL_SECTION;

	addr.cs_reg = AMP_CS_PORT();
	addr.cs_bit = AMP_CS_BIT();
	
	ENTER_CRITICAL_SECTION();
	s.state = VAR_PREAMP_INIT;
	s.calling_mod_id = NULL_PID;
	s.gain = DEFAULT_GAIN;
	ker_register_module(sos_get_header_address(mod_header));
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


static int8_t var_preamp_msg_handler(void *state, Message *msg)
{ 
	HAS_CRITICAL_SECTION;
	
	switch (msg->type) {
		case MSG_INIT:
			var_preamp_on();					// shutdown amplifier (active high)
			if (ker_var_preamp_setGain(VAR_PREAMP_PID, DEFAULT_GAIN) != SOS_OK) {
				s.state = VAR_PREAMP_BACKOFF;
				post_short(VAR_PREAMP_PID, VAR_PREAMP_PID, MSG_INIT, 0, 0,  SOS_MSG_SYSTEM_PRIORITY);
				break;
			}
			s.state = VAR_PREAMP_INIT_BUSY;
			break;

		case MSG_FINAL:
			var_preamp_on();					// shutdown amplifier (active high)
			break;

		case MSG_SPI_SEND_DONE:
			ENTER_CRITICAL_SECTION();
			if ((s.state != VAR_PREAMP_INIT_BUSY) && (s.calling_mod_id != NULL_PID)) {
				post_short(s.calling_mod_id, VAR_PREAMP_PID, MSG_VAR_PREAMP_DONE, 0, 0,  SOS_MSG_HIGH_PRIORITY);
			}
			s.state = VAR_PREAMP_IDLE;
			s.calling_mod_id = NULL_PID;
			LEAVE_CRITICAL_SECTION();
			break;

		case MSG_SPI_READ_DONE:
		case MSG_ERROR:
			ENTER_CRITICAL_SECTION();
			s.state = VAR_PREAMP_ERROR;
			s.gain = DEFAULT_GAIN;
			LEAVE_CRITICAL_SECTION();
			break;

		case MSG_VAR_PREAMP_DONE:
			ENTER_CRITICAL_SECTION();
			s.state = VAR_PREAMP_IDLE;
			LEAVE_CRITICAL_SECTION();
			break;

		default:
			return -EINVAL;
	}
	return SOS_OK;
}


int8_t ker_var_preamp_setGain(uint8_t calling_id, uint8_t gain){
	HAS_CRITICAL_SECTION;

	if ((calling_id != VAR_PREAMP_PID) && (s.state != VAR_PREAMP_IDLE) && (s.state != VAR_PREAMP_BACKOFF)) {
		return -EBUSY;
	}

	s.gain = gain & 0x0f;

	ENTER_CRITICAL_SECTION();
	if(ker_spi_reserve_bus(VAR_PREAMP_PID, addr, SPI_SYS_NULL_FLAG) != SOS_OK) {
		ENTER_CRITICAL_SECTION();
		ker_spi_release_bus(VAR_PREAMP_PID);
		s.state = VAR_PREAMP_ERROR;
		LEAVE_CRITICAL_SECTION();
		return -EBUSY;
	}

	if (ker_spi_send_data( &(s.gain), WRITE_LEN, 0, VAR_PREAMP_PID) != SOS_OK) {
		ENTER_CRITICAL_SECTION();
		ker_spi_release_bus(VAR_PREAMP_PID);
		s.state = VAR_PREAMP_ERROR;
		LEAVE_CRITICAL_SECTION();
		return -EBUSY;
	}
	s.calling_mod_id = calling_id;
	if (s.state != VAR_PREAMP_INIT_BUSY) {
		s.state = VAR_PREAMP_BUSY;
	}
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}

