/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sos.h>
#include <adc_driver_private.h>

#define LED_DEBUG
#include <led_dbg.h>

/**
 * This implements the ADC12 module found on the MSP430x13x, MSP430x15x, and 
 * MSP430x16x.
 * It uses DMA (in block-to-block addressing mode, single conversion) for all 
 * data collection requests.
 */

int8_t ker_adc_bind_channel (sos_pid_t driver_id, uint16_t channels, uint8_t control_cb_fid, uint8_t cb_fid, void *config);
int8_t ker_adc_unbind_channel (sos_pid_t driver_id, uint16_t channels);
int8_t ker_adc_get_data (uint8_t command, sos_pid_t app_id, uint16_t channels, 
						sample_context_t *param, void *context);
int8_t ker_adc_stop_data (sos_pid_t app_id, uint16_t channels);

//-----------------------------------------------------------------------------
// LOCAL VARIABLES
//-----------------------------------------------------------------------------
typedef struct adc_proc_state {
	func_cb_ptr data_ready[ADC_DRIVER_CHANNEL_MAPSIZE];
	func_cb_ptr sensor_feedback[ADC_DRIVER_CHANNEL_MAPSIZE];
	channel_map_t channel_config_map[ADC_DRIVER_CHANNEL_MAPSIZE];
	uint8_t state;
	request_queue_t request_queue;
	data_request_t *new_request, *current_request;
	// These variables are set according to current 
	// request when sampling is active.
	uint32_t current_sample_cnt, target_samples_in_iteration;
	sampling_mode_t mode;
	uint16_t num_channels;
	sensor_data_msg_t **send_buf_array;
	uint8_t buf_ptr;
	uint16_t *buf[2];
} adc_proc_state_t;

static adc_proc_state_t s;
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// INTERNAL FUNCTIONS
//-----------------------------------------------------------------------------
static void adc_on();
static void adc_off();
static int8_t adc_start_sampling();
static int8_t adc_stop_sampling();
static int8_t task_next_sample();
static int8_t handle_data_event(uint8_t buf_ptr, uint16_t num_samples);
static inline void reset_timera();
static inline void reset_adc();
static inline void reset_dma();
static inline void reset_active_request_params();
static inline int8_t post_task_to_start_next_sampling();
static inline int8_t post_data_ready_event(uint8_t buf_ptr, uint32_t cnt);

static void request_enqueue();
static void request_remove(data_request_t *del);
static data_request_t* request_dequeue();
//-----------------------------------------------------------------------------

static int8_t adc_proc_msg_handler(void *state, Message *msg);
static sos_module_t adc_proc_module;

static const mod_header_t mod_header SOS_MODULE_HEADER =
{
  	.mod_id = ADC_DRIVER_PID,
	.state_size = 0,
	.num_prov_func = 0,
	.num_sub_func = (ADC_DRIVER_CHANNEL_MAPSIZE*2),
	.module_handler= adc_proc_msg_handler,
	.funct = {
		// Data ready callbacks for all supported channels
		// sensor 0
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 1
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 2
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 3
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 4
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 5
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 6
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 7
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 8
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 9
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 10
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// sensor 11
		{error_8, "cCS3", RUNTIME_PID, RUNTIME_FID},
		// Sensor control interfaces for all supported channels
		// sensor 0
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 1
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 2
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 3
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 4
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 5
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 6
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 7
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 8
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 9
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 10
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
		// sensor 11
		{error_8, "cCw2", RUNTIME_PID, RUNTIME_FID},
	},
};


