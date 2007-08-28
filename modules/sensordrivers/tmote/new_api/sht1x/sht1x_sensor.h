/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */

/**
 * Driver Support for the SHT11
 * 
 * This module currently provides support for measuring
 * temperature and humidity from the SHT11 chip.
 * 
 * \author Kapy Kangombe, John Hicks, James Segedy {jsegedy@gmail.com}
 * \date 07-2005
 * Ported driver from TinyOS to SOS
 *
 * \author Roy Shea (roy@cs.ucla.edu)
 * \date 06-2006
 * Ported driver to current version of SOS
 *
 * \author Thomas Schmid (thomas.schmid@ucla.edu)
 * \date 07-2007
 * Ported driver from SHT11 to SHT11
 */

#ifndef _SHT1x_SENSOR_H_
#define _SHT1x_SENSOR_H_

#include "sht1x.h"

typedef int8_t (*sht1x_get_data_func_t)(func_cb_ptr cb, sht1x_sensor_command_t command, 
	sos_pid_t app_id, uint16_t channels, sample_context_t *param, void *context);
typedef int8_t (*sht1x_stop_data_func_t)(func_cb_ptr cb, sos_pid_t app_id, uint16_t channels);


#endif // _SHT11_H_

