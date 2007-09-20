/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE     
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
 * 94704.  Attention:  Intel License Inquiry.
 *
 */

#ifndef _ADC_DRIVER_PRIVATE_H
#define _ADC_DRIVER_PRIVATE_H

// This file is only included by the ADC driver module.

#include <sos_types.h>
#include <proc_msg_types.h>
#include "adc_driver_common.h"

/**
 * calibration channels
 */
//#define ADC_DRIVER_BANDGAP		0x1E
//#define ADC_DRIVER_GND			0x1F

// Internal ADC channel limit definitions
//#define ADC_DRIVER_CHANNEL_MAX		0x0F
//#define ADC_DRIVER_CHANNEL_NULL		0xFF

// Settings for ADC :
// source clock: SMCLK clocked at 1 Mhz.
#define ADC_SOURCE_CLOCK				ADC12SSEL_1
// sample and conversion timings.
#define ADC_CONVERSION_SC				SHS_0
#define ADC_CONVERSION_TIMER_A			SHS_1
#define ADC_PULSE_SAMPLING				SHP
// V+ : internal voltage reference
// V- : AVss
#define ADC_VOLT_INTERNAL_REF			SREF_1
// Clock prescaler: Default = 1
#define ADC_CLOCK_SCALE_1			ADC12DIV_0
// Conversion modes
// Not exposing repeat modes to provide precise
// timer control over sampling.
#define ADC_CONV_SEQUENCE			CONSEQ_1
#define ADC_CONV_SINGLE				CONSEQ_0
#define ADC_CONV_SINGLE_REPEAT		CONSEQ_2
// Multiple sample and conversion for sequence
// conversion mode.
#define ADC_MULTIPLE_SAMPLE_CONVERSION	MSC
// First conversion memory in sequence
#define ADC_CONV_MEMORY_START	CSTARTADD_0

//#define adc_proc_interrupt() SIGNAL(SIG_ADC)

// Settings for DMA :
// DMA Trigger source - ADC interrupt flag
#define DMA0TSEL_6			(6<<0) /* DMA channel 0 transfer select 6:  ADC12IFG */
#define DMA_0_TRIGGER_ADC	DMA0TSEL_6
// DMA addressing modes
#define DMA_ADDR_MODE_BLOCK_BLOCK	(DMASRCINCR_3 | DMADSTINCR_3)
#define DMA_ADDR_MODE_FIXED_BLOCK	(DMASRCINCR_0 | DMADSTINCR_3)
// Default DMA addressing mode - Block to Block
#define DMA_ADDR_MODE	DMA_ADDR_MODE_BLOCK_BLOCK
// DMA Transfer modes
#define DMA_TRANSFER_MODE_BLOCK		DMADT_1
#define DMA_TRANSFER_MODE_SINGLE	DMADT_0
// Default DMA Transfer mode - Block
#define DMA_TRANSFER_MODE			DMA_TRANSFER_MODE_BLOCK
// Source - Word to Destination - Word
#define DMA_SRC_DST_TYPE	DMASWDW

// Settings for Timer A :
// Source clock - SMCLK (set to 1 Mhz in SOS)
//#define TIMERA_SOURCE_CLK	TASSEL_2
// Source clock - ACLK (set to 32 Khz in SOS)
#define TIMERA_SOURCE_CLK	TASSEL_1
// Clock divider: Set to 8, so effective clock rate is 4 Khz
#define TIMERA_CLK_DIVIDE	ID_0
// Output mode - Set/Reset
#define TIMERA_OUTPUT_MODE	OUTMOD_3
// Counting mode - Up
#define TIMERA_COUNT_MODE	MC_1

// Channel map for each registered sensor driver,
// combined with data_ready function pointers in
// driver state.
typedef struct {
	sos_pid_t driver_id;
	sensor_config_t config;
} channel_map_t;

/**
 * possible states for ADC driver
 */
enum {
	ADC_DRIVER_INIT		= 0x01,  	// system uninitalized
	ADC_DRIVER_IDLE		= 0x02,    	// system initalized and idle
	ADC_DRIVER_BUSY		= 0x03,
	ADC_DRIVER_ERROR		= 0x04,   	// error state
	ADC_DRIVER_HW_ON		= 0x80,		// ADC is ON
};


// Status for each sensor data request from application.
typedef enum {
	REQUEST_INIT,
	REQUEST_REGISTERED,
	REQUEST_ACTIVE,
	REQUEST_LAST_EVENT,
	REQUEST_COMPLETE,
} request_status_t;

typedef enum {
	ADC_SINGLE_SAMPLE,
	ADC_PERIODIC_SAMPLE,
} sampling_mode_t;

// Information related to request for sensor data.
typedef struct data_request_t {
	sos_pid_t app_id;
	uint16_t channels;
	uint32_t period;
	uint16_t samples;
	uint16_t event_samples;
	sensor_config_t config;
	request_status_t status;
	void *sensor_context;
	struct data_request_t *next;
} data_request_t;

// FIFO data request queue with a pointer for
// head and tail.
typedef struct {
	data_request_t *head, *tail;
} request_queue_t;

typedef int8_t (*data_ready_func_t)(func_cb_ptr cb, adc_feedback_t fb, sos_pid_t app_id,
                            uint16_t channels, sensor_data_msg_t* buf);
typedef int8_t (*sensor_control_func_t)(func_cb_ptr cb, sensor_driver_command_t command,
                        sensor_id_t sensor, sample_context_t *param, void *context);
typedef int8_t (*sensor_feedback_func_t)(func_cb_ptr cb, sensor_driver_command_t command,
                        uint16_t channel, void *context);

#endif //  