static int8_t adc_proc_msg_handler(void *state, Message *msg) {
	switch (msg->type) {
		case MSG_ADC_START_NEXT_SAMPLING: {
			// Task to start next sampling from the request queue.
			return task_next_sample();
		}
		case MSG_ADC_HANDLE_DATA_EVENT: {
			// Handle the data_ready event.
			// Process and pass data to higher layer.
			MsgParam *t = (MsgParam *)msg->data;
			return handle_data_event(t->byte, t->word);
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}

//-----------------------------------------------------------------------------
// Internal Tasks
//-----------------------------------------------------------------------------
static inline int8_t post_task_to_start_next_sampling() {
	post_short(ADC_DRIVER_PID, ADC_DRIVER_PID, MSG_ADC_START_NEXT_SAMPLING, 0, 0, 0);
	return SOS_OK;
}

static inline int8_t post_data_ready_event(uint8_t buf_ptr, uint32_t cnt) {
	post_short(ADC_DRIVER_PID, ADC_DRIVER_PID, MSG_ADC_HANDLE_DATA_EVENT, buf_ptr, (uint16_t)cnt, SOS_MSG_SYSTEM_PRIORITY);
	return SOS_OK;
}

static int8_t task_next_sample() {
	HAS_CRITICAL_SECTION;
	uint8_t i;

	ENTER_CRITICAL_SECTION();
	// Check status of current request.
	// Remove current request if it has been completed.
	if (s.current_request != NULL) {
		if (s.current_request->status != REQUEST_COMPLETE) {
			// Current request has not been completed. Return.
			return -EINVAL;
		} else {
			// Disable the sensors if they are still registered/bound
			// to their respective hardware channels.
			for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
				if ( (BV(i) & s.current_request->channels) &&
					 (s.channel_config_map[i].driver_id != NULL_PID) ) {
					SOS_CALL(s.sensor_feedback[i], sensor_feedback_func_t, SENSOR_DISABLE_COMMAND, BV(i), s.current_request->sensor_context);
				}
			}
			// Free up the sampling buffers.
			if (s.buf[0] != NULL) {
				ker_free(s.buf[0]);
				s.buf[0] = NULL;
			}
			if (s.buf[1] != NULL) {
				ker_free(s.buf[1]);
				s.buf[1] = NULL;
			}
			// Free up the array of pointers to data buffers for each channel.
			if (s.send_buf_array != NULL) {
				ker_free(s.send_buf_array);
				s.send_buf_array = NULL;
			}
			// Free up the memory occupied by current request.
			// Freeing up sensor_context is the responsibility of the
			// application.
			ker_free(s.current_request);
			s.current_request = NULL;
			// ADC driver is IDLE now.
			s.state = (s.state & ADC_DRIVER_HW_ON) | ADC_DRIVER_IDLE;
		}
	}
	// The code should reach here only if current_request = NULL and
	// ADC is IDLE.
	if ((s.state & ADC_DRIVER_IDLE) != ADC_DRIVER_IDLE) {
			LEAVE_CRITICAL_SECTION();
			return -EINVAL;
	}

	// Check if there is any pending request.
	s.current_request = request_dequeue();
	if (s.current_request == NULL) {
		// Turn off ADC if queue is empty.
		if (s.state & ADC_DRIVER_HW_ON) {
			adc_off();
			s.state = ADC_DRIVER_IDLE;
		}
	} else {
		// Turn ON the adc if it was OFF earlier.
		if (!(s.state & ADC_DRIVER_HW_ON)) {
			adc_on();
			s.state |= ADC_DRIVER_HW_ON;
			// Enable/configure sensors used in the current request.
			for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
				if (BV(i) & s.current_request->channels) {
					SOS_CALL(s.sensor_feedback[i], sensor_feedback_func_t, SENSOR_ENABLE_COMMAND, BV(i), s.current_request->sensor_context);
				}
			}
		}
		// Setup ADC, DMA and Timer A (unit 1) acording to user parameters,
		// and start sampling.
		if (adc_start_sampling() < 0) {
			// Disable the sensors.
			for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
				if (BV(i) & s.current_request->channels) {
					SOS_CALL(s.sensor_feedback[i], sensor_feedback_func_t, SENSOR_DISABLE_COMMAND, BV(i), s.current_request->sensor_context);
				}
			}
			// Send an error message back to application to
			// indicate error.
			for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
				if (BV(i) & s.current_request->channels) {
					SOS_CALL(s.data_ready[i], data_ready_func_t, ADC_SENSOR_ERROR, s.current_request->app_id, 
									s.current_request->channels, NULL);
					break;
				}
			}
			// Mark current request as COMPLETE to remove it.
			s.current_request->status = REQUEST_COMPLETE;
			// Post task to start next sampling. 
			post_task_to_start_next_sampling();
		} else {
			// Sampling started fine. Mark ADC driver as BUSY
			// and current_request as ACTIVE.
			s.state = ADC_DRIVER_HW_ON | ADC_DRIVER_BUSY;
			s.current_request->status = REQUEST_ACTIVE;
		}
	}
	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}

