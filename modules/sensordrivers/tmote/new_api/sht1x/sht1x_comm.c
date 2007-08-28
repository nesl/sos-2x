/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver Support for the SHT1x on TMote
 * 
 * This module currently provides support for measuring
 * temperature and humidity from the SHT1x chip.
 * 
 * \author Kapy Kangombe, John Hicks, James Segedy {jsegedy@gmail.com}
 * \date 07-2005
 * Ported driver from TinyOS to SOS
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \date 06-2006
 * \date 05-2007
 * Ported driver to current version of SOS
 *
 * \author Thomas Schmid (thomas.schmid@ucla.edu)
 * \date 07-2007
 * Ported driver from SHT1x to SHT1x
 *
 * \author Rahul Balani (rahulb@ee.ucla.edu)
 * \date 08-2007
 * Ported driver to new sensing subsystem
 */

#include <module.h>
#include "sht1x_comm.h"

#define LED_DEBUG
#include <led_dbg.h>

//-----------------------------------------------------------------------------
// LOCAL VARIABLES
//-----------------------------------------------------------------------------
typedef struct 
{
	func_cb_ptr sensor_data_ready;
	func_cb_ptr sensor_feedback;
	uint8_t state;
	request_queue_t request_queue;
	data_request_t *new_request, *current_request;
	// These variables are set according to current 
	// request when sampling is active.
	uint32_t current_sample_cnt, target_samples_in_iteration;
	uint16_t num_channels;
	uint8_t next_channel;
	uint16_t *buf;
} sht1x_comm_state_t;

static sht1x_comm_state_t s;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// INTERNAL FUNCTIONS
//-----------------------------------------------------------------------------
static void delay();
static int8_t write_byte(uint8_t byte);
static uint8_t read_byte(sht1x_ack_t ack);
static void start_transmission();
static void connection_reset();
static int8_t measure(uint8_t channel);
static bool get_data(uint16_t *data, uint8_t *checksum, sht1x_command_t command);
static int8_t task_next_sample();
static inline int8_t post_task_to_start_next_sampling();
static inline void reset_active_request_params();
static void serve_request();
static int8_t sht1x_start_sampling();
//static int8_t sht1x_stop_sampling();
static void process_sample_buffer(uint8_t ch);
static int8_t handle_data_ready(uint32_t num_samples);

static void request_enqueue();
static void request_remove(data_request_t *del);
static data_request_t* request_dequeue();

static int8_t driver_error(func_cb_ptr cb); 
static void reinitialize_comm();
static int8_t sht1x_get_data (func_cb_ptr cb, sht1x_sensor_command_t command, 
	sos_pid_t app_id, uint16_t channels, sample_context_t *param, void *context);
static int8_t sht1x_stop_data (func_cb_ptr cb, sos_pid_t app_id, uint16_t channels);
//-----------------------------------------------------------------------------


static int8_t sht1x_msg_handler(void *start, Message *e);
static sos_module_t sht1x_comm_module;

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = SHT1x_COMM_PID,
    .code_id         = ehtons(SHT1x_COMM_PID),
    .state_size     = 0,
    .num_sub_func   = 2,
    .num_prov_func  = 2,
    .module_handler = sht1x_msg_handler,
	.funct			= {
		{driver_error, "cCS3", SHT1x_SENSOR_PID, SHT1x_DATA_READY_FID},
		{driver_error, "cCw2", SHT1x_SENSOR_PID, SHT1x_FEEDBACK_FID},
		{sht1x_get_data, "cyy5", SHT1x_COMM_PID, SHT1x_GET_DATA_FID},
		{sht1x_stop_data, "cyS2", SHT1x_COMM_PID, SHT1x_STOP_DATA_FID},
	},
};

