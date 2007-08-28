/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
	  
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: sensor.h,v 1.6 2006/08/30 22:24:51 martinm Exp $ 
 */
#ifndef _SENSOR_KERNEL_H_
#define _SENSOR_KERNEL_H_

// This file exposes the sensor API to applications written as kernel extensions.

#include <sensor_system.h>
#include <sos_types.h>

/**
 * @brief Initialize the sensor interface
 **/
int8_t sensor_system_init();

/**
 * @brief System call for getting data from the sensor
 * @param sensor_id Type of the sensor whose data is being requested
 */
// Functions used by sensor drivers
int8_t ker_sensor_driver_register(sos_pid_t driver_id, sensor_id_t sensor, uint8_t sensor_control_fid);

int8_t ker_sensor_driver_deregister(sos_pid_t driver_id, sensor_id_t sensor);

// Functions used by application modules
int8_t ker_sensor_start_sampling(sos_pid_t app_id, sensor_id_t *sensors, unsigned int num_sensors, 
						sample_context_t *param, void *context);

int8_t ker_sensor_stop_sampling(sos_pid_t app_id, sensor_id_t sensor);

/**
 * @brief remove all sensor drivers of particular pid
 */
int8_t sensor_remove_all(sos_pid_t pid);

#ifdef FAULT_TOLERANT_SOS
/**
 * @brief Micro-reboot all sensor drivers of a particular pid
 */
int8_t sensor_micro_reboot(sos_pid_t pid);
#endif

#endif // _SENSOR_KERNEL_H_