static int8_t handle_data_event(uint8_t buf_ptr, uint16_t num_samples) {
	uint16_t *reading = NULL;
	uint16_t i;
	uint8_t ch;
	uint32_t j;

	// Save the important variables that can be modified by
	// DMA interrupt handler.
	//uint8_t buf_ptr = (s.buf_ptr + 1) % 2;
	//uint32_t num_samples = s.target_samples_in_iteration;

	// Ignore, if request is marked COMPLETE.
	if (s.current_request->status == REQUEST_COMPLETE) return -EINVAL;

	// Take action depending on number of channels sampled.
	if (s.num_channels == 1) {
		// Allocate buffer for the channel.
		sensor_data_msg_t* send_buf = (sensor_data_msg_t *)ker_malloc(sizeof(sensor_data_msg_t) + (num_samples*sizeof(uint16_t)), ADC_DRIVER_PID);
		if (send_buf == NULL) goto handle_data_exit;
		// Copy samples into the send_buf.
		send_buf->status = SENSOR_DATA;
		send_buf->sensor = 0;
		send_buf->num_samples = num_samples;
		memcpy(send_buf->buf, s.buf[buf_ptr], num_samples * sizeof(uint16_t));
		// Call data_ready event for the requested channel.
		for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
			if (BV(i) & s.current_request->channels) {
				SOS_CALL(s.data_ready[i], data_ready_func_t, ADC_SENSOR_SEND_DATA, s.current_request->app_id, BV(i), send_buf);
				break;
			}
		}
	} else {
		// Allocate a data buffer for each channel.
		// The pointers to these buffers are stored in send_buf_array.
		for (i = 0; i < s.num_channels; i++) {
			s.send_buf_array[i] = (sensor_data_msg_t *)ker_malloc(sizeof(sensor_data_msg_t) + (sizeof(uint16_t) * num_samples), 
														ADC_DRIVER_PID);
			if (s.send_buf_array[i] == NULL) {
				int16_t k;
				for (k = i - 1; k > -1; k--) {
						ker_free(s.send_buf_array[k]);
				}
				goto handle_data_exit;
			}
			s.send_buf_array[i]->status = SENSOR_DATA;
			s.send_buf_array[i]->sensor = 0;
			s.send_buf_array[i]->num_samples = num_samples;
		}
		// Move the samples from s.buf[buf_ptr] to above data buffers.
		reading = s.buf[buf_ptr];
		for (j = 0; j < num_samples; j++) {
			ch = 0;
			for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
				if (BV(i) & s.current_request->channels) {
					s.send_buf_array[ch]->buf[j] = *reading;
					ch++;
					reading++;
				}
			}
		}
		// Call data_ready event for each requested channel.
		ch = 0;
		for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
			if (BV(i) & s.current_request->channels) {
				SOS_CALL(s.data_ready[i], data_ready_func_t, ADC_SENSOR_SEND_DATA, s.current_request->app_id, 
								BV(i), s.send_buf_array[ch]);
				ch++;
			}
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
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Internal functions
//-----------------------------------------------------------------------------
static inline void reset_active_request_params() {
	s.current_sample_cnt = 0;
	s.target_samples_in_iteration = 0;
	s.mode = ADC_SINGLE_SAMPLE;
	s.num_channels = 0;
	s.buf_ptr = 0;
}

static inline void reset_timera() {
	TACTL = TACLR;			// Stop Timer A (Set TACLR, counting mode = 0)
	TACCR0 = 0;				// Reset timer period..
	TACCR1 = 0;
	TAIV = 0;
}

static inline void reset_adc() {
	// Configure ADC (remember to keep ENC = 0 while configuring).
	// Set fixed default ADC parameters.
	// Sample time, reference voltage and conversion start mode is set 
	// in adc_start_sampling according to user parameters.
	ADC12CTL0 = ( REFON | ADC12ON | ADC_MULTIPLE_SAMPLE_CONVERSION );
	ADC12CTL1 = ( ADC_CONV_MEMORY_START | ADC_PULSE_SAMPLING | 
					ADC_CONV_SEQUENCE | ADC_CLOCK_SCALE_1 |
					ADC_SOURCE_CLOCK );
	ADC12IFG = 0;			// Clear any pending interrupts.
	
}

