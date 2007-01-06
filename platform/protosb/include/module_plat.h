/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/** 
 * @brief Header file for protoSB specific modules
 */
#ifndef _MODULE_PLAT_H
#define _MODULE_PLAT_H

#include <kertable_plat.h>
#include <bus_cb.h>
#include <ads8341_func_adc.h>

#ifdef _MODULE_

#include <hardware_types.h>

/**
 * @brief SPI bus arbitration functions
 * @param pid Process id requesting/freeing bus
 */
typedef int8_t (*ker_spi_reserve_bus_func_t)(sos_pid_t pid, spi_addr_t addr, uint8_t flags);
static inline int8_t ker_spi_reserve_bus(sos_pid_t pid, spi_addr_t addr, uint8_t flags)
{
	ker_spi_reserve_bus_func_t func = (ker_spi_reserve_bus_func_t)get_kertable_entry(PROC_KERTABLE_END+1);
	return func(pid, addr, flags);
}


typedef int8_t (*ker_spi_release_bus_func_t)(sos_pid_t pid);
static inline int8_t ker_spi_release_bus(sos_pid_t pid)
{
	ker_spi_release_bus_func_t func = (ker_spi_release_bus_func_t)get_kertable_entry(PROC_KERTABLE_END+2);
	return func(pid);
}


typedef int8_t (*ker_spi_send_data_func_t)(
		uint8_t *buf,
		uint8_t buf_len,
		uint8_t flags,
		sos_pid_t pid);
static inline int8_t ker_spi_send_data(
		uint8_t *msg,
		uint8_t msg_size,
		uint8_t flags,
		sos_pid_t pid) {
	ker_spi_send_data_func_t func = (ker_spi_send_data_func_t)get_kertable_entry(PROC_KERTABLE_END+3);
	return func(msg, msg_size, flags, pid);
}


typedef int8_t (*ker_spi_read_data_func_t)(
		uint8_t *sharedBuf,
		uint8_t read_size,
		uint8_t flags,
		sos_pid_t pid);
static inline int8_t ker_spi_read_data(
		uint8_t *sharedBuf,
		uint8_t read_size,
		uint8_t flags,
		sos_pid_t pid) {
	ker_spi_read_data_func_t func = (ker_spi_read_data_func_t)get_kertable_entry(PROC_KERTABLE_END+4);
	return func(sharedBuf, read_size, flags, pid);
}


/**
 * @brief Voltage Refefence control
 */
typedef int8_t (*ker_vref_func_t)(uint8_t action);
static inline int8_t ker_vref(uint8_t action){
	ker_vref_func_t func = (ker_vref_func_t)get_kertable_entry(PROC_KERTABLE_END+5);
	return func(action);
}


/**
 * @brief Prototyping Preamp control
 */
typedef int8_t (*ker_preamp_func_t)(uint8_t action);
static inline int8_t ker_preamp(uint8_t action){
	ker_preamp_func_t func = (ker_preamp_func_t)get_kertable_entry(PROC_KERTABLE_END+6);
	return func(action);
}


/**
 * @brief ADC Bind Port
 **/
static inline int8_t ker_ads8341_adc_bindPort(uint8_t port, uint8_t adcPort, uint8_t driverpid){
  ker_ads8341_adc_bindPort_func_t func = (ker_ads8341_adc_bindPort_func_t)get_kertable_entry(PROC_KERTABLE_END+7);
  return func(port, adcPort, driverpid);
}


/**
 * @brief ADC get Data
 **/
static inline int8_t ker_ads8341_adc_getData(uint8_t port){
  ker_ads8341_adc_getData_func_t func = (ker_ads8341_adc_getData_func_t)get_kertable_entry(PROC_KERTABLE_END+8);
  return func(port);
}


/**
 * Get Perodic ADC data
 */
static inline int8_t ker_ads8341_adc_getPerodicData(uint8_t port, uint8_t prescaler, uint8_t interval, uint8_t count){
  ker_ads8341_adc_getPerodicData_func_t func = (ker_ads8341_adc_getPerodicData_func_t)get_kertable_entry(PROC_KERTABLE_END+9);
  return func(port, prescaler, interval, count);
}


/**
 * Get Perodic ADC data using DMA
 */
