#include <sos.h>
#include "i2c.h"

/**
 ****************************************
 Initialize the I2C hardware on an AVR
 ****************************************
 */
int8_t i2c_init() {
    return SOS_OK;
}



/**
 ****************************************
 SOS Specific Interface to the i2c
 ****************************************
 */

/**
 * SOS handler for i2c communications
 */
int8_t i2c_handler(void *state, Message *e) {
    switch(e->type){
        default:
            return -EINVAL;
    }
    return SOS_OK;
}


/**
 * Send data over the i2c
 */
int8_t ker_i2c_send_data(uint8_t i2c_addr,
        uint8_t *msg,
        uint8_t msg_size,
        uint8_t calling_id) {
    return SOS_OK;
}

/**
 * Read data from the i2c.
 */
int8_t ker_i2c_read_data(uint8_t i2c_addr,
        uint8_t read_size,
        uint8_t calling_id) {
    return SOS_OK;
}


int8_t ker_i2c_register_slave(uint8_t calling_id, uint8_t slave_addr) {
    return SOS_OK;
}


int8_t ker_i2c_release_slave(uint8_t calling_id) {
    return SOS_OK;
}



int8_t ker_i2c_slave_start(uint8_t *msg, uint8_t msg_size) {
    return SOS_OK;
}


/**
 * Send MSG_I2C_READ_DONE
 */
void i2c_read_done(uint8_t length) {
}

/**
 * Send MSG_I2C_SEND_DONE
 */
void i2c_send_done(bool writing) {
}