static inline void reset_dma() {
	// Fixed default DMA configuration.
	// Use DMA channel 0 for ADC transfers
	// DMAEN should be set in adc_start_sampling. DMAEN = 0 here.
	// DMAEN = 0 before modifying DMACTL0.
	// Destination memory address and size of block
	// is set in adc_start_sampling too.
	DMA0CTL = ( DMA_TRANSFER_MODE | DMA_ADDR_MODE | DMA_SRC_DST_TYPE );
	DMACTL0 = DMA_0_TRIGGER_ADC;
	DMACTL1 = 0;
	DMA0SZ = 0;
}

static void adc_on() {
	// Stop ADC, Timer A and DMA to remove any inconsistency.
	adc_off();
	reset_adc();
	reset_dma();
}

static void adc_off() {
	reset_timera();			// Stop Timer A.
	DMA0CTL = 0; 			// Stop DMA transfers.
	ADC12CTL0 = 0;			// Disable ADC.
	ADC12CTL1 = 0;
	ADC12IFG = 0;			// Clear any pending interrupts.
}

static int8_t adc_start_sampling() {
	// Start sampling.
	// Assume: current_request is valid,
	// samples and event_samples has been set up correctly.
	uint8_t i;
	char *adc_conv_mem_ctrl = ADC12MCTL;

	// First reset the active request parameters in driver state.
	reset_active_request_params();

	// Set ADC conversion memory control registers according
	// to requested channels.
	s.num_channels = 0;
	for(i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
		if (BV(i) & s.current_request->channels) {
			*adc_conv_mem_ctrl = ADC_VOLT_INTERNAL_REF | i;
			adc_conv_mem_ctrl++;
			s.num_channels++;
		}
	}

	// Allocate memory for event_samples sensor readings for each channel.
	// We allocate two buffers and switch them when one is full so as to 
	// enable continuous sampling.
	s.buf[0] = (uint16_t *)ker_malloc(
					s.current_request->event_samples * s.num_channels * sizeof(uint16_t),
					ADC_DRIVER_PID);
	if (s.buf[0] == NULL) {
		return -ENOMEM;
	}
	s.buf_ptr = 0;
	s.buf[1] = NULL;
	// Allocate second buffer only if you need it:
	// continuous unlimited sampling OR 
	// limited sampling where data is sent to higher layer before sampling is complete.
	if ( (s.current_request->samples == 0) || 
		 ((s.current_request->samples > 0) && 
		  (s.current_request->event_samples < s.current_request->samples)) ) {
		s.buf[1] = (uint16_t *)ker_malloc( 
					s.current_request->event_samples * s.num_channels * sizeof(uint16_t),
					ADC_DRIVER_PID);
		if (s.buf[1] == NULL) {
			return -ENOMEM;
		}
	}

	s.send_buf_array = NULL;
	if (s.num_channels > 1) {
		// Allocate an array of buffer pointers if more than one channels
		// need to be sampled simultaneously.
		// This array is freed by the driver when this sampling
		// is stopped.
		s.send_buf_array = (sensor_data_msg_t **)ker_malloc(s.num_channels*sizeof(sensor_data_msg_t*), ADC_DRIVER_PID);
		if (s.send_buf_array == NULL) return -ENOMEM;
	}

	// Continuous sampling proceeds in iterations, where each iteration length
	// is equal to the number of samples required to raise the next data_ready event.
	s.current_sample_cnt = 0;
	s.target_samples_in_iteration = s.current_request->event_samples;

	// Set EOS bit in last conversion memory ctrl register
	adc_conv_mem_ctrl--;
	*adc_conv_mem_ctrl |= EOS; 
	
	// Set sample time and reference voltage according to sensor parameters.
	ADC12CTL0 |= (s.current_request->config.sht0 | s.current_request->config.ref2_5);

	if (s.num_channels > 1) {
		// Use default configuration for DMA
		// Set DMA tranfer size DMASZ to number of channels
		DMA0SZ = s.num_channels;
	} else {
		// Set DMA tranfer size DMASZ to number of samples per event
		DMA0SZ = s.target_samples_in_iteration;
		// Set single transfer mode, fixed to block addressing mode
		// so as to minimize DMA interrupts
		DMA0CTL = ( DMA_TRANSFER_MODE_SINGLE | DMA_ADDR_MODE_FIXED_BLOCK | DMA_SRC_DST_TYPE );
	}

	// Set DMA source address to ADC12MEM0
	DMA0SA = (unsigned int)(ADC12MEM);

	// Set DMA destination address to first buffer buf[0]
	DMA0DA = (unsigned int)(s.buf[0]);

	// Enable DMA and it's interrupt
	DMA0CTL |= ( DMAEN | DMAIE );

	// Setup final ADC configuration
	if (s.current_request->samples == 1) {
		// Only one sample is requested.
		s.mode = ADC_SINGLE_SAMPLE;
		// ADC conversion is initiated by the ADC12SC bit set by software.
		// Timer A is not used here.
		// Set ADC conversion start mode to ADC12SC bit.
		ADC12CTL1 |= ADC_CONVERSION_SC;

		// Enable and start ADC conversion.
		ADC12CTL0 |= ( ENC | ADC12SC );
	} else {
		// Number of samples > 1. Start periodic sampling.
		s.mode = ADC_PERIODIC_SAMPLE;
		// ADC conversion is initiated by Timer A, unit 1.
		ADC12CTL1 |= ADC_CONVERSION_TIMER_A;

		if (s.num_channels == 1) {
			// First, unset the previous conversion mode
			ADC12CTL1 &= ~(CONSEQ_0);
			// Set ADC to sample a single channel repeatedly
			ADC12CTL1 |= ADC_CONV_SINGLE_REPEAT;
			// Disable automatic multiple sample conversion
			ADC12CTL0 &= ~(ADC_MULTIPLE_SAMPLE_CONVERSION);
		}

		// Configure Timer A, output unit 1.
		TACTL = TACLR;
		TACCR0 = s.current_request->period; 	// time when reset
		TACCR1 = s.current_request->period/2; 	// time when set
		// Compare mode
		// XXX: Enable interrupt mode for debugging
		TACCTL1 = TIMERA_OUTPUT_MODE; 
		// Timer A: Interrupt disabled, No timer divider
		TACTL = ( TIMERA_SOURCE_CLK | TIMERA_CLK_DIVIDE ) & (~TACLR);
		
		// Enable ADC conversion.
		ADC12CTL0 |= ENC;

		// Start counting, UP mode
		TACTL |= TIMERA_COUNT_MODE;
	}

	return SOS_OK;
}

