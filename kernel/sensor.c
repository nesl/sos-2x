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
 */
#include <sos.h>
#include <fntable_types.h>
#include <message.h>

#include "sensor.h"

#ifndef SOS_DEBUG_SENSOR_API
#undef DEBUG
#define DEBUG(...)
#endif
//! Maximum types of sensors supported by SOS
#define MAX_SENSOR_ID  10

/**
 * @brief Private state of the sensor module
 */
typedef struct sensor_state {
	sos_pid_t pid;               //! Process id of the sensor driver
	sos_pid_t client_pid;        //! Client module requesting the sensor data
	void *ctx;     //! context that is saved when the sensor is registered
} sensor_state_t;
static sensor_state_t st[MAX_SENSOR_ID];

// need to be declared seperatly because of how SOS does function mapping
static func_cb_ptr sensor_func_ptr[MAX_SENSOR_ID];

static int8_t sensor_handler(void *state, Message *msg);

static sos_module_t sensor_module;

static mod_header_t mod_header SOS_MODULE_HEADER =
{
	mod_id : KER_SENSOR_PID,
	state_size : 0,
	num_prov_func : 0,
	num_sub_func : MAX_SENSOR_ID,
	module_handler: sensor_handler,
	funct : {
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
	},
};


static int8_t sensor_handler(void *state, Message *msg)
{
	return -EINVAL;
}


/**
 * @brief Initialize the sensor interface
 */
int8_t sensor_init() {
  uint8_t i;

  for (i = 0; i < MAX_SENSOR_ID; i++){
		st[i].pid = NULL_PID;
		st[i].client_pid = NULL_PID;
		st[i].ctx = NULL;
	}
	sched_register_kernel_module(&sensor_module, sos_get_header_address(mod_header), sensor_func_ptr);
	return SOS_OK;
}


/**
 * @brief Register a new sensor driver
 */
int8_t ker_sensor_register(sos_pid_t calling_id, uint8_t sensor_id, uint8_t sensor_fid, void *ctx) {

	if (sensor_id > MAX_SENSOR_ID) {
		return -EINVAL;
	}
	if(st[sensor_id].pid != NULL_PID) {
		return -EBUSY;
	}

	// try to register all necessary function calls
	// if any fail do cleanup
	if(ker_fntable_subscribe(KER_SENSOR_PID, calling_id, sensor_fid, sensor_id) < 0) {
		return -EINVAL;
	}
	
  st[sensor_id].ctx = ctx;
  st[sensor_id].pid = calling_id;

  return SOS_OK;
}


/**
 * @brief De-Register a sensor driver
 */
//! XXX: We are not registering the get_data function, should be de-register it ?
int8_t ker_sensor_deregister(sos_pid_t calling_id, uint8_t sensor_id) {

	if ((sensor_id > MAX_SENSOR_ID) || (st[sensor_id].pid != calling_id)) {
		return -EINVAL;
	}

	// disable sensor before unregistering function calls
  SOS_CALL(sensor_func_ptr[sensor_id], sensor_func_t, SENSOR_DISABLE_CMD,  st[sensor_id].ctx);

	//sensor_func_ptr[sensor_id] = NULL;

  st[sensor_id].pid = NULL_PID;
  st[sensor_id].ctx = NULL;

  return SOS_OK;
}


/**
 * @brief Get the sensor data
 */
int8_t ker_sensor_get_data(sos_pid_t calling_id, uint8_t sensor_id) 
{
	int8_t ret;
	if ((sensor_id > MAX_SENSOR_ID) || (st[sensor_id].pid == NULL_PID) || (st[sensor_id].client_pid != NULL_PID)) {
		return -EINVAL;
	}

	st[sensor_id].client_pid = calling_id;  //changed

	ret = SOS_CALL(sensor_func_ptr[sensor_id], sensor_func_t, SENSOR_GET_DATA_CMD, st[sensor_id].ctx);
	if (SOS_OK != ret) {
		//! XXX ????
		st[sensor_id].client_pid = NULL_PID; //changed
		return -EINVAL;
	}
	

  return SOS_OK;
}

