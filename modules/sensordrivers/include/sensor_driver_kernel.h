/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
	  
#ifndef _SENSOR_DRIVER_KERNEL_H_
#define _SENSOR_DRIVER_KERNEL_H_

// This file should only be included by sensor driver
// modules implemented as kernel extensions ONLY.

#include <sensor_api_driver.h>
#include <sensor_driver_private.h>
#include <sos_types.h>

/**
 * @brief System call for a driver to register a sensor with its own policy
 * @param sensor_driver_pid The module ID of the driver
 * @param sensor_id Type of the sensor being registered
 * @param get_data_fid The function pointer pointer FID of the get data function implemented by the driver
 */
extern int8_t ker_sensor_driver_register(sos_pid_t sensor_driver_pid, uint8_t sensor_id, uint8_t sensor_fid, void* data);


/**
 * @param sensor_id Type of the sensor being deregistered
 * @param get_data_fid The function pointer pointer FID of the get data function implemented by the driver
 */
extern int8_t ker_sensor_driver_deregister(sos_pid_t sensor_driver_pid, uint8_t sensor_id);

#endif // _SENSOR_H_