static int8_t adc_stop_sampling() {
	// Bring hardware back to same state as after
	// call to adc_on().
	// Stop Timer A and DMA.
	reset_timera();
	reset_dma();
	// Stop ADC conversion immediately by setting
	// conversion sequence mode to single and resetting
	// ENC.
	ADC12CTL1 &= ~(BV(1) | BV(2));
	ADC12CTL0 &= ~(ENC);
	reset_adc();
	return SOS_OK;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS: ADC API
//-----------------------------------------------------------------------------
int8_t adc_driver_init() {
	HAS_CRITICAL_SECTION;
	uint8_t i;
	
	ENTER_CRITICAL_SECTION();

	// ADC is uninitialized.
	s.state = ADC_DRIVER_INIT;
	
	// Setup the ADC, but do not turn it ON yet.
	//adc_proc_hardware_init();
  
  	// Initialize the ADC driver state.
	for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++){
		s.channel_config_map[i].driver_id = NULL_PID;
	}
	s.request_queue.head = NULL;
	s.request_queue.tail = NULL;
	s.new_request = NULL;
	s.current_request = NULL;
	reset_active_request_params();
	s.buf[0] = NULL;
	s.buf[1] = NULL;
	s.send_buf_array = NULL;

	// Register the driver.
	sched_register_kernel_module(&adc_proc_module, sos_get_header_address(mod_header), &s);
	
	// wtf???
	//s.refVal = 0x17d; // Reference value assuming 3.3 Volt power source

	// ADC driver is now configured and idle.
	// Note: the ADC is still OFF. Turn it ON before processing
	// data requests.
	s.state = ADC_DRIVER_IDLE;

	LEAVE_CRITICAL_SECTION();
	
	return SOS_OK;
}

int8_t ker_sys_adc_bind_channel (uint16_t channels, uint8_t control_cb_fid, 
												uint8_t cb_fid, void *config) {
	sos_pid_t driver_id = ker_get_current_pid();
	return ker_adc_bind_channel(driver_id, channels, control_cb_fid, cb_fid, config);
}

