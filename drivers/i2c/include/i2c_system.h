/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file i2c_system.h
 * @brief Multimaster i2c SOS interface
 * @author Naim Busek <ndbusek@gmail.com>
 **/

#ifndef _I2C_SYSTEM_H
#define _I2C_SYSTEM_H
#include <proc_msg_types.h>

// i2c flags shared across i2c_system and i2c driver
#define I2C_SYS_SOS_MSG_FLAG 0x80 // shared with i2c driver
#define I2C_SYS_TX_FLAG      0x40 // shared with i2c driver
#define I2C_SYS_RX_PEND_FLAG 0x20 // shared with i2c driver
#define I2C_SYS_MASTER_FLAG  0x10 // shared with i2c driver
#define I2C_SYS_RSVRD_1_FLAG 0x08
#define I2C_SYS_RSVRD_2_FLAG 0x04
#define I2C_SYS_RSVRD_3_FLAG 0x02
#define I2C_SYS_ERROR_FLAG   0x01 // shared with i2c driver

#define I2C_SYS_NULL_FLAG    0x00

#define I2C_SYS_SHARED_FLAGS_MSK 0xF1

int8_t i2c_system_init();

void i2c_read_done(uint8_t *buff, uint8_t len, uint8_t rxStatus);
void i2c_send_done(uint8_t status);

#ifndef _MODULE_
/**
 * @brief I2C HPL Related Functions
 */
int8_t ker_i2c_reserve_bus(uint8_t calling_id, uint8_t i2c_addr, uint8_t flags);
int8_t ker_i2c_release_bus(uint8_t calling_id);

int8_t ker_i2c_send_data(
		uint8_t i2c_addr,
        uint8_t *msg, 
        uint8_t msg_size, 
        uint8_t calling_id);
int8_t ker_i2c_read_data(
		uint8_t i2c_addr,
        uint8_t read_size, 
        uint8_t calling_id);

#endif

#endif // _I2C_SYSTEM_H