static void process_sample_buffer(uint8_t ch) {
	// Process current sample buffer to check if
	// current iteration is complete, start next iteration when required,
	// and post buffer to the application.
	s.next_channel = ch;
	if (s.current_sample_cnt == (s.target_samples_in_iteration * s.num_channels)) {
		uint32_t num_samples = s.current_sample_cnt;
		if (s.current_request->samples > 0) {
			s.current_request->samples -= s.target_samples_in_iteration;
			if (s.current_request->samples == 0) {
				// Stop sampling.
				connection_reset();
				// Mark it as last data packet so that hardware is reset
				// before next sampling begins.
				s.current_request->status = REQUEST_LAST_EVENT;
			} else {
				// More samples need to be collected.
				// Start next iteration.
				// Wait for user specified period before starting next measurement.
				if (ker_timer_start(SHT1x_COMM_PID, SHT1x_SAMPLE_TIMER, s.current_request->period) < 0) {
					// Error while starting measurement
					connection_reset();
					// Mark it as last data packet so that hardware is reset
					// before next sampling begins.
					s.current_request->status = REQUEST_LAST_EVENT;
				} else {
					// Reset current sample count and target samples in next
					// iteration.
					s.current_request->status = REQUEST_NEXT_ITERATION;
					s.target_samples_in_iteration = 
							(s.current_request->event_samples < s.current_request->samples) ?
							s.current_request->event_samples : s.current_request->samples;
				}
			}
		} else {
			// Continuous sampling is requested.
			// Start next iteration.
			// Wait for user specified period before starting next measurement.
			if (ker_timer_start(SHT1x_COMM_PID, SHT1x_SAMPLE_TIMER, s.current_request->period) < 0) {
				// Error while starting measurement
				connection_reset();
				// Mark it as last data packet so that hardware is reset
				// before next sampling begins.
				s.current_request->status = REQUEST_LAST_EVENT;
			} else {
				// Reset current sample count and target samples in next
				// iteration.
				s.target_samples_in_iteration = s.current_request->event_samples;
				s.current_request->status = REQUEST_NEXT_ITERATION;
			}
		}
		// Post the current obtained samples to application.
		handle_data_ready(num_samples);
	} else {
		// Start next measurement for same iteration
		// Wait for user specified period before starting next measurement.
		if (ker_timer_start(SHT1x_COMM_PID, SHT1x_SAMPLE_TIMER, s.current_request->period) < 0) {
			// Error while starting measurement
			connection_reset();
			// Mark it as last data packet so that hardware is reset
			// before next sampling begins.
			s.current_request->status = REQUEST_LAST_EVENT;
			// Post the current obtained samples to application.
			handle_data_ready(s.current_sample_cnt);
		}
	}
}

static int8_t handle_data_ready(uint32_t num_samples) {
	uint16_t *reading = NULL;
	uint16_t i;
	uint8_t ch;
	uint32_t j;

	// Ignore, if request is marked COMPLETE.
	if (s.current_request->status == REQUEST_COMPLETE) return -EINVAL;

	// Take action depending on number of channels sampled.
	if (s.num_channels == 1) {
		// Allocate buffer for the channel.
		sensor_data_msg_t* send_buf = (sensor_data_msg_t *)ker_malloc(
							sizeof(sensor_data_msg_t) + (num_samples*sizeof(uint16_t)), 
							SHT1x_COMM_PID);
		if (send_buf == NULL) goto handle_data_exit;
		// Copy samples into the send_buf.
		send_buf->status = SENSOR_DATA;
		send_buf->sensor = 0;
		send_buf->num_samples = num_samples;
		memcpy(send_buf->buf, s.buf, num_samples * sizeof(uint16_t));
		// Call data_ready event for the requested channel.
		if (SHT1x_TEMPERATURE_CH & s.current_request->channels) {
			ch = SHT1x_TEMPERATURE_CH;
		} else {
			ch = SHT1x_HUMIDITY_CH;
		}
		SOS_CALL(s.sensor_data_ready, sensor_data_ready_func_t, SHT1x_SENSOR_SEND_DATA, 
				s.current_request->app_id, ch, send_buf);
		// Check if ERROR flag was set. This happens when no sensor driver exists.
		if (s.state & SHT1x_COMM_ERROR) {
			ker_free(send_buf);
			reinitialize_comm();
			return SOS_OK;
		}
	} else {
		// Allocate a data buffer for each channel.
		// The pointers to these buffers are stored in send_buf_array.
		// For this driver, num_channels = SHT1x_NUM_SENSORS if code reaches here.
		sensor_data_msg_t *send_buf_array[SHT1x_NUM_SENSORS];
		num_samples /= SHT1x_NUM_SENSORS;
		for (i = 0; i < SHT1x_NUM_SENSORS; i++) {
			send_buf_array[i] = (sensor_data_msg_t *)ker_malloc(
							sizeof(sensor_data_msg_t) + (sizeof(uint16_t) * num_samples), 
							SHT1x_COMM_PID);
			if (send_buf_array[i] == NULL) {
				int16_t k;
				for (k = i - 1; k > -1; k--) {
						ker_free(send_buf_array[k]);
				}
				goto handle_data_exit;
			}
			send_buf_array[i]->status = SENSOR_DATA;
			send_buf_array[i]->sensor = 0;
			send_buf_array[i]->num_samples = num_samples;
		}
		// Move the samples from s.buf to above data buffers.
		reading = s.buf;
		for (j = 0; j < num_samples; j++) {
			ch = 0;
			send_buf_array[ch]->buf[j] = *reading;
			ch++;
			reading++;
			send_buf_array[ch]->buf[j] = *reading;
			reading++;
		}
		// Call data_ready event for each requested channel.
		SOS_CALL(s.sensor_data_ready, sensor_data_ready_func_t, SHT1x_SENSOR_SEND_DATA, 
				s.current_request->app_id, SHT1x_TEMPERATURE_CH, send_buf_array[0]);
		SOS_CALL(s.sensor_data_ready, sensor_data_ready_func_t, SHT1x_SENSOR_SEND_DATA, 
				s.current_request->app_id, SHT1x_HUMIDITY_CH, send_buf_array[1]);
		// Check if ERROR flag was set. This happens when no sensor driver exists.
		if (s.state & SHT1x_COMM_ERROR) {
			ker_free(send_buf_array[0]);
			ker_free(send_buf_array[1]);
			reinitialize_comm();
			return SOS_OK;
		}
	}

	// If request was marked LAST_EVENT, mark it COMPLETE and post task to
	// start next sampling.
handle_data_exit:
	if (s.current_request->status == REQUEST_LAST_EVENT) {
		s.current_request->status = REQUEST_COMPLETE;
		post_task_to_start_next_sampling();
	}
	return SOS_OK;
}