int8_t ker_adc_bind_channel (sos_pid_t driver_id, uint16_t channels, uint8_t control_cb_fid,  
								uint8_t cb_fid, void *config) {
	HAS_CRITICAL_SECTION;

	uint8_t i;
	int8_t sub_fn = -1;

	if (channels == 0) return -EINVAL;

	// Check if channels fall in the current supported range.
	if ((channels & ~(ADC_DRIVER_CHANNEL_BITMASK)) > 0) return -EINVAL;

	// Check if channels are already registered.
	// A channel can be bound to only one sensor.
	for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
		if (BV(i) & channels) {
			if (s.channel_config_map[i].driver_id != NULL_PID) return -EINVAL;
		}
	}

	ENTER_CRITICAL_SECTION();
	
	// It is a valid request.
	// Store information in respective channel blocks in channel_config_map.
	// Subscribe to data_ready functions provided by the sensor driver.
	for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
		if (BV(i) & channels) {
			// If channel 'i' is being registered by the sensor driver 
			if (sub_fn < 0) {
				// If the data_ready() callback function is being subscribed for first time.
				if ( (ker_fntable_subscribe(ADC_DRIVER_PID, driver_id, cb_fid, i) < 0) ||
					 (ker_fntable_subscribe(ADC_DRIVER_PID, driver_id, control_cb_fid, (ADC_DRIVER_CHANNEL_MAPSIZE + i)) < 0) ) {
					// Subscription failed. Return error code.
					LEAVE_CRITICAL_SECTION();
					return -EINVAL;
				} else {
					// Subscription successful. Store the index for later use.
					sub_fn = i;
				}
			} else {
				// data_ready() function was subscribed earlier. Re-use stored pointer.
				s.data_ready[i] = s.data_ready[sub_fn];
				s.sensor_feedback[i] = s.sensor_feedback[sub_fn];
			}
			// Save other information.
			s.channel_config_map[i].driver_id = driver_id;
			s.channel_config_map[i].config.sht0 = ((sensor_config_t *)config)->sht0;
			s.channel_config_map[i].config.ref2_5 = ((sensor_config_t *)config)->ref2_5;
		}
	}

	// Enable external ADC pins
	P6SEL |= (channels & 0xFF); // adc function (instead of general IO)
	P6DIR &= ~(channels & 0xFF); // input (instead of output)

	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}

int8_t ker_sys_adc_unbind_channel (uint16_t channels) {
	sos_pid_t driver_id = ker_get_current_pid();
	return ker_adc_unbind_channel(driver_id, channels);
}

