#ifndef _SHT1x_COMM_PRIVATE_H_
#define _SHT1x_COMM_PRIVATE_H_

//#include <bitsop.h>
#include <proc_msg_types.h>
#include "sht1x.h"

////
// Hardware specific defines
//
// These may be moved to a driver file later on in the future.
////
//                                   adr command  r/w
#define STATUS_REG_WRITE    0x06  // 000   0011    0
#define STATUS_REG_READ     0x07  // 000   0011    1
#define MEASURE_TEMPERATURE 0x03  // 000   0001    1
#define MEASURE_HUMIDITY    0x05  // 000   0010    1
#define RESET               0x1e  // 000   1111    0

#define SHT1x_DELAY_TIME	20

////
// Local enums
////

/**
 * Type of reading that is requisted from the SHT11
 */
typedef enum
{
    TEMPERATURE, 
    HUMIDITY,
} sht1x_command_t;


/**
 * Ack flag
 */
typedef enum
{
    ACK,
    NOACK,
} sht1x_ack_t;


/**
 * Local timer
 */
enum
{
    SHT1x_TEMPERATURE_TIMER,
    SHT1x_HUMIDITY_TIMER,
    SHT1x_DELAY_TIMER,
    SHT1x_SAMPLE_TIMER,
};

enum {
	SHT1x_COMM_INIT       = 0x01,     // system uninitalized
    SHT1x_COMM_IDLE       = 0x02,     // system initalized and idle
	SHT1x_COMM_BUSY       = 0x03,
	SHT1x_COMM_ERROR      = 0x04,     // error state
	SHT1x_COMM_HW_ON      = 0x80,     // ADC is ON
};

// Status for each sensor data request from application.
typedef enum {
    REQUEST_INIT,
	REQUEST_REGISTERED,
	REQUEST_DELAY,
	REQUEST_ACTIVE,
	REQUEST_NEXT_ITERATION,
	REQUEST_LAST_EVENT,
	REQUEST_COMPLETE,
} request_status_t;


// Information related to request for sensor data.
typedef struct data_request_t {
	sos_pid_t app_id;
	uint16_t channels;
	uint32_t delay;
	uint32_t period;
	uint16_t samples;
	uint16_t event_samples;
	request_status_t status;
	void *sensor_context;
	struct data_request_t *next;
} data_request_t;

// FIFO data request queue with a pointer for
// head and tail.
typedef struct {
	data_request_t *head, *tail;
} request_queue_t;


typedef int8_t (*sensor_data_ready_func_t)(func_cb_ptr cb, sht1x_feedback_t fb, sos_pid_t app_id,
                            uint16_t channels, sensor_data_msg_t* buf);
typedef int8_t (*sensor_feedback_func_t)(func_cb_ptr cb, sensor_driver_command_t command,
                        uint16_t channel, void *context);

#endif

