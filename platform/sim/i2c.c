/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/**
 * @file i2c.c
 * @brief AVR specific I2C driver 
 * @author AVR App Note 311 and 315
 * @author Modified by Roy Shea
 *
 * This work is directly from the Atmel AVR application notes AVR311
 * and AVR315.
 **/

#include <sos.h>

#include "i2c.h"
#include "i2c_system.h"

/*
 * TODO:  I think that these are not being used (3-18-05)...
#define TW_STATUS_MASK  0xf8
#define TW_START 0x08
#define TW_MR_SLA_ACK 0x18
*/



/**
 ****************************************
 Initialize the I2C hardware on an AVR
 ****************************************
 */
int8_t TWI_hardware_init() {
    return -EINVAL;
}    

/**
 ****************************************
 General TWI Status and control
 ****************************************
 */


/**
 * Call this function to test if the TWI_ISR is busy transmitting.
 */
uint8_t TWI_Transceiver_Busy() 
{
    return 0;
}



uint8_t TWI_Get_State_Info() 
{
    return 0;
}


uint8_t TWI_Get_Data_From_Transceiver( 
        uint8_t *msg, 
        uint8_t msg_size) 
{ 
    return 0;
}




/**
 ****************************************
 Slave Control
 ****************************************
 */


void TWI_Slave_Initialize(uint8_t TWI_ownAddress) 
{
}


void TWI_Slave_Start_Transceiver() 
{
}


void TWI_Slave_Start_Transceiver_With_Data( 
        uint8_t *msg, 
        uint8_t msg_size) 
{ 
}



/**
 ****************************************
 Master Control
 ****************************************
 */


void TWI_Master_Initialize() 
{
}

void TWI_Master_Start_Transceiver() 
{
}


void TWI_Master_Start_Transceiver_With_Data( 
        uint8_t *msg, 
        uint8_t msg_size ) 
{
}


// ********** Interrupt Handlers ********** //


//SIGNAL(SIG_2WIRE_SERIAL) {}

