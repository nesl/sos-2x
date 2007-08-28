#ifndef _SENSOR_PRIVATE_H_
#define _SENSOR_PRIVATE_H_

#include <sos_types.h>
#include <sensor_api_driver.h>
#include <sensor_system.h>

// This file is required for ker_* definitions as ker_* 
// functions are accessed by corresponding ker_sys_* wrappers
// in sensor_system.c
#include <sensor_kernel.h>

// This file contains private definitions for sensor API.
typedef int8_t (*sensor_control_fn_t)(func_cb_ptr cb, sensor_driver_command_t command, 
		sos_pid_t app_id, sensor_id_t sensor, sample_context_t *param, void *context);

#endif