static int8_t sht1x_msg_handler(void *state, Message *msg) { 
    switch(msg->type)
    {
		case MSG_SHT1x_START_NEXT_SAMPLING: {
			if (s.state & SHT1x_COMM_ERROR) return -EINVAL;
			// Task to start next sampling from the request queue.
			return task_next_sample();
		}
        case MSG_TIMER_TIMEOUT: {
			uint8_t checksum;
			bool no_error;
			MsgParam *param = (MsgParam*) (msg->data);

			if (s.state & SHT1x_COMM_ERROR) return -EINVAL;

			if ( (s.current_request == NULL) ||
				 (s.current_request->status == REQUEST_COMPLETE) ||
				 (s.current_request->status == REQUEST_LAST_EVENT) ) {
					return -EINVAL;
			}
			switch (param->byte) {
				case SHT1x_DELAY_TIMER: {
					if (s.current_request->status != REQUEST_DELAY) {
						return -EINVAL;
					}
					serve_request();
					break;
				}
				case SHT1x_SAMPLE_TIMER: {
					// Verify current request status.
					if ( (s.current_request->status != REQUEST_ACTIVE) &&
						 (s.current_request->status != REQUEST_NEXT_ITERATION) ) {
						return -EINVAL;
					}
					// Request status is correct. Start next measurement/sampling.
					if (measure(s.next_channel) < 0) {
						// Error while starting measurement
						connection_reset();
						if (s.current_request->status == REQUEST_ACTIVE) {
							// The current partially filled data buffer needs to be
							// posted to the application before stopping the request.
							// Mark it as last data packet so that hardware is reset
							// before next sampling begins.
							s.current_request->status = REQUEST_LAST_EVENT;
							// Post the current obtained samples to application.
							handle_data_ready(s.current_sample_cnt);
						} else {
							// The last data buffer has already been posted to application.
							// Stop the request.
							s.current_request->status = REQUEST_COMPLETE;
							post_task_to_start_next_sampling();
						}
					} else {
						// Measurement has started successfully.
						if (s.current_request->status == REQUEST_NEXT_ITERATION) {
							// Reset sample count for new iteration.
							s.current_sample_cnt = 0;
							s.current_request->status = REQUEST_ACTIVE;
						}
					}
					break;
				}
				case SHT1x_TEMPERATURE_TIMER: {
					// Get the sample from chip.
					no_error = get_data(&(s.buf[s.current_sample_cnt]), &checksum, TEMPERATURE);
					if (no_error == false) {
						// Some error while getting the data.
						s.buf[s.current_sample_cnt] = 0xFFFF;
						s.current_sample_cnt++;
						// Sampling has already been stopped.
						// Mark it as last data packet so that hardware is reset
						// before next sampling begins.
						s.current_request->status = REQUEST_LAST_EVENT;
						// Post the current obtained samples to application.
						handle_data_ready(s.current_sample_cnt);
						return SOS_OK;
					}
					// Got the data sample successfully from sht1x chip.
					s.current_sample_cnt++;
					// Now check if need to get humidity sample too.
					if (SHT1x_HUMIDITY_CH & s.current_request->channels) {
						// Current sample is not finished. Need to get
						// humidity data too.
						if (measure(SHT1x_HUMIDITY_CH) < 0) {
							// Error while starting measurement
							connection_reset();
							s.buf[s.current_sample_cnt] = 0xFFFF;
							s.current_sample_cnt++;
							// Mark it as last data packet so that hardware is reset
							// before next sampling begins.
							s.current_request->status = REQUEST_LAST_EVENT;
							// Post the current obtained samples to application.
							handle_data_ready(s.current_sample_cnt);
						}
						return SOS_OK;
					}
					// If code reaches here, it means application only needs temperature data.
					process_sample_buffer(SHT1x_TEMPERATURE_CH);
					break;
				}
				case SHT1x_HUMIDITY_TIMER: {
					// Get the sample from chip.
					no_error = get_data(&(s.buf[s.current_sample_cnt]), &checksum, HUMIDITY);
					if (no_error == false) {
						// Some error while getting the data.
						s.buf[s.current_sample_cnt] = 0xFFFF;
						s.current_sample_cnt++;
						// Sampling has already been stopped.
						// Mark it as last data packet so that hardware is reset
						// before next sampling begins.
						s.current_request->status = REQUEST_LAST_EVENT;
						// Post the current obtained samples to application.
						handle_data_ready(s.current_sample_cnt);
						return SOS_OK;
					}
					// Got the data sample successfully from sht1x chip.
					s.current_sample_cnt++;
					process_sample_buffer(s.current_request->channels);
					break;
				}
				default:
						break;
			}
			break;
		}
        default:
            {
                return -EINVAL;
            }      

    }

    return SOS_OK;

}


