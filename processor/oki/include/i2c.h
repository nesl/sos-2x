/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file i2c.h
 * @brief Multimaster i2c SOS interface
 * @author Roy Shea
 **/

#ifndef _I2C_H
#define _I2C_H

/**
 * @brief I2C Module messages
 */
#define MSG_SENDING_TASK            (PROC_MSG_START + 0)
#define MSG_READING_TASK            (PROC_MSG_START + 1)
#define MSG_SLAVE_SENDING_TASK      (PROC_MSG_START + 2)
#define MSG_SLAVE_READING_TASK      (PROC_MSG_START + 3)
#define MSG_SLAVE_STOP_TASK         (PROC_MSG_START + 4)
#define MSG_SEND_READ_REQUEST_TASK  (PROC_MSG_START + 5)

int8_t i2c_init();
int8_t i2c_handler(void *state, Message *e);
void i2c_slave_stop(uint8_t len);
void i2c_read_done(uint8_t len);
void i2c_send_done(bool writing);

#ifndef _MODULE_
/**
 * @brief I2C HPL Related Functions
 */
int8_t ker_i2c_send_data(uint8_t i2c_addr,
        uint8_t *msg,
        uint8_t msg_size,
        uint8_t calling_id);
int8_t ker_i2c_read_data(uint8_t i2c_addr,
        uint8_t msg_size,
        uint8_t calling_id);

int8_t ker_i2c_register_slave(uint8_t calling_id, uint8_t slave_addr);
int8_t ker_i2c_release_slave(uint8_t calling_mod_id);
int8_t ker_i2c_slave_start(uint8_t *msg, uint8_t msg_size);

#endif

#endif // _I2C_H

