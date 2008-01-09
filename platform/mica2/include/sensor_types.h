#ifndef _SENSOR_TYPES_H_
#define _SENSOR_TYPES_H_

typedef uint8_t sensor_id_t;

// Add new sensor ID's here when you add drivers for them.
enum {
	LIGHT_AMBIENT_SENSOR    = 0,
	LIGHT_PAR_SENSOR,
	HUMIDITY_SENSOR,
	TEMPERATURE_SENSOR,
	INTERNAL_TEMPERATURE_SENSOR,
	ACCEL_X_SENSOR,
	ACCEL_Y_SENSOR,
};
// Update this number if the total number of sensors
// increases beyond this current limit.
#define MAX_NUM_SENSORS   8

#endif