////
// Implementation of local functions
////

static int8_t task_next_sample() {
	// Check status of current request.
	// Remove current request if it has been completed.
	if (s.current_request != NULL) {
		if (s.current_request->status != REQUEST_COMPLETE) {
			// Current request has not been completed. Return.
			return -EINVAL;
		} else {
			// Disable the sensors if they are still registered/bound
			// to their respective hardware channels.
			SOS_CALL(s.sensor_feedback, sensor_feedback_func_t, SENSOR_DISABLE_CMD, 
					SHT1x_TEMPERATURE_CH, s.current_request->sensor_context);
			// Check if ERROR flag was set. This happens when no sensor driver exists.
			if (s.state & SHT1x_COMM_ERROR) {
				reinitialize_comm();
				return SOS_OK;
			}
			// Free up the array of pointers to data buffers for each channel.
			if (s.buf != NULL) {
				ker_free(s.buf);
				s.buf = NULL;
			}
			// Free up the memory occupied by current request.
			// Freeing up sensor_context is the responsibility of the
			// application.
			ker_free(s.current_request);
			s.current_request = NULL;
			// SHT1x driver is IDLE now.
			s.state = (s.state & SHT1x_COMM_HW_ON) | SHT1x_COMM_IDLE;
		}
	}
	// The code should reach here only if current_request = NULL and
	// SHT1x is IDLE.
	if ((s.state & SHT1x_COMM_IDLE) != SHT1x_COMM_IDLE) {
			return -EINVAL;
	}

	// Check if there is any pending request.
	s.current_request = request_dequeue();
	if (s.current_request == NULL) {
		// Turn off ADC if queue is empty.
		if (s.state & SHT1x_COMM_HW_ON) {
			disable_sht1x();
			s.state = SHT1x_COMM_IDLE;
		}
	} else {
		// Turn ON the SHT1x chip if it was OFF earlier.
		if (!(s.state & SHT1x_COMM_HW_ON)) {
			enable_sht1x();
			s.state |= SHT1x_COMM_HW_ON;
			ker_timer_start(SHT1x_COMM_PID, SHT1x_DELAY_TIMER, SHT1x_DELAY_TIME);
			s.current_request->status = REQUEST_DELAY;
			return SOS_OK;
		}
		serve_request();
	}
	return SOS_OK;
}

