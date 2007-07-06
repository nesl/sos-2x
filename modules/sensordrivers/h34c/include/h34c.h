/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */


#ifndef _H34C_H_
#define _H34C_H_

#ifndef PC_PLATFORM
#include <adc_proc_common.h>
#include <pin_map.h>
#endif

// sensorboard sensor types
enum {
	H34C_ACCEL_0_SID=1,
	H34C_ACCEL_1_SID,
	H34C_ACCEL_2_SID,
};

#define H34C_ACCEL_0_TYPE ACCELEROMETER_SENSOR
#define H34C_ACCEL_1_TYPE ACCELEROMETER_SENSOR
#define H34C_ACCEL_2_TYPE ACCELEROMETER_SENSOR

// add port mapping for h34c
#define H34C_ACCEL_0_HW_CH ADC_PROC_CH1
#define H34C_ACCEL_1_HW_CH ADC_PROC_CH2
#define H34C_ACCEL_2_HW_CH ADC_PROC_CH3

#ifndef PC_PLATFORM
ALIAS_IO_PIN(ACCEL_EN, PW0);
#endif

#endif // _H34C_H_

