/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#ifndef _ADC_DRIVER_KERNEL_H_
#define _ADC_DRIVER_KERNEL_H_

// This header file is meant to be included only by sensor drivers
// implemented as kernel extensions that use the ADC interface directly.

#include <sos_types.h>
#include <adc_driver_common.h>

/** 
 * @brief init function for ADC
 */
extern int8_t adc_driver_init();

extern int8_t ker_adc_bind_channel (sos_pid_t driver_id, uint16_t channels, uint8_t control_cb_fid, 
									uint8_t cb_fid, void *config);
extern int8_t ker_adc_unbind_channel (sos_pid_t driver_id, uint16_t channels);
extern int8_t ker_adc_get_data (uint8_t command, sos_pid_t app_id, uint16_t channels, 
						sample_context_t *param, void *context);
extern int8_t ker_adc_stop_data (sos_pid_t app_id, uint16_t channels);

#endif 