static void serve_request() {
	// Start serving the current request after its delay has expired.
	// Enable/configure sensors.
	SOS_CALL(s.sensor_feedback, sensor_feedback_func_t, SENSOR_ENABLE_CMD, 
			SHT1x_TEMPERATURE_CH, s.current_request->sensor_context);
	// Check if ERROR flag was set. This happens when no sensor driver exists.
	if (s.state & SHT1x_COMM_ERROR) {
		reinitialize_comm();
		return;
	}
	if (sht1x_start_sampling() < 0) {
		// Disable the sensors.
		SOS_CALL(s.sensor_feedback, sensor_feedback_func_t, SENSOR_DISABLE_CMD, 
				SHT1x_TEMPERATURE_CH, s.current_request->sensor_context);
		// Send an error message back to application to
		// indicate error.
		SOS_CALL(s.sensor_data_ready, sensor_data_ready_func_t, SHT1x_SENSOR_ERROR, 
				s.current_request->app_id, s.current_request->channels, NULL);
		// Check if ERROR flag was set. This happens when no sensor driver exists.
		if (s.state & SHT1x_COMM_ERROR) {
			reinitialize_comm();
			return;
		}
		// Mark current request as COMPLETE to remove it.
		s.current_request->status = REQUEST_COMPLETE;
		// Post task to start next sampling. 
		post_task_to_start_next_sampling();
		// Reset the SHt1x chip.
		connection_reset();
	} else {
		// Sampling started fine. Mark ADC driver as BUSY
		// and current_request as ACTIVE.
		s.state = SHT1x_COMM_HW_ON | SHT1x_COMM_BUSY;
		s.current_request->status = REQUEST_ACTIVE;
	}
}

static int8_t sht1x_start_sampling() {
	// Start sampling.
	// Assume: current_request is valid,
	// samples and event_samples has been set up correctly.
	uint8_t i;

	// First reset the active request parameters in driver state.
	reset_active_request_params();

	// Calculate number of channels (sensors) that need to be sampled.
	s.num_channels = 0;
	for(i = 0; i < SHT1x_NUM_SENSORS; i++) {
		if (BV(i) & s.current_request->channels) {
			s.num_channels++;
		}
	}

	// Allocate memory for event_samples sensor readings for each channel.
	// We allocate two buffers and switch them when one is full so as to 
	// enable continuous sampling.
	s.buf = (uint16_t *)ker_malloc(
					s.current_request->event_samples * s.num_channels * sizeof(uint16_t),
					SHT1x_COMM_PID);
	if (s.buf == NULL) {
		return -ENOMEM;
	}

	// Continuous sampling proceeds in iterations, where each iteration length
	// is equal to the number of samples required to raise the next data_ready event.
	s.current_sample_cnt = 0;
	s.target_samples_in_iteration = s.current_request->event_samples;

	/** 
	 * Is a reset neeeded before talking to the sht1x?  Once
	 * the code is up and running, look into removing it.
	 */
	make_clock_output();
	connection_reset(); 

	// Start sampling.
	return measure(s.current_request->channels);
}

static inline int8_t post_task_to_start_next_sampling() {
	post_short(SHT1x_COMM_PID, SHT1x_COMM_PID, MSG_SHT1x_START_NEXT_SAMPLING, 0, 0, 0);
	return SOS_OK;
}

static inline void reset_active_request_params() {
	s.current_sample_cnt = 0;
	s.target_samples_in_iteration = 0;
	s.num_channels = 0;
	s.next_channel = 0;
}

static void reinitialize_comm() {
	// Stop all timers.
	ker_timer_stop(SHT1x_COMM_PID, SHT1x_TEMPERATURE_TIMER);
	ker_timer_stop(SHT1x_COMM_PID, SHT1x_HUMIDITY_TIMER);
	ker_timer_stop(SHT1x_COMM_PID, SHT1x_DELAY_TIMER);
	ker_timer_stop(SHT1x_COMM_PID, SHT1x_SAMPLE_TIMER);
	// Disable hardware.
	disable_sht1x();
	// Reset hardware state.
	// Error state is cleared when sensor driver access the get_data
	// or stop_data interface later.
	s.state = SHT1x_COMM_ERROR | SHT1x_COMM_IDLE;
	// Flush the request queue.
	data_request_t *itr = request_dequeue();
	while (itr != NULL) {
		ker_free(itr);
		itr = request_dequeue();
	}
	if (s.current_request != NULL) {
		ker_free(s.current_request);
	}
	if (s.new_request != NULL) {
		ker_free(s.new_request);
	}
	if (s.buf != NULL) {
		ker_free(s.buf);
	}
	s.new_request = NULL;
	s.current_request = NULL;
	s.buf = NULL;
	reset_active_request_params();
}

