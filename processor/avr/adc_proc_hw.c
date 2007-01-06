/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

//-----------------------------------------------------------------------------
// INCLUDES
#include <adc_proc_hw.h>

// set to external AREF as hardware default
#define DEFAULT_VREF ADC_PROC_REF_AREF
// set GND as default input
#define DEFAULT_MUX_CH ADC_PROC_GND
// Set Prescaler division factor to 64 
#define DEFAULT_PRESCALER ADC_PROC_CLK_64

//-----------------------------------------------------------------------------
// INITIALIZE ADC hardware
int8_t adc_proc_hardware_init() {
  HAS_CRITICAL_SECTION;

  ENTER_CRITICAL_SECTION();
	ADMUX = (DEFAULT_VREF | DEFAULT_MUX_CH);

	// disable ADC, clear any pending interrupts and enable ADC
	ADCSR &= ~_BV(ADEN);
	ADCSR |= DEFAULT_PRESCALER;
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}