int8_t ker_adc_unbind_channel (sos_pid_t driver_id, uint16_t channels) {
	HAS_CRITICAL_SECTION;

	uint8_t i;

	// Check if the request is valid i.e. it should contain channels currently
	// registered by driver_id.
	for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
		if (BV(i) & channels) {
			if ((s.channel_config_map[i].driver_id == NULL_PID) ||
				(s.channel_config_map[i].driver_id != driver_id)) {
				// Remove the channel from request.
				channels &= ~BV(i);
			}
		}
	}
	if (channels == 0) return SOS_OK;

	ENTER_CRITICAL_SECTION();

	// Check current request if it includes atleast one of the channels
	// being unbound (de-registered).
	if ((s.current_request != NULL) && (s.current_request->channels & channels)) {
		// Stop sampling on all channels, and send an appropriate message to
		// the application with a channel bitmask indicating the channel being
		// unbound.
		adc_stop_sampling();
		s.current_request->status = REQUEST_COMPLETE;
		post_task_to_start_next_sampling();
		for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
			if (BV(i) & s.current_request->channels & channels) {
				SOS_CALL(s.data_ready[i], data_ready_func_t, ADC_SENSOR_CHANNEL_UNBOUND, s.current_request->app_id, 
						(channels & s.current_request->channels), NULL);
				break;
			}
		}
	}

	// Check request queue. Remove requests that contain atleast one of the
	// channels being unbound.
	data_request_t *itr = s.request_queue.head;
	while (itr != NULL) {
		if (itr->channels & channels) {
			// Send appropriate message to application.
			for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
				if (BV(i) & itr->channels & channels) { 
					SOS_CALL(s.data_ready[i], data_ready_func_t, ADC_SENSOR_CHANNEL_UNBOUND, s.current_request->app_id, 
							(itr->channels & channels), NULL);
					break;
				}
			}
			// Remove the request from queue.
			data_request_t *del = itr;
			itr = itr->next;
			request_remove(del);
		} else {
			itr = itr->next;
		}
	}

	// Unbind the channel(s) i.e. set driver_id = NULL_PID.
	for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
		if (BV(i) & channels) {
			s.channel_config_map[i].driver_id = NULL_PID;
		}
	}
	// Set the pins to GPIO function
	P6SEL &= ~(channels & 0xFF);
	// Set the direction as output
	P6DIR |= (channels & 0xFF);

	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t ker_adc_get_data (uint8_t command, sos_pid_t app_id, uint16_t channels, 
						sample_context_t *param, void *context) {
	HAS_CRITICAL_SECTION;

	// Verify the sample context parameters.
	// period should always be greater than 2 units.
	if (param->period < 2) return -EINVAL;
	// event_samples should never be zero.
	if (param->event_samples == 0) return -EINVAL;
	// If samples > 0, event_samples <= samples.
	if ((param->samples > 0) && (param->event_samples > param->samples)) {
		param->event_samples = param->samples;
	}

	// Take action based on command.
	switch (command) {
		case ADC_REGISTER_REQUEST: {
			uint16_t i;
			ENTER_CRITICAL_SECTION();
			// Check if new request exists.
			if (s.new_request == NULL) {
				// Create one, if it does not.
				s.new_request = ker_malloc(sizeof(data_request_t), ADC_DRIVER_PID);
				if (s.new_request == NULL) {
					LEAVE_CRITICAL_SECTION();
					return -EINVAL;
				}
				// Initialize the new request.
				s.new_request->status = REQUEST_INIT;
				s.new_request->app_id = app_id;
				s.new_request->channels = 0;
				s.new_request->period = param->period;
				s.new_request->samples = param->samples;
				s.new_request->event_samples = param->event_samples;
				for (i = 0; i < ADC_DRIVER_CHANNEL_MAPSIZE; i++) {
					if (BV(i) & channels) {
						memcpy(&(s.new_request->config), 
							&(s.channel_config_map[i].config),
							sizeof(sensor_config_t));
					}
				}
				s.new_request->sensor_context = (sample_context_t *)context;
				s.new_request->next = NULL;
			} else if (s.new_request->app_id != app_id) {
				// Some other application interfered in request registration.
				// Signal error to the interfering app.
				LEAVE_CRITICAL_SECTION();
				return -EINVAL;
			}
			// Merge channel bitmask.
			s.new_request->channels |= channels;
			LEAVE_CRITICAL_SECTION();
			break;
		}
		case ADC_REMOVE_REQUEST: {
			if (s.new_request == NULL) return SOS_OK;
			if (s.new_request->app_id != app_id) return -EINVAL;
			ENTER_CRITICAL_SECTION();
			ker_free(s.new_request);
			s.new_request = NULL;
			LEAVE_CRITICAL_SECTION();
			break;
		}
		case ADC_GET_DATA: {
			// Ignore, if there is no new request.
			if (s.new_request == NULL) {
				return SOS_OK;
			}
			// Return error if some other application requests to start
			// collecting data.
			if (s.new_request->app_id != app_id) return -EINVAL;

			ENTER_CRITICAL_SECTION();
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
				LEAVE_CRITICAL_SECTION();
				return -EINVAL;
			}
			LEAVE_CRITICAL_SECTION();
			break;
		}
		default: return -EINVAL;
	}

	return SOS_OK;
}

