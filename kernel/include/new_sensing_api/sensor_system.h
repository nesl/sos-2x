#ifndef _SENSOR_SYSTEM_H_
#define _SENSOR_SYSTEM_H_

// This file is included by sos.h (and hence, module.h) 
// and sys_module.h .
// Thus it is included in ALL components of the 
// sampling sub-system i.e. hardware drivers, sensor
// drivers, sensor API and application modules.

#include <sos_types.h>

// Universal sensor ID's
// Starts with common sensors found across all platforms.
// Add new sensor ID's here when you add drivers for them.
typedef enum {
	LIGHT_AMBIENT_SENSOR    = 0,
	LIGHT_PAR_SENSOR,
	HUMIDITY_SENSOR,
	TEMPERATURE_SENSOR,
	INTERNAL_TEMPERATURE_SENSOR,
	ACCEL_X_SENSOR,
	ACCEL_Y_SENSOR,
	MIC_SENSOR,
	MAG_X_SENSOR,
	MAG_Y_SENSOR,
    ACCEL_SENSOR,
} sensor_id_t;

// Update this number if the total number of sensors
// increases beyond this current limit.
#define MAX_NUM_SENSORS   8

// Sensor status indicated by sensor driver.
typedef enum {
	SENSOR_DATA					= 0,
	SENSOR_DRIVER_UNREGISTERED,
	SENSOR_SAMPLING_STOPPED,
	SENSOR_SAMPLING_ERROR,
	SENSOR_STATUS_UNKNOWN		= 0xFF,
} sensor_status_t;

// Sensor data is wrapped in this structure
// to provide additional meta-data to the application.
typedef struct {
	sensor_status_t status;
	sensor_id_t sensor;
	unsigned int num_samples;
	uint16_t buf[];
} sensor_data_msg_t;

// Sensing parameters are set by the application
// using this sturcture.
typedef struct {
	uint32_t delay;
	uint32_t period;
	uint16_t samples;
	uint16_t event_samples;
} sample_context_t;

// Control interface for interaction between sensor API
// and sensor drivers, and hardware drivers and sensor drivers.
// Application modules do not need to bother with this.
typedef enum {
	SENSOR_REGISTER_REQUEST_COMMAND	= 0,
	SENSOR_REMOVE_REQUEST_COMMAND,
	SENSOR_GET_DATA_COMMAND,
	SENSOR_STOP_DATA_COMMAND,
	SENSOR_ENABLE_COMMAND,
	SENSOR_DISABLE_COMMAND,
} sensor_driver_command_t;

#endif

