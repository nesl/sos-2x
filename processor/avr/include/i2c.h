/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file i2c.h
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

#ifndef _I2C_H_
#define _I2C_H_

/**
 * TWI Status/Control register definitions
 */

// TWI Bit rate Register setting.  See Application note for detailed
// information on setting this value.

// Ram - This was the original value 
// Max speed
#define TWI_TWBR 0x0A
// Ram - But I am changing it so that the I2C speed is below 100 KHz so that
// it becomes compatible with SMBus devices like the TAOS photosensor
//#define TWI_TWBR 0x20

// Not used defines!
// This driver presumes prescaler = 00
//#define TWI_TWPS          0x00

// get value of twi status register
#define i2c_getStatus()    (TWSR)

// set twi control register
#define i2c_setCtrlReg(b)  (TWCR = (b))

// read twi data register
#define i2c_getByte()      (TWDR)
// set twi data register
#define i2c_setByte(b)     (TWDR = (b))

#define i2c_interrupt() SIGNAL(SIG_2WIRE_SERIAL)

/**
 * Function definitions
 */
int8_t i2c_hardware_init();

int8_t i2c_initTransceiver(
		uint8_t ownAddress,
		uint8_t flags);

int8_t i2c_startTransceiverTx(
		uint8_t addr,
		uint8_t *msg,
		uint8_t msg_len,
		uint8_t flags);

int8_t i2c_startTransceiverRx(
		uint8_t addr,
		uint8_t msg_len,
		uint8_t flags);

int8_t i2c_restartTransceiver(
		uint8_t flags);

/**
 * Bit and byte definitions
 */
#define TWI_READ_BIT  0 // Bit position for R/W bit in "address byte".
#define TWI_ADR_BITS  1 // Bit position for LSB of the slave address bits in the init byte.

//#define TRUE          1
//#define FALSE         0

/***********************************
 * TWI State codes
 **********************************/

// General TWI Master staus codes
#define TWI_START                  0x08  // START has been transmitted
#define TWI_REP_START              0x10  // Repeated START has been transmitted
#define TWI_ARB_LOST               0x38  // Arbitration lost

// TWI Master Transmitter staus codes
#define TWI_MTX_ADR_ACK            0x18 // SLA+W has been tramsmitted and ACK received
#define TWI_MTX_ADR_NACK           0x20 // SLA+W has been tramsmitted and NACK received
#define TWI_MTX_DATA_ACK           0x28 // Data byte has been tramsmitted and ACK received
#define TWI_MTX_DATA_NACK          0x30 // Data byte has been tramsmitted and NACK received

// TWI Master Receiver staus codes
#define TWI_MRX_ADR_ACK            0x40 // SLA+R has been tramsmitted and ACK received
#define TWI_MRX_ADR_NACK           0x48 // SLA+R has been tramsmitted and NACK received
#define TWI_MRX_DATA_ACK           0x50 // Data byte has been received and ACK tramsmitted
#define TWI_MRX_DATA_NACK          0x58 // Data byte has been received and NACK tramsmitted

// TWI Slave Transmitter staus codes
#define TWI_STX_ADR_ACK            0xA8 // Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0 // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWI_STX_DATA_ACK           0xB8 // Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_NACK          0xC0 // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8 // Last data byte in TWDR has been transmitted (TWEA = 0); ACK has been received

// TWI Slave Receiver staus codes
#define TWI_SRX_ADR_ACK            0x60 // Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68 // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWI_SRX_GEN_ACK            0x70 // General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78 // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_ACK       0x80 // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWI_SRX_ADR_DATA_NACK      0x88 // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWI_SRX_GEN_DATA_ACK       0x90 // Previously addressed with general call; data has been received; ACK has been returned
#define TWI_SRX_GEN_DATA_NACK      0x98 // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWI_SRX_STOP_RESTART       0xA0 // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWI Miscellaneous status codes
#define TWI_NO_STATE               0xF8 // No relevant state information available; TWINT = 0
#define TWI_BUS_ERROR              0x00 // Bus error due to an illegal START or STOP condition

#endif //_I2C_H_

