/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */


#ifndef _TMOTE_SENSOR_H_
#define _TMOTE_SENSOR_H_

#ifndef PC_PLATFORM
#include <hardware.h>
#include <pin_map.h>
#include <msp430/adc12.h>
#endif

// sensorboard sensor types
enum {
	TEMP_SID= 10,
    PAR_SID = 4,
    TSR_SID = 5,
    VCC_SID = 11,
};

#define TEMP_TYPE TEMPERATURE_SENSOR
#define PAR_TYPE LIGHT_SENSOR
#define TSR_TYPE LIGHT_SENSOR
#define VCC_TYPE VOLTAGE_SENSOR

// Port mapping for Tmote.
/*
 * Do not use INCH_A* defines as they work only
 * on Linux, and not on Mac OS.
 * #define TEMP_HW_CH INCH_TEMP
 * #define PAR_HW_CH INCH_A4
 * #define TSR_HW_CH INCH_A5
 * #define VCC_HW_CH INCH_VCC2
*/
#define TEMP_HW_CH INCH_10
#define PAR_HW_CH INCH_4
#define TSR_HW_CH INCH_5
#define VCC_HW_CH INCH_11

#endif // _TMOTE_SENSOR_H_

