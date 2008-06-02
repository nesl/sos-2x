#ifndef _SENSOR_TYPES_H_
#define _SENSOR_TYPES_H_

typedef uint8_t sensor_id_t;

#ifdef TMOTE_INVENT_SENSOR_BOARD
// Sensor ID's for Tmote Invent platform and sensor board.
// Add new sensor ID's here when you add drivers for them.
enum {
	LIGHT_AMBIENT_SENSOR    = 0,
	ACCEL_X_SENSOR,
	ACCEL_Y_SENSOR,
	MIC_SENSOR,
};
// Update this number if the total number of sensors
// increases beyond this current limit.
#define MAX_NUM_SENSORS   4

#else 

#ifdef TMOTE_IMPACT_SENSOR_BOARD
// Sensor ID's for Tmote Sky platform and custom ADXL321 sensor
// board for measuring Impact Shock.
// Add new sensor ID's here when you add drivers for them.
enum {
	ACCEL_X_SENSOR	= 0,
	ACCEL_Y_SENSOR,
	ACCEL_X_FILTER,
	ACCEL_Y_FILTER,
};
// Update this number if the total number of sensors
// increases beyond this current limit.
#define MAX_NUM_SENSORS   4

#else
// Sensor ID's for Tmote Sky platform with optional sensors.
// Add new sensor ID's here when you add drivers for them.
enum {
	LIGHT_AMBIENT_SENSOR    = 0,
	LIGHT_PAR_SENSOR,
	HUMIDITY_SENSOR,
	TEMPERATURE_SENSOR,
	INTERNAL_TEMPERATURE_SENSOR,
	ACCEL_X_SENSOR,
	ACCEL_Y_SENSOR,
	INTERNAL_VOLTAGE_SENSOR,
};
// Update this number if the total number of sensors
// increases beyond this current limit.
#define MAX_NUM_SENSORS   8

#endif
#endif


#endif

