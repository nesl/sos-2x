/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief ADC module for the ADS8341 on the protosb platform
 * @author Naim Busek <ndbusek@gmail.com>
 */

#include <hardware.h>
#include <sos_info.h>

#include <ads8341_adc_hal.h>

int8_t ads8341_adc_hardware_init(void) {
	
	ads8341_adc_disable_interrupt();
	ads8341_adc_clk_disable_interrupt();
	TCCR2 = (1<<WGM21);		// Clear Timer on Compare (CTC)
	TCNT2 = 0x00;					// zero counter
	
	ads8341_adc_off();
	SET_ADC_CS(); // !CS active low so we default to not selected

	SET_ADC_SHDN_DD_OUT();  // set up CS and SHDN as outputs
	SET_ADC_CS_DD_OUT();

	SET_ADC_BUSY_DD_IN();
	ads8341_adc_config_interrupt();  // initalize to trigger on a leading edge

	return SOS_OK;
}