int8_t sht1x_comm_init() {
	s.state = SHT1x_COMM_INIT;
	
	sched_register_kernel_module(&sht1x_comm_module, 
				sos_get_header_address(mod_header), &s);

	s.request_queue.head = NULL;
	s.request_queue.tail = NULL;
	s.new_request = NULL;
	s.current_request = NULL;
	reset_active_request_params();
	s.buf = NULL;
	
	ker_timer_init(SHT1x_COMM_PID, SHT1x_TEMPERATURE_TIMER, TIMER_ONE_SHOT);
	ker_timer_init(SHT1x_COMM_PID, SHT1x_HUMIDITY_TIMER, TIMER_ONE_SHOT);
	ker_timer_init(SHT1x_COMM_PID, SHT1x_DELAY_TIMER, TIMER_ONE_SHOT);
	ker_timer_init(SHT1x_COMM_PID, SHT1x_SAMPLE_TIMER, TIMER_ONE_SHOT);
	
	s.state = SHT1x_COMM_IDLE;

	return SOS_OK;
}


static int8_t sht1x_get_data (func_cb_ptr cb, sht1x_sensor_command_t command, sos_pid_t app_id, 
	uint16_t channels, sample_context_t *param, void *context) {
	// Clear the ERROR flag from hardware state.
	// Assumption: This function can only be called by a valid
	// sensor driver.
	s.state &= ~(SHT1x_COMM_ERROR);
	// Verify the sample context parameters.
	// period should always be greater than 20 units.
	// This is the smallest delay that can be set with a software timer in SOS.
	if (param->period < 20) return -EINVAL;
	// event_samples should never be zero.
	if (param->event_samples == 0) return -EINVAL;
	// If samples > 0, event_samples <= samples.
	if ((param->samples > 0) && (param->event_samples > param->samples)) {
		param->event_samples = param->samples;
	}

	// Take action based on command.
	switch (command) {
		case SHT1x_REGISTER_REQUEST: {
			// Check if new request exists.
			if (s.new_request == NULL) {
				// Create one, if it does not.
				s.new_request = ker_malloc(sizeof(data_request_t), SHT1x_COMM_PID);
				if (s.new_request == NULL) {
					return -EINVAL;
				}
				// Initialize the new request.
				s.new_request->status = REQUEST_INIT;
				s.new_request->app_id = app_id;
				s.new_request->channels = 0;
				s.new_request->delay = param->delay;
				s.new_request->period = param->period;
				s.new_request->samples = param->samples;
				s.new_request->event_samples = param->event_samples;
				s.new_request->next = NULL;
			} else if (s.new_request->app_id != app_id) {
				// Some other application interfered in request registration.
				// Signal error to the interfering app.
				return -EINVAL;
			}
			// Merge channel bitmask.
			s.new_request->channels |= channels;
			break;
		}
		case SHT1x_REMOVE_REQUEST: {
			if (s.new_request == NULL) return SOS_OK;
			if (s.new_request->app_id != app_id) return -EINVAL;
			ker_free(s.new_request);
			s.new_request = NULL;
			break;
		}
		case SHT1x_GET_DATA: {
			// Ignore, if there is no new request.
			if (s.new_request == NULL) {
				return SOS_OK;
			}
			// Return error if some other application requests to start
			// collecting data.
			if (s.new_request->app_id != app_id) return -EINVAL;

			// Mark the request as registered.
			s.new_request->status = REQUEST_REGISTERED;

			// Sanity check: atleast one channel should be sampled.
			if (s.new_request->channels != 0) {
				// Put the request in request_queue.
				request_enqueue();
				s.new_request = NULL;
				// Post the task to start sampling.
				post_task_to_start_next_sampling();
			} else {
				// None of the channels has been requested to be sampled.
				// Discard the request. (freeing up sensor_context is the 
				// responsibility of the application).
				ker_free(s.new_request);
				s.new_request = NULL;
				return -EINVAL;
			}
			break;
		}
		default: return -EINVAL;
	}

	return SOS_OK;
}

