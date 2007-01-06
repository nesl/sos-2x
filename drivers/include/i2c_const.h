/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file i2c_const.h
 * @brief Multimaster i2c
 * @author AVR App Note 311 and 315
 * @author Roy Shea
 * @author Naim Busek <ndbusek@gmail.com>
 *
 * This work is an extension of the Atmel Avr application notes
 * avr311 and avr315.  It extends them to provide full multi master
 * support handling all states of the state machine and using a flag
 * seperate from TWIE to determin bus activit.
 * 
 */

#ifndef _I2C_CONST_H_
#define _I2C_CONST_H_

/**
 * some address definitions from the i2c spec
 * these are for a 7 bit address space 
 * addresses of the form 1111xxx are reserved
 */
#define I2C_BCAST_ADDRESS   0x00
#define I2C_MAX_VALID_ADDRESS  0x77
#define I2C_RSVD_ADDR_START 0x78
#define I2C_RSVD_ADDR_STOP  0x7F

#endif //_I2C_CONST_H_

