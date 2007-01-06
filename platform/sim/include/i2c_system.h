/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file i2c_system.h
 * @brief Multimaster i2c SOS interface
 * @author Roy Shea
 **/

#ifndef _I2C_SYSTEM_H
#define _I2C_SYSTEM_H

/**
 * @brief I2C Module messages
 */
#define MSG_SENDING_TASK           (PLAT_MSG_START + 0)
#define MSG_READING_TASK           (PLAT_MSG_START + 1)
#define MSG_SLAVE_SENDING_TASK     (PLAT_MSG_START + 2)
#define MSG_SLAVE_READING_TASK     (PLAT_MSG_START + 3)

int8_t i2c_init();
int8_t i2c_handler(void *state, Message *e);
void i2c_slave_getting_data();
void i2c_update_state(uint8_t state);

#ifndef _MODULE_
/**
 * @brief I2C HPL Related Functions
 */
int8_t ker_i2c_reserve(uint8_t calling_mod_id);
int8_t ker_i2c_release(uint8_t calling_mod_id);
int8_t ker_i2c_send_data(uint8_t *msg, uint8_t msg_size);
int8_t ker_i2c_read_data(uint8_t addr, uint8_t msg_size);
int8_t ker_i2c_register_slave(
        uint8_t mod_id, 
        uint8_t *msg, 
        uint8_t msg_size,
        uint8_t slave_addr);
int8_t ker_i2c_release_slave(uint8_t calling_mod_id);
#endif

#endif // _I2C_SYSTEM_H

