/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @brief example hardware abstraction layer for the ADS8341
 * @author Naim Busek <ndbusek@gmail.com>
 */

/**
 * to use this component your platform must define the platform specific
 * hardware initalization functions
 *
 * this is done by creating a ads8341_adc_hal.c file in your platform
 * directory that defines the following functions
 *
 * the following is an example from the protosb platform
 * 
 * please use the values defined in the device header file when defining
 * these values
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

