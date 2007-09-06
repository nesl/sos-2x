/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sos.h>
#include <sensor_private.h>
#ifdef SOS_USE_PREEMPTION
#include <priority.h>
#endif

#ifndef SOS_DEBUG_SENSOR_API
#undef DEBUG
#define DEBUG(...)
#endif


typedef struct {
	sos_pid_t driver_id[MAX_NUM_SENSORS];		//! Process id of the sensor driver
} sensor_state_t;

static sensor_state_t s;

// need to be declared seperatly because of how SOS does function mapping
#ifdef SOS_USE_PREEMPTION
static func_cb_ptr *sensor_func_ptr;
#else
static func_cb_ptr sensor_func_ptr[MAX_NUM_SENSORS];
#endif

static int8_t sensor_handler(void *state, Message *msg);

#ifndef SOS_USE_PREEMPTION
static sos_module_t sensor_module;
#endif

static mod_header_t mod_header SOS_MODULE_HEADER =
{
	mod_id : KER_SENSOR_PID,
#ifdef SOS_USE_PREEMPTION
	state_size : sizeof(func_cb_ptr) * MAX_NUM_SENSORS,
#else
	state_size : 0,
#endif
	num_prov_func : 0,
	num_sub_func : MAX_NUM_SENSORS,
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
	},
};


static int8_t sensor_handler(void *state, Message *msg)
{
	return -EINVAL;
}


/**
 * @brief Initialize the sensor interface
 */
int8_t sensor_system_init() {
	unsigned int i;

	for (i = 0; i < MAX_NUM_SENSORS; i++) {
		s.driver_id[i] = NULL_PID;
	}
#ifdef SOS_USE_PREEMPTION
	ker_register_module(sos_get_header_address(mod_header));
	sensor_func_ptr = ker_get_module_state(KER_SENSOR_PID);
#else
	sched_register_kernel_module(&sensor_module, sos_get_header_address(mod_header), sensor_func_ptr);
#endif
	return SOS_OK;
}

/**
 * @brief Register a new sensor driver
 */
int8_t ker_sys_sensor_driver_register(sensor_id_t sensor, uint8_t sensor_control_fid) {
	sos_pid_t driver_id = ker_get_current_pid();
	return ker_sensor_driver_register(driver_id, sensor, sensor_control_fid);
}

int8_t ker_sensor_driver_register(sos_pid_t driver_id, sensor_id_t sensor, uint8_t sensor_control_fid) {
	// Verify driver id and sensor id.
	// Verify if the sensor has already been registered.
	if ((driver_id == NULL_PID) || 
		(sensor >= MAX_NUM_SENSORS) || 
		(s.driver_id[sensor] != NULL_PID)) {
		return -EINVAL;
	}

	// Subscribe to sensor control function.
	if (ker_fntable_subscribe(KER_SENSOR_PID, driver_id, sensor_control_fid, sensor) < 0) {
		return -EINVAL;
	}

	// Registration successful.
	s.driver_id[sensor] = driver_id;

	return SOS_OK;
}


int8_t ker_sys_sensor_driver_deregister(sensor_id_t sensor) {
	sos_pid_t driver_id = ker_get_current_pid();
	return ker_sensor_driver_deregister(driver_id, sensor);
}

/**
 * @brief De-Register a sensor driver
 */
int8_t ker_sensor_driver_deregister(sos_pid_t driver_id, sensor_id_t sensor) {
	// Verify driver id and sensor id.
	// Verify if the sensor has already been registered.
	if ((driver_id == NULL_PID) || 
		(sensor >= MAX_NUM_SENSORS) || 
		(s.driver_id[sensor] == NULL_PID)) {
		return -EINVAL;
	}

	// Driver de-registered.
	s.driver_id[sensor] = NULL_PID;

	return SOS_OK;
}


int8_t ker_sys_sensor_start_sampling(sensor_id_t *sensors, unsigned int num_sensors, 
						sample_context_t *param, void *context) {
	sos_pid_t app_id = ker_get_current_pid();
	return ker_sensor_start_sampling(app_id, sensors, num_sensors, param, context);
}

/**
 * @brief Get the sensor data
 */
int8_t ker_sensor_start_sampling(sos_pid_t app_id, sensor_id_t *sensors, unsigned int num_sensors, 
						sample_context_t *param, void *context) {
	unsigned int i;

	// Verify application id.
	if (app_id == NULL_PID) return -EINVAL;

	// Verify sensor ids and the sensor drivers that support these
	// sensors are registered.
	for (i = 0; i < num_sensors; i++) {
		if ((sensors[i] >= MAX_NUM_SENSORS) || 
			(s.driver_id[sensors[i]] == NULL_PID)) { 
			return -EINVAL;
		}
	}

	if (param == NULL) return -EINVAL;

	// Register request to sensor drivers.
	for (i = 0; i < num_sensors; i++) {
		int8_t ret = SOS_CALL(sensor_func_ptr[sensors[i]], sensor_control_fn_t, SENSOR_REGISTER_REQUEST_COMMAND, 
							app_id, sensors[i], param, context);
		if (ret < 0) {
			// Remove request from other drivers too if there is some error
			// while registering it.
			int j;
			for (j = i-1; j > -1; j--) {
				SOS_CALL(sensor_func_ptr[sensors[j]], sensor_control_fn_t, SENSOR_REMOVE_REQUEST_COMMAND, 
						app_id, sensors[j], param, context);
			}
			return ret;
		}
	}

	// Issue GET_DATA command to sensor drivers.
	for (i = 0; i < num_sensors; i++) {
		SOS_CALL(sensor_func_ptr[sensors[i]], sensor_control_fn_t, SENSOR_GET_DATA_COMMAND, 
				app_id, sensors[i], param, context);
	}

	return SOS_OK;
}


int8_t ker_sys_sensor_stop_sampling(sensor_id_t sensor) { 
	sos_pid_t app_id = ker_get_current_pid();
	return ker_sensor_stop_sampling(app_id, sensor);
}

int8_t ker_sensor_stop_sampling(sos_pid_t app_id, sensor_id_t sensor) {
	if ((app_id == NULL_PID) ||
		(sensor > MAX_NUM_SENSORS) ||
		(s.driver_id[sensor] != NULL_PID)) {
		return -EINVAL;
	}

	int8_t ret = SOS_CALL(sensor_func_ptr[sensor], sensor_control_fn_t, SENSOR_STOP_DATA_COMMAND, 
						app_id, sensor, NULL, NULL);

	return ret;
}

#ifdef NEW_SENSING_API
// Function only used by scheduler.
int8_t sensor_remove_all(sos_pid_t pid) {
	if (pid == NULL_PID) return -EINVAL;
	uint8_t i = 0;

	for (i = 0; i < MAX_NUM_SENSORS; i++) {
		if (s.driver_id[i] == pid) {
			ker_sensor_driver_deregister(pid, i);
		}
	}

	return SOS_OK;
}
#endif

