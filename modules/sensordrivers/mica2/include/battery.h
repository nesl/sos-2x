/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */


#ifndef _MICA2_SENSOR_
#define _MICA2_SENSOR_

#ifndef PC_PLATFORM
#include <adc_proc_common.h>
#include <pin_map.h>
#endif

// sensorboard sensor types
enum {
	MICA2_BATTERY_SID=1,
};

#define MICA2_BATTERY_TYPE VOLTAGE_SENSOR

// add port mapping for h34c
#define MICA2_BATTERY_CH ADC_PROC_CH7

// BAT_MON is defined in platform/mica2/include/pin_map.h as:
// ALIAS_IO_PIN( BAT_MON, PINA5);

#endif // _MICA2_SENSOR_