int8_t ker_adc_stop_data (sos_pid_t app_id, uint16_t channels) {
	// Stop and remove all data requests from 'app_id' that contain
	// atleast one or more 'channels'.
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
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
			adc_stop_sampling();
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
	LEAVE_CRITICAL_SECTION();
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
//-----------------------------------------------------------------------------
/*
interrupt (TIMERA1_VECTOR) timera1_interrupt() {
	unsigned int taiv = TAIV;
	if (taiv & 0x0A)
			LED_DBG(LED_GREEN_TOGGLE);
	//if (taiv & 0x02)
	//		LED_DBG(LED_YELLOW_TOGGLE);
}

//interrupt (TIMERA0_VECTOR) timera0_interrupt() {
//	LED_DBG(LED_GREEN_TOGGLE);
//}
*/

#ifdef NEW_SENSING_API

interrupt (DACDMA_VECTOR) dac_dma_interrupt () {
	// Check the source of interrupt.
	if ((DMA0CTL & DMAIFG) == DMAIFG) {
		// Interrupt is from DMA.
		LED_DBG(LED_GREEN_TOGGLE);
		// Clear the interrupt flag.
		DMA0CTL &= ~DMAIFG;
		// Check the current request.
		if ( (s.current_request != NULL) && 
			 (s.current_request->status != REQUEST_COMPLETE) ) {
			// Current request is valid and active.
			// Update number of current samples collected.
			if (s.num_channels > 1) {
				s.current_sample_cnt++;
			} else {
				s.current_sample_cnt += s.target_samples_in_iteration;
			}
			// Check if current buffer is full
			if (s.current_sample_cnt == s.target_samples_in_iteration) {
				// Raise data_ready event.
				// Post task to handle sensor data.
				post_data_ready_event(s.buf_ptr, s.current_sample_cnt);
				// Update total sample count.
				if (s.current_request->samples > 0) {
					// Limited sampling.
					s.current_request->samples -= s.target_samples_in_iteration;
					if (s.current_request->samples == 0) {
						// Request completed.
						adc_stop_sampling(); 
						// Set current request status.
						s.current_request->status = REQUEST_LAST_EVENT;
					} else {
						// Start a new iteration to collect more samples.
						s.current_sample_cnt = 0;
						s.target_samples_in_iteration = 
								(s.current_request->samples < s.current_request->event_samples) ? 
								s.current_request->samples : s.current_request->event_samples;
						// Switch the DMA buffer.
						s.buf_ptr = (s.buf_ptr + 1) % 2;
						DMA0DA = (unsigned int)s.buf[s.buf_ptr];
						if (s.num_channels > 1) {
							DMA0SZ = s.num_channels;
						} else {
							DMA0SZ = s.target_samples_in_iteration;
						}
						DMA0SA = (unsigned int)ADC12MEM;
						// Toggle ENC bit in ADC.
						ADC12CTL0 &= ~(ENC);
						// Enable DMA
						DMA0CTL |= ( DMAEN | DMAIE );
						// Toggle ENC bit in ADC.
						ADC12CTL0 |= ENC;
					}
				} else {
					// Unlimited sampling.
					// Start a new iteration.
					s.current_sample_cnt = 0;
					s.target_samples_in_iteration = s.current_request->event_samples;
					// Switch the DMA buffer.
					s.buf_ptr = (s.buf_ptr + 1) % 2;
					DMA0DA = (unsigned int)s.buf[s.buf_ptr];
					if (s.num_channels > 1) {
						DMA0SZ = s.num_channels;
					} else {
						DMA0SZ = s.target_samples_in_iteration;
					}
					DMA0SA = (unsigned int)ADC12MEM;
					// Toggle ENC bit in ADC.
					ADC12CTL0 &= ~(ENC);
					// Enable DMA
					DMA0CTL |= ( DMAEN | DMAIE );
					// Toggle ENC bit in ADC.
					ADC12CTL0 |= ENC;
				}
			} else {
				// More samples need to be collected in this iteration
				// before raising the data_ready event.
				// Control reaches here only if multiple channnels are
				// being sampled simultaneously.
				// Advance the DMA buffer pointer by current sample count.
				DMA0DA = (unsigned int) ( s.buf[s.buf_ptr] + 
								(s.num_channels * s.current_sample_cnt) );
				DMA0SZ = s.num_channels;
				DMA0SA = (unsigned int)ADC12MEM;
				// Toggle ENC bit in ADC.
				ADC12CTL0 &= ~(ENC);
				// Enable DMA
				DMA0CTL |= ( DMAEN | DMAIE );
				// Toggle ENC bit in ADC.
				ADC12CTL0 |= ENC;
			}
		}	//If current request valid. END.
	} else if ((DMA0CTL & DMAABORT) == DMAABORT) {
		// DMA got aboarted.. try again next time
		adc_stop_sampling();
		// Set current request status.
		s.current_request->status = REQUEST_COMPLETE;
		// Post task
		post_task_to_start_next_sampling();
	} else {
		// Maybe source of interrupt was DAC.
		// Ignore.
	}
}
#endif

