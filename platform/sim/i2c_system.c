/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @file i2c_system.c
 * @brief SOS interface to I2C
 * @author Roy Shea
 *
 **/

#include <sos.h>

#include <malloc.h>

#include "i2c_system.h"
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
//TODO: Clean up scoping
//static int8_t i2c_handler(void *state, Message *e) {
int8_t i2c_handler(void *state, Message *e) {
    return -EINVAL;
}


/**
 * Reserve the i2c for transmission
 */
int8_t ker_i2c_reserve(uint8_t calling_mod_id) {
    return -EINVAL;

}


/**
 * Release the i2c
 */
int8_t ker_i2c_release(uint8_t calling_mod_id) {
    return -EINVAL;
}

/**
 * Send data over the i2c
 */
int8_t ker_i2c_send_data(uint8_t *msg, uint8_t msg_size) {
    return -EINVAL;
}

/**
 * Read data from the i2c
 */
int8_t ker_i2c_read_data(uint8_t addr, uint8_t read_size) {
    return -EINVAL;
}


int8_t ker_i2c_register_slave(
        uint8_t mod_id, 
        uint8_t *msg, 
        uint8_t msg_size,
        uint8_t slave_addr) {
    return -EINVAL;
}


int8_t ker_i2c_release_slave(uint8_t calling_mod_id) {
    return -EINVAL;
}

        
/**
 * Signal that the slave is getting data
 */
void i2c_slave_getting_data() {
}


/**
 * Update SOS state
 */
void i2c_update_state(uint8_t state) {
}


