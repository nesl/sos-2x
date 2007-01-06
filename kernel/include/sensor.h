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
#ifndef _SENSOR_H_
#define _SENSOR_H_

/**
 * @brief sensor type used in ker_sensor_read
 */
//#include <sensorboards.h>

enum {
		SENSOR_CONTROL_FID=0,
		SENSOR_DATA_READY_FID,
};

enum {
		SENSOR_GET_DATA_CMD=0,
		SENSOR_ENABLE_CMD,
		SENSOR_DISABLE_CMD,
		SENSOR_CONFIG_CMD,
};

/**
 * prototype for function calls to sensor drivers
 *
 * the options may change to a void* in the future if 8 bits is not enough context
 */
typedef int8_t (*sensor_func_t)(func_cb_ptr cb, uint8_t cmd, void *context);


#ifndef _MODULE_
#include <sos_types.h>


/**
 * @brief Initialize the sensor interface
 **/
extern int8_t sensor_init();


/**
 * @brief System call for a driver to register a sensor with its own policy
 * @param sensor_driver_pid The module ID of the driver
 * @param sensor_id Type of the sensor being registered
 * @param get_data_fid The function pointer pointer FID of the get data function implemented by the driver
 */
extern int8_t ker_sensor_register(sos_pid_t sensor_driver_pid, uint8_t sensor_id, uint8_t sensor_fid, void* data);


/**
 * @param sensor_id Type of the sensor being deregistered
 * @param get_data_fid The function pointer pointer FID of the get data function implemented by the driver
 */
extern int8_t ker_sensor_deregister(sos_pid_t sensor_driver_pid, uint8_t sensor_id);


/**
 * @brief System call for removing driver from sensor API
 * @param sensor_driver_pid The module ID of the driver
 * @brief Micro-reboot all sensor drivers of a particular pid
 */
extern int8_t sensor_micro_reboot(sos_pid_t pid);


/**
 * @brief System call for enabling aqusition of from a sensor
 * @param client_pid The process id of the module enabling the sensor
 * @param sensor_id of the sensor whose data is being enabled
 */
extern int8_t ker_sensor_enable(sos_pid_t client_pid, uint8_t sensor_id);


/**
 * @brief System call for enabling aqusition of from a sensor
 * @param client_pid The process id of the module enabling the sensor
 * @param sensor_id of the sensor whose data is being disaabled
 */
extern int8_t ker_sensor_disable(sos_pid_t client_pid, uint8_t sensor_id);


/**
 * @brief System call for enabling aqusition of from a sensor
 * @param client_pid The process id of the module enabling the sensor
 * @param sensor_id of the sensor whose data is being disaabled
 * @param sensor_new_state pointer to a dynamicly allocated state struct
 */
extern int8_t ker_sensor_control(sos_pid_t client_pid, uint8_t sensor_id, void* sensor_new_state);


/**
 * @brief System call for getting data from the sensor
 * @param client_pid The process id of the module requesting sensor data
 * @param sensor_id Type of the sensor whose data is being requested
 */
extern int8_t ker_sensor_get_data(sos_pid_t client_pid, uint8_t sensor_id);


/**
 * @brief Data ready system call for signalling to the kernel, the receipt of new sensor data
 * @param sensor_id The type of the sensor
 * @param sensor_data The value of data received from the sensor
 */
extern int8_t ker_sensor_data_ready(uint8_t sensor_id, uint16_t sensor_data, uint8_t status);


/**
 * @brief remove all sensor drivers of particular pid
 */
int8_t sensor_remove_all(sos_pid_t pid);

#ifdef FAULT_TOLERANT_SOS
/**
 * @brief Micro-reboot all sensor drivers of a particular pid
 */
extern int8_t sensor_micro_reboot(sos_pid_t pid);
#endif

#endif // _MODULE_

#endif // _SENSOR_H_
