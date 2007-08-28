#ifndef _MODULE_PROC_H
#define _MODULE_PROC_H

/**
 **/
extern int8_t ker_i2c_release_bus(uint8_t calling_mod_id);


/**
 * @brief I2C send data function.
 * @param msg Pointer to the data to send
 * @param msg_size Length of data to send
 **/
extern int8_t ker_i2c_send_data(
		uint8_t i2c_addr,
		uint8_t *msg,
		uint8_t msg_size,
		uint8_t calling_id);

/**
 * @brief I2C read data function. Split phase call. Reply is MSG_I2C_READ_DONE
 * @param msg_size Length of data to read
 **/
extern int8_t ker_i2c_read_data(
		uint8_t i2c_addr,
		uint8_t msg_size,
		uint8_t calling_id);


/**
 * @brief Exception handler in the case of a memory access fault
 * @note Currently supported only for the AVR processor
 */
extern void ker_memmap_perms_check(void* x);

#endif