static int8_t sht1x_stop_data (func_cb_ptr cb, sos_pid_t app_id, uint16_t channels) {
	// Clear ERROR flag from hardware state.
	// Assumption: This function can only be called by a valid
	// sensor driver.
	s.state &= ~(SHT1x_COMM_ERROR);
	// Stop and remove all data requests from 'app_id' that contain
	// atleast one or more 'channels'.
	// Check current request. 
	if (s.current_request != NULL) {
		// Check if application id matches and the current 
		// request is sampling atleast one of the channels
		// included in the stop request.
		// The current request should not have been marked
		// COMPLETE already.
		if ( (s.current_request->app_id == app_id) &&
			 (s.current_request->status != REQUEST_COMPLETE) &&
			 (s.current_request->channels & channels) ) {
			// Stop the current sampling immediately
			connection_reset();
			s.current_request->status = REQUEST_COMPLETE;
			post_task_to_start_next_sampling();
		}
	}
	// Check request queue.
	data_request_t *itr = s.request_queue.head;
	while (itr != NULL) {
		// Check if application id matches and the queued 
		// request contains atleast one of the channels
		// included in the stop request.
		if ( (itr->app_id == app_id) &&
			 (itr->channels & channels) ) {
			// Remove the request from queue.
			data_request_t *del = itr;
			itr = itr->next;
			request_remove(del);
		} else {
			itr = itr->next;
		}
	}
	return SOS_OK;
}

//-----------------------------------------------------------------------------
// Request queue handling functions.
//-----------------------------------------------------------------------------
static void request_enqueue() {
	if (s.request_queue.tail == NULL) {
		s.request_queue.head = s.new_request;
	} else {
		s.request_queue.tail->next = s.new_request;
	}
	s.request_queue.tail = s.new_request;
}

static void request_remove(data_request_t *del) {
	data_request_t *itr = s.request_queue.head;

	// If the element to be removed is at head of queue.
	if (del == itr) {
		s.request_queue.head = itr->next;
		if (s.request_queue.head == NULL) {
			s.request_queue.tail = NULL;
		}
		goto exit_remove;
	}

	// Search for element in the remaining queue.
	while ((itr != NULL) && (itr->next != del)) {
		itr = itr->next;
	}

	// Found the element at position itr->next.
	if (itr != NULL) {
		itr->next = del->next;
		del->next = NULL;
		// Element was tail of queue. Update tail pointer.
		if (del == s.request_queue.tail) {
			s.request_queue.tail = itr;
		}
	}

exit_remove:
	ker_free(del);
}

static data_request_t* request_dequeue() {
	data_request_t *ret = s.request_queue.head;
	if (ret == NULL) return NULL;

	s.request_queue.head = s.request_queue.head->next;
	if (s.request_queue.head == NULL) {
		s.request_queue.tail = NULL;
	}
	return ret;
}
/**
 * Hardware specific delay.  This is target towards the mica2 and micaZ motes.
 * 
 * These will probably be moved into a driver file at some point, since this
 * is hardware specific.
 */
static void delay() {
    asm volatile  ("nop" ::);
    asm volatile  ("nop" ::);
    asm volatile  ("nop" ::);
}


/** 
 * Write a byte to the SHT1x
 *
 * \param byte Byte to write to the SHT1x
 *
 * \return True if there were no errors
 *
 */
static int8_t write_byte(uint8_t byte)
{
    HAS_CRITICAL_SECTION;

    uint8_t i;

    ENTER_CRITICAL_SECTION();
    for (i=0x80; i>0; i = i>>1) 
    {
        if (i & byte) {
            set_data();
        } else {
            clear_data();
        }

        set_clock();
        delay();
        delay();
        clear_clock();        
    }

    set_data();
    make_data_input();
    delay();
    set_clock();

    // SHT1x pulls line low to signal ACK so 0 is good
    if (GETBIT(SHT1x_READ_PIN, SHT1x_PIN_NUMBER) != 0) {
        return -EINVAL;
    }

    delay();
    clear_clock();
    make_data_input();
    
    LEAVE_CRITICAL_SECTION();
    return SOS_OK;
}