static inline int8_t ker_ads8341_adc_getDataDMA(uint8_t port, uint8_t prescaler, uint8_t interval, uint8_t *sharedMemory, uint8_t count){
  ker_ads8341_adc_getDataDMA_func_t func = (ker_ads8341_adc_getDataDMA_func_t)get_kertable_entry(PROC_KERTABLE_END+10);
  return func(port, prescaler, interval, sharedMemory, count);
}


/**
 * Stop Perodic ADC data
 */
static inline int8_t ker_ads8341_adc_stopPerodicData(uint8_t port){
  ker_ads8341_adc_stopPerodicData_func_t func = (ker_ads8341_adc_stopPerodicData_func_t)get_kertable_entry(PROC_KERTABLE_END+11);
  return func(port);
}


/**
 * @brief Set Preamp Gain
 */
typedef int8_t (*ker_ltc6915_amp_setGain_func_t)(uint8_t calling_id, uint8_t gain);
static inline int8_t ker_ltc6915_amp_setGain(uint8_t calling_id, uint8_t gain){
  ker_ltc6915_amp_setGain_func_t func = (ker_ltc6915_amp_setGain_func_t)get_kertable_entry(PROC_KERTABLE_END+12);
  return func(calling_id, gain);
}


/**
 * @brief Voltage Regulator control
 */
typedef int8_t (*ker_vreg_func_t)(uint8_t action);
static inline int8_t ker_vreg(uint8_t action){
	ker_vreg_func_t func = (ker_vreg_func_t)get_kertable_entry(PROC_KERTABLE_END+13);
	return func(action);
}


/**
 * @brief FET Switches control
 */
typedef int8_t (*ker_switches_func_t)(uint8_t action);
static inline int8_t ker_switches(uint8_t action){
	ker_switches_func_t func = (ker_switches_func_t)get_kertable_entry(PROC_KERTABLE_END+14);
	return func(action);
}


/**
 * @brief Set Input MUX channel
 */
typedef int8_t (*ker_mux_set_func_t)(sos_pid_t pid, uint8_t channel);
static inline int8_t ker_mux_set(sos_pid_t pid, uint8_t channel){
	ker_mux_set_func_t func = (ker_mux_set_func_t)get_kertable_entry(PROC_KERTABLE_END+15);
	return func(pid, channel);
}


/**
 * @brief Read Input MUX
 */
typedef int8_t (*ker_mux_read_func_t)(sos_pid_t pid);
static inline int8_t ker_mux_read(sos_pid_t pid){
	ker_mux_read_func_t func = (ker_mux_read_func_t)get_kertable_entry(PROC_KERTABLE_END+16);
	return func(pid);
}


/**
 * @brief Register SPI read and send callbacks
 */
typedef int8_t (*ker_spi_register_read_cb_t)(sos_pid_t pid, bus_read_done_cb_t cb);
typedef int8_t (*ker_spi_register_send_cb_t)(sos_pid_t pid, bus_send_done_cb_t cb);
typedef int8_t (*ker_spi_unregister_cb_t)(sos_pid_t pid);
static inline int8_t ker_spi_register_read_cb(sos_pid_t pid, bus_read_done_cb_t cb){
	ker_spi_register_read_cb_t func = (ker_spi_register_read_cb_t)get_kertable_entry(PROC_KERTABLE_END+17);
	return func(pid,cb);
}
static inline int8_t ker_spi_unregister_read_cb(sos_pid_t pid){
	ker_spi_unregister_cb_t func = (ker_spi_unregister_cb_t)get_kertable_entry(PROC_KERTABLE_END+18);
	return func(pid);
}
static inline int8_t ker_spi_register_send_cb(sos_pid_t pid, bus_send_done_cb_t cb){
	ker_spi_register_send_cb_t func = (ker_spi_register_send_cb_t)get_kertable_entry(PROC_KERTABLE_END+19);
	return func(pid,cb);
}
static inline int8_t ker_spi_unregister_send_cb(sos_pid_t pid){
	ker_spi_unregister_cb_t func = (ker_spi_unregister_cb_t)get_kertable_entry(PROC_KERTABLE_END+20);
	return func(pid);
}


#endif /* ifdef _MODULE_ */
#endif /* #ifndef _MODULE_PLAT_H */

