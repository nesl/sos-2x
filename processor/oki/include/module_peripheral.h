
#ifndef _MODULE_PERIPHERAL_H
#define _MODULE_PERIPHERAL_H

/**
 * Enable Radio Link Layer Ack */
typedef void (*ker_radio_ack_func_t)(void);
static inline void ker_radio_ack_enable(void)
{
	ker_radio_ack_func_t func = (ker_radio_ack_func_t)get_kertable_entry(64);
	func();
	return;
}

/**
 * Disable Radio Link Layer Ack
 */
static inline void ker_radio_ack_disable(void)
{
	ker_radio_ack_func_t func = (ker_radio_ack_func_t)get_kertable_entry(65);
	func();
	return;
}

/**
 * @brief ADC Bind Port
 **/
typedef int8_t (*adc_bindPort_func_t)(uint8_t port, uint8_t adcPort, uint8_t driverpid);
static inline int8_t ker_adc_proc_bindPort(uint8_t port, uint8_t adcPort, uint8_t driverpid)
{
	adc_bindPort_func_t func = (adc_bindPort_func_t)get_kertable_entry(66);
	return func(port, adcPort, driverpid);
}


typedef int8_t (*adc_getData_func_t)(uint8_t port);
static inline int8_t ker_adc_proc_getData(uint8_t port)
{
	adc_getData_func_t func = (adc_getData_func_t)get_kertable_entry(67);
	return func(port);
}

/**
 * Get continuous data from ADC
 */
static inline int8_t ker_adc_proc_getContinuousData(uint8_t port)
{
	adc_getData_func_t func = (adc_getData_func_t)get_kertable_entry(68);
	return func(port);
}


/**
 * Get Calibrated ADC data
 */
static inline int8_t ker_adc_proc_getCalData(uint8_t port)
{
	adc_getData_func_t func = (adc_getData_func_t)get_kertable_entry(69);
	return func(port);
}


/**
 * Get Continuous Calibrated ADC data
 */
static inline int8_t ker_adc_proc_getCalContinuousData(uint8_t port)
{
	adc_getData_func_t func = (adc_getData_func_t)get_kertable_entry(70);
	return func(port);
}

/**
 * @brief I2C slave send data function.
 * @param msg Pointer to the data to send
 * @param msg_size Length of data to send
 **/
typedef int8_t (*i2c_slave_start_func_t)(uint8_t *msg, uint8_t msg_size);
static inline int8_t ker_i2c_slave_start(uint8_t *msg, uint8_t msg_size){
	i2c_slave_start_func_t func = (i2c_slave_start_func_t)get_kertable_entry(71);
	return func(msg, msg_size);
}

/**
 * @brief I2C send data function.
 * @param msg Pointer to the data to send
 * @param msg_size Length of data to send
 **/
typedef int8_t (*i2c_send_data_func_t)(uint8_t i2c_addr,
		 uint8_t *msg,
		 uint8_t msg_size,
		 uint8_t calling_id);
static inline int8_t ker_i2c_send_data(uint8_t i2c_addr,
		uint8_t *msg,
		uint8_t msg_size,
		uint8_t calling_id)
{
	i2c_send_data_func_t func = (i2c_send_data_func_t)get_kertable_entry(72);
	return func(i2c_addr, msg, msg_size, calling_id);
}

/**
 * @brief I2C read data function. Split phase call. Reply is I2C_READ_TASK
 * @param msg_size Length of data to read
 **/
typedef int8_t (*i2c_read_data_func_t)(uint8_t i2c_addr,
		uint8_t msg_size,
		uint8_t calling_id);
static inline int8_t ker_i2c_read_data(uint8_t i2c_addr,
		uint8_t msg_size,
		uint8_t calling_id)
{
	i2c_read_data_func_t func = (i2c_read_data_func_t)get_kertable_entry(73);
	return func(i2c_addr, msg_size, calling_id);
}

/**
 **/
typedef int8_t (*i2c_register_slave_func_t)(uint8_t mod_id, uint8_t slave_addr);
static inline int8_t ker_i2c_register_slave(uint8_t mod_id, uint8_t slave_addr) {
	i2c_register_slave_func_t func =
		(i2c_register_slave_func_t)get_kertable_entry(74);
	return func(mod_id, slave_addr);
}

/**
 **/
typedef int8_t (*i2c_release_slave_func_t)(uint8_t calling_mod_id);
static inline int8_t ker_i2c_release_slave(uint8_t calling_mod_id)
{
	i2c_release_slave_func_t func =
		(i2c_release_slave_func_t)get_kertable_entry(75);
	return func(calling_mod_id);
}

#endif