/** 
 * Reads a byte from the SHT1x 
 *
 * \param ack Enable or disable ack for the byte
 *
 * \return Byte read from the SHT1x
 *
 */
static uint8_t read_byte(sht1x_ack_t ack)
{
    HAS_CRITICAL_SECTION;

    uint8_t i;
    uint8_t val;

    val = 0;

    ENTER_CRITICAL_SECTION();

    set_data();
    make_data_input();
    delay();

    for (i=0x80; i>0; i = i>>1) 
    {
        set_clock();
        delay();
    
        // SHT1x pulls line low to signal 0, otherwise, we need to put on one
        // into the correct bit of val
        if (GETBIT(SHT1x_READ_PIN, SHT1x_PIN_NUMBER) != 0) {
            val = (val | i);
        }
        clear_clock();
    }

    make_data_output();

    if (ack == ACK) {
        clear_data();
    }

    delay();
    set_clock();
    delay();
    clear_clock();
    LEAVE_CRITICAL_SECTION();

    return val;
}

/** 
 * Generates a transmission start
 * 
 * The lines should look like:
 *       ____      _____
 * DATA:     |____|
 *          __    __
 * SCK : __|  |__|  |____
 */
static void start_transmission() 
{
    HAS_CRITICAL_SECTION;
    ENTER_CRITICAL_SECTION();
    make_data_output();
    set_data(); 
    clear_clock();
    delay();
    set_clock();
    delay();
    clear_data();
    delay();
    clear_clock();
    delay();
    set_clock();
    delay();
    set_data();
    delay();
    clear_clock();
    LEAVE_CRITICAL_SECTION();
}


/**
 * Reset a connection if anything should go wrong
 */
static void connection_reset() 
{
    HAS_CRITICAL_SECTION;

    uint8_t i;

    ENTER_CRITICAL_SECTION();
    make_data_output();
    set_data(); 
    clear_clock();
    for (i=0; i<9; i++) 
    {
        set_clock();
        delay();
        clear_clock();
    }
    LEAVE_CRITICAL_SECTION();
}


/** 
 * Begin a measurement of either temperature or humidity from the SHT1x
 *
 * \param command Either TEMPERATURE to request a temperature measurement or
 * HUMIDITY to measure humidity
 *
 * \return True if there were no errors
 * 
 */
static int8_t measure(uint8_t channel) 
{
	int8_t ret;

    start_transmission();
	
	if (SHT1x_TEMPERATURE_CH & channel) {
		ret = write_byte(MEASURE_TEMPERATURE);
		if (ret < 0) return ret;
		if (ker_timer_start(SHT1x_COMM_PID, SHT1x_TEMPERATURE_TIMER, SHT1x_TEMPERATURE_TIME) < 0) {
			return -EINVAL;
		}
	} else if (SHT1x_HUMIDITY_CH & channel) {
		ret = write_byte(MEASURE_HUMIDITY);
		if (ret < 0) return ret;
		if (ker_timer_start(SHT1x_COMM_PID, SHT1x_HUMIDITY_TIMER, SHT1x_HUMIDITY_TIME) < 0) {
			return -EINVAL;
		}
	}
    return SOS_OK;
}


/** 
 * Get the measurement from the SHT1x
 *
 * \param data Pointer to a 2-byte data buffer
 *
 * \param checksum Pointer to a 1-byte checksum
 *
 * \param command Either TEMPERATURE to request a temperature measurement or
 * HUMIDITY to measure humidity
 *
 * \return True if there were no errors
 *
 * \note The timing used to delay calls to this function are dependent on the
 * measurement size being requested from the SHT1x.  This code assumes the
 * defaults of 12-bit temperature readings and 14-bit humidity readings.
 * 
 */
static bool get_data(uint16_t *data, uint8_t *checksum, sht1x_command_t command)
{
    make_data_input();
    delay();
    
    // SHT1x pulls line low to signal that the data is ready
    if (GETBIT(SHT1x_READ_PIN, SHT1x_PIN_NUMBER) != 0) {
        return false;
    }

    *data = (read_byte(ACK)<<8);
    *data += read_byte(ACK);
    *checksum  = read_byte(NOACK);
    
    return true;
}

static int8_t driver_error(func_cb_ptr cb) {
	s.state |= SHT1x_COMM_ERROR;
	return -EINVAL;
}

#ifndef _MODULE_
mod_header_ptr sht1x_comm_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif



