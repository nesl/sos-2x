
#ifndef _MODULE_PROC_H
#define _MODULE_PROC_H

#include <kertable_proc.h>

#if 0
#ifdef _MODULE_

/**
 * @brief ADC Bind Port
 **/
typedef int8_t (*adc_bindPort_func_t)(uint8_t port, uint8_t adcPort, sos_pid_t calling_id, uint8_t cb_fid);
static inline int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcPort, sos_pid_t calling_id, uint8_t cb_fid)
{
	adc_bindPort_func_t func = (adc_bindPort_func_t)get_kertable_entry(SYS_KERTABLE_END+1);
	return func(port, adcPort, calling_id, cb_fid);
}


/**
 * @brief ADC UnBind Port
 **/
typedef int8_t (*adc_unbindPort_func_t)(uint8_t port, sos_pid_t calling_id);
static inline int8_t ker_adc_proc_unbindPort(uint8_t port, sos_pid_t calling_id)
{
	adc_unbindPort_func_t func = (adc_unbindPort_func_t)get_kertable_entry(SYS_KERTABLE_END+2);
	return func(port, calling_id);
}

/**
 * Get data from ADC
 **/
typedef int8_t (*adc_getData_func_t)(uint8_t port, uint8_t flags);
static inline int8_t ker_adc_proc_getData(uint8_t port, uint8_t flags)
{
	adc_getData_func_t func = (adc_getData_func_t)get_kertable_entry(SYS_KERTABLE_END+3);
	return func(port, flags);
}

/**
 * Get periodic data from ADC
 */
typedef int8_t (*adc_getPeriodicData_func_t)(uint8_t port, uint8_t prescaler, uint16_t count);
static inline int8_t ker_adc_proc_getPeriodicData(uint8_t port, uint8_t prescaler, uint16_t count)
{
	adc_getPeriodicData_func_t func = (adc_getPeriodicData_func_t)get_kertable_entry(SYS_KERTABLE_END+4);
	return func(port, prescaler, count);
}


/**
 * Stop periodic ADC data
 */
typedef int8_t (*adc_stopPeriodicData_func_t)(uint8_t port);
static inline int8_t ker_adc_proc_stopPerodicData(uint8_t port)
{
	adc_stopPeriodicData_func_t func = (adc_stopPeriodicData_func_t)get_kertable_entry(SYS_KERTABLE_END+5);
	return func(port);
}


#endif /* ifdef _MODULE_ */
#endif
#endif

