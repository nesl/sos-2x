#ifndef _ADC_H_
#define _ADC_H_

#include <sos_types.h>

typedef int8_t (*sys_adc_bind_port_ker_func_t)(uint8_t port, uint8_t adcPort, sos_pid_t calling_id, uint8_t cb_fid);

static inline int8_t sys_adc_bind_port(uint8_t port, uint8_t adcPort, sos_pid_t calling_id, uint8_t cb_fid) {
	return ((sys_adc_bind_port_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*31))
					(port, adcPort, calling_id, cb_fid);
}

typedef int8_t (*sys_adc_unbind_port_ker_func_t)(uint8_t port, sos_pid_t pid);

static inline int8_t sys_adc_unbind_port(uint8_t port, sos_pid_t pid) {
	return ((sys_adc_unbind_port_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*32))(port, pid);
}

typedef int8_t (*sys_adc_get_data_ker_func_t)(uint8_t port, uint8_t flags);

static inline int8_t sys_adc_get_data(uint8_t port, uint8_t flags) {
	return ((sys_adc_get_data_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*33))(port, flags);
}

typedef int8_t (*sys_adc_get_periodic_data_ker_func_t)(uint8_t port, uint8_t prescaler, uint16_t count);

static inline int8_t sys_adc_get_periodic_data(uint8_t port, uint8_t prescaler, uint16_t count) {
	return ((sys_adc_get_periodic_data_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*34))(port, prescaler, count);
}

typedef int8_t (*sys_adc_stop_periodic_data_ker_func_t)(uint8_t port);

static inline int8_t sys_adc_stop_periodic_data(uint8_t port) {
	return ((sys_adc_stop_periodic_data_ker_func_t)(SYS_JUMP_TBL_START+SYS_JUMP_TBL_SIZE*35))(port);
}

#endif
