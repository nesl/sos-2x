#ifndef __adc_proc_common_h_
#define __adc_proc_common_h_

#include <adc_proc_hw.h>

// 8 ports for the user 2 for the system
#define ADC_PROC_PORTMAPSIZE 8
#define ADC_PROC_EXTENDED_PORTMAPSIZE 10

enum {
	ADC_PROC_PORT0=0,
	ADC_PROC_PORT1,
	ADC_PROC_PORT2,
	ADC_PROC_PORT3,
	ADC_PROC_PORT4,
	ADC_PROC_PORT5,
	ADC_PROC_PORT6,
	ADC_PROC_PORT7,
	// system calibration ports
	ADC_PROC_SYS_PORT8,
	ADC_PROC_SYS_PORT9,
};

#endif