int8_t ker_sys_sensor_get_data( uint8_t sensor_id )
{
	int8_t ret;
	sos_pid_t calling_id = ker_get_current_pid();

	if ((sensor_id > MAX_SENSOR_ID) || (st[sensor_id].pid == NULL_PID) || (st[sensor_id].client_pid != NULL_PID)) {
		return -EINVAL;
	}
	st[sensor_id].client_pid = calling_id;  //changed

	ret = SOS_CALL(sensor_func_ptr[sensor_id], sensor_func_t, SENSOR_GET_DATA_CMD, st[sensor_id].ctx);
	if (SOS_OK != ret) {
		//! XXX ????
		st[sensor_id].client_pid = NULL_PID; //changed
		return -EINVAL;
	}
	return SOS_OK;
}


/**
 * @brief enable the sensor
 */
int8_t ker_sensor_enable(sos_pid_t calling_id, uint8_t sensor_id) 
{
	int8_t ret;
	if ((sensor_id > MAX_SENSOR_ID) || (st[sensor_id].pid == NULL_PID) || (st[sensor_id].client_pid != NULL_PID)) {
		return -EINVAL;
	}

	ret = SOS_CALL(sensor_func_ptr[sensor_id], sensor_func_t, SENSOR_ENABLE_CMD, st[sensor_id].ctx);
	if (SOS_OK != ret) {
		//! XXX ????
		return -EINVAL;
	}

  return SOS_OK;
}


/**
 * @brief disable the sensor
 */
int8_t ker_sensor_disable(sos_pid_t calling_id, uint8_t sensor_id) 
{
	int8_t ret;
	if ((sensor_id > MAX_SENSOR_ID) || (st[sensor_id].pid == NULL_PID) || (st[sensor_id].client_pid != NULL_PID)) {
		return -EINVAL;
	}

	ret = SOS_CALL(sensor_func_ptr[sensor_id], sensor_func_t, SENSOR_DISABLE_CMD, st[sensor_id].ctx);
	if (SOS_OK != ret) {
		//! XXX ????
		return -EINVAL;
	}
	st[sensor_id].client_pid = NULL_PID;

  return SOS_OK;
}


/**
 * @brief reconfigure the sensor
 */
int8_t ker_sensor_control(sos_pid_t calling_id, uint8_t sensor_id, void* sensor_new_state) 
{
	int8_t ret;
	if ((sensor_id > MAX_SENSOR_ID) || (st[sensor_id].pid == NULL_PID) || (st[sensor_id].client_pid != NULL_PID)) {
		return -EINVAL;
	}

	ret = SOS_CALL(sensor_func_ptr[sensor_id], sensor_func_t, SENSOR_CONFIG_CMD, sensor_new_state);
	if (SOS_OK != ret) {
		//! XXX ????
		return -EINVAL;
	}

  return SOS_OK;
}





/**
 * @brief The data ready message to the application
 */
int8_t ker_sensor_data_ready(uint8_t sensor_id, uint16_t sensor_data, uint8_t status) {

	if ((sensor_id > MAX_SENSOR_ID) || (NULL_PID == st[sensor_id].pid) || (NULL_PID == st[sensor_id].client_pid)) {
		return -EINVAL;
	}
	
	//! There is no need to make this message a high priority one as it is the sampling which needs to be done asap and not the delivery
	if ((0x3f & status) != 0) {
		post_short(st[sensor_id].client_pid, KER_SENSOR_PID, MSG_ERROR, sensor_id, sensor_data, 0);
	} else {
		post_short(st[sensor_id].client_pid, KER_SENSOR_PID, MSG_DATA_READY, sensor_id, sensor_data, 0);
	}
	st[sensor_id].client_pid = NULL_PID;

  return SOS_OK;
}


int8_t sensor_remove_all(sos_pid_t pid)
{
	uint8_t i;

	for(i = 0; i < MAX_SENSOR_ID; i++) {
		if(st[i].pid == pid) {
			ker_sensor_deregister(st[i].pid, i);
		}
	}
	return SOS_OK;
}


#ifdef FAULT_TOLERANT_SOS
int8_t sensor_micro_reboot(sos_pid_t pid)
{
   uint8_t i;
   for (i = 0; i < MAX_SENSOR_ID; i++){
      if ((st[i].pid == pid) & 
          (st[i].client_pid != NULL_PID)){
         //!XXX Ram - Need to send a response to any waiting client
         // to make the recovery seamless.
         // But what would be a good value to return ?
         sensor_data_ready(i, 0xFFFF);   
      }
   }
   return SOS_OK;
}
#endif

