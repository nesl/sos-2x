#ifndef _SENSOR_DRIVER_PRIVATE_H_
#define _SENSOR_DRIVER_PRIVATE_H_

// This file is private to sensor driver modules only.
// It should be included by ALL sensor driver modules
// irrespective of whether they are implemented as
// loadable modules or as kernel extensions.

#include <sensor_system.h>

// Function ID's for control and data_ready
// callback functions published by sensor drivers.
enum {
	SENSOR_CONTROL_FID      = 0,
	SENSOR_DATA_READY_FID,
	SENSOR_FEEDBACK_FID,
};

// Internal sensor driver state
typedef enum {
	DRIVER_ENABLE	= 0,
	DRIVER_DISABLE,
	DRIVER_ERROR,
} sensor_driver_state_t;

// Sensor ID to underlying hardware channel mapping
typedef struct sensor_channel_map_t {
	sensor_id_t sensor;
	uint16_t channel;
} sensor_channel_map_t;

static inline uint16_t get_channel(sensor_id_t sensor, sensor_channel_map_t map[], 
					unsigned int len) {
	unsigned int i = 0;
	for (; i < len; i++) {
		if (map[i].sensor == sensor) return map[i].channel;
	}

	return 0;
}

static inline uint16_t get_all_channels(sensor_channel_map_t map[], unsigned int len) {
	unsigned int i = 0;
	uint16_t channels = 0;

	for (; i < len; i++) {
		channels |= map[i].channel;
	}

	return channels;
}

static inline sensor_id_t get_sensor(uint16_t channel, sensor_channel_map_t map[], 
					unsigned int len) {
	unsigned int i = 0;
	for (; i < len; i++) {
		if (map[i].channel & channel) return map[i].sensor;
	}

	return MAX_NUM_SENSORS;
}

#endif

