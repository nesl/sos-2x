#ifndef _SHT1x_H_
#define _SHT1x_H_

#include <sht1x_plat.h>

#define SHT1x_TEMPERATURE_CH		0x01
#define SHT1x_HUMIDITY_CH			0x02

#define SHT1x_NUM_SENSORS			2

// Command sent by sensor driver to sht1x driver.
typedef enum {
	SHT1x_REGISTER_REQUEST,
	SHT1x_REMOVE_REQUEST,
	SHT1x_GET_DATA,
} sht1x_sensor_command_t;

// Feedback from sht1x driver to sensor driver.
typedef enum {
    SHT1x_SENSOR_SEND_DATA    = 0,
	SHT1x_SENSOR_CHANNEL_UNBOUND,
	SHT1x_SENSOR_SAMPLING_DONE,
	SHT1x_SENSOR_ERROR,
} sht1x_feedback_t;

enum {
	SHT1x_DATA_READY_FID	= 1,
	SHT1x_FEEDBACK_FID		= 2,
	SHT1x_GET_DATA_FID		= 3,
	SHT1x_STOP_DATA_FID		= 4,
};

#endif

