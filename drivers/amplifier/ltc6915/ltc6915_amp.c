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

#include "ltc6915.h"
#include "ltc6915_amp.h"

// set the default of off (gain 0)
#define DEFAULT_GAIN LTC6915_GAIN_0

#define WRITE_LEN 1

/**
 *  * Varrious states that the module transitions through
 *   */
enum {
	LTC6915_AMP_INIT=0,
	LTC6915_AMP_INIT_BUSY,
	LTC6915_AMP_BACKOFF,
	LTC6915_AMP_WAIT,
	LTC6915_AMP_IDLE,
	LTC6915_AMP_BUSY,
	LTC6915_AMP_ERROR,
};


/**
 * Module state
 */
typedef struct ltc6915_amp_state {
	uint8_t state;
	uint8_t calling_mod_id;
	spi_addr_t addr;
	uint8_t gain;
	uint8_t flags;
} ltc6915_amp_state_t;


static ltc6915_amp_state_t s;

static int8_t ltc6915_amp_msg_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = LTC6915_AMP_PID,
	.state_size     = 0,
	.num_timers     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = ltc6915_amp_msg_handler,
};


int8_t ltc6915_amp_init() {
	HAS_CRITICAL_SECTION;

	ltc6915_amp_hardware_init();

	ENTER_CRITICAL_SECTION();
	s.state = LTC6915_AMP_INIT;
	s.calling_mod_id = NULL_PID;
	s.gain = DEFAULT_GAIN;
	s.addr = ltc6915_amp_hal_get_addr();
	ker_register_module(sos_get_header_address(mod_header));
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


static int8_t ltc6915_amp_msg_handler(void *state, Message *msg)
{ 
	HAS_CRITICAL_SECTION;
	
	switch (msg->type) {
		case MSG_INIT:
			ltc6915_amp_hal_on();

			if (ker_ltc6915_amp_setGain(LTC6915_AMP_PID, DEFAULT_GAIN) != SOS_OK) {
				s.state = LTC6915_AMP_BACKOFF;
				post_short(LTC6915_AMP_PID, LTC6915_AMP_PID, MSG_INIT, 0, 0,  SOS_MSG_SYSTEM_PRIORITY);
				return -EBUSY;
			}
			s.state = LTC6915_AMP_INIT_BUSY;
			break;

		case MSG_FINAL:
			ltc6915_amp_hal_off();
			break;

		case MSG_SPI_SEND_DONE:
			ENTER_CRITICAL_SECTION();
			if ((s.state != LTC6915_AMP_INIT_BUSY) && (s.calling_mod_id != NULL_PID)) {
				post_short(s.calling_mod_id, LTC6915_AMP_PID, MSG_VAR_PREAMP_DONE, 0, 0,  SOS_MSG_HIGH_PRIORITY);
			}
			s.state = LTC6915_AMP_IDLE;
			s.calling_mod_id = NULL_PID;
			LEAVE_CRITICAL_SECTION();
			break;

		case MSG_SPI_READ_DONE:
		case MSG_ERROR:
			ENTER_CRITICAL_SECTION();
			s.state = LTC6915_AMP_ERROR;
			s.gain = DEFAULT_GAIN;
			LEAVE_CRITICAL_SECTION();
			break;

		case MSG_VAR_PREAMP_DONE:
			ENTER_CRITICAL_SECTION();
			s.state = LTC6915_AMP_IDLE;
			LEAVE_CRITICAL_SECTION();
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}


int8_t ker_ltc6915_amp_setGain(uint8_t calling_id, uint8_t gain){
	HAS_CRITICAL_SECTION;

	if ((calling_id != LTC6915_AMP_PID) && (s.state != LTC6915_AMP_IDLE) && (s.state != LTC6915_AMP_BACKOFF)) {
		return -EBUSY;
	}

	s.gain = gain & 0x0f;

	ENTER_CRITICAL_SECTION();
	if(ker_spi_reserve_bus(LTC6915_AMP_PID, s.addr, SPI_SYS_NULL_FLAG) != SOS_OK) {
		ENTER_CRITICAL_SECTION();
		ker_spi_release_bus(LTC6915_AMP_PID);
		s.state = LTC6915_AMP_ERROR;
		LEAVE_CRITICAL_SECTION();
		return -EBUSY;
	}

	if (ker_spi_send_data( &(s.gain), WRITE_LEN, LTC6915_AMP_PID) != SOS_OK) {
		ENTER_CRITICAL_SECTION();
		ker_spi_release_bus(LTC6915_AMP_PID);
		s.state = LTC6915_AMP_ERROR;
		LEAVE_CRITICAL_SECTION();
		return -EBUSY;
	}
	s.calling_mod_id = calling_id;
	if (s.state != LTC6915_AMP_INIT_BUSY) {
		s.state = LTC6915_AMP_BUSY;
	}
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}

