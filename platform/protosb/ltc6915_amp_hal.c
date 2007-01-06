/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief controling variable gain preamp
 * @author Naim Busek {ndbusek@gmail.com
 */

#include <sos_info.h>
#include <hardware.h>
		     
#include "ltc6915_amp_hal.h"

int8_t ltc6915_amp_hardware_init() {

	ltc6915_amp_hal_off();	// shutdown amplifier (active high)
	SET_AMP_CS();						// CS active low so we default to not selected

	SET_AMP_SHDN_DD_OUT();	// set up CS and SHDN as outputs
	SET_AMP_CS_DD_OUT();
	
	return SOS_OK;
}

