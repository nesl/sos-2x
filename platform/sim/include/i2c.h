/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file i2c.h
 * @brief Multimaster i2c
 * @author AVR App Note 311 and 315
 * @author Roy Shea
 *
 * This work is a merege of the header files from the Atmel
 * Avr application notes avr311 and avr315.
 **/

#ifndef _I2C_H_
#define _I2C_H_

#include <message_types.h>

#define MAX_I2C_WAIT 128


// TODO: EVIL.  This should not be exposed in this manner
extern uint8_t TWI_buf[];


/**
 * Constants used by i2c
 */
enum {TWI_NOT_BUSY=1, TWI_SENDING, TWI_READING};


/**
 * TWI Status/Control register definitions
 */

// Set this to the largest message size that will be sent including address
// byte.
#define TWI_BUFFER_SIZE 9

// TWI Bit rate Register setting.  See Application note for detailed
// information on setting this value.
#define TWI_TWBR 0x0C

// Not used defines!
// This driver presumes prescaler = 00
//#define TWI_TWPS          0x00

/**
 * Global definitions
 */

// Status byte holding flags.
union TWI_statusReg
{
    uint8_t all;
    struct
    {
        uint8_t lastTransOK:1;
        uint8_t RxDataInBuf:1;
        // If genAddressCall is TRUE then General call, if FALSE then TWI
        // Address;
        uint8_t genAddressCall:1;
        uint8_t unusedBits:5;
    };
};

extern union TWI_statusReg TWI_statusReg;

/**
 * Function definitions
 */
int8_t TWI_hardware_init();

uint8_t TWI_Transceiver_Busy();
uint8_t TWI_Get_State_Info();
uint8_t TWI_Get_Data_From_Transceiver( uint8_t *msg, uint8_t msgSize);

void TWI_Slave_Initialize(uint8_t TWI_ownAddress);
void TWI_Slave_Start_Transceiver();
void TWI_Slave_Start_Transceiver_With_Data(uint8_t *msg , uint8_t msg_size);

void TWI_Master_Initialize();
void TWI_Master_Start_Transceiver();
void TWI_Master_Start_Transceiver_With_Data(uint8_t *msg, uint8_t msg_size);

/**
 * Bit and byte definitions
 */
// Bit position for R/W bit in "address byte".
#define TWI_READ_BIT  0
// Bit position for LSB of the slave address bits in the init byte.
#define TWI_ADR_BITS  1
#define TWI_GEN_BIT   0


#define TRUE          1
#define FALSE         0

/**
 * TWI State codes
 */

/**
 * General TWI Master staus codes
 */
#define TWI_START                  0x08  // START has been transmitted
#define TWI_REP_START              0x10  // Repeated START has been transmitted
#define TWI_ARB_LOST               0x38  // Arbitration lost

/**
 * TWI Master Transmitter staus codes
 */
// SLA+W has been tramsmitted and ACK received
#define TWI_MTX_ADR_ACK            0x18
// SLA+W has been tramsmitted and NACK received
#define TWI_MTX_ADR_NACK           0x20
// Data byte has been tramsmitted and ACK received
#define TWI_MTX_DATA_ACK           0x28
// Data byte has been tramsmitted and NACK received
#define TWI_MTX_DATA_NACK          0x30

/**
 * TWI Master Receiver staus codes
 */
// SLA+R has been tramsmitted and ACK received
#define TWI_MRX_ADR_ACK            0x40
// SLA+R has been tramsmitted and NACK received
#define TWI_MRX_ADR_NACK           0x48
// Data byte has been received and ACK tramsmitted
#define TWI_MRX_DATA_ACK           0x50
// Data byte has been received and NACK tramsmitted
#define TWI_MRX_DATA_NACK          0x58

/**
 * TWI Slave Transmitter staus codes
 */
// Own SLA+R has been received; ACK has been returned
#define TWI_STX_ADR_ACK            0xA8
// Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has
// been returned
#define TWI_STX_ADR_ACK_M_ARB_LOST 0xB0
// Data byte in TWDR has been transmitted; ACK has been received
#define TWI_STX_DATA_ACK           0xB8
// Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWI_STX_DATA_NACK          0xC0
// Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been
// received
#define TWI_STX_DATA_ACK_LAST_BYTE 0xC8

/**
 * TWI Slave Receiver staus codes
 */
// Own SLA+W has been received ACK has been returned
#define TWI_SRX_ADR_ACK            0x60
// Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has
// been returned
#define TWI_SRX_ADR_ACK_M_ARB_LOST 0x68
// General call address has been received; ACK has been returned
#define TWI_SRX_GEN_ACK            0x70
// Arbitration lost in SLA+R/W as Master; General call address has been
// received; ACK has been returned
#define TWI_SRX_GEN_ACK_M_ARB_LOST 0x78
// Previously addressed with own SLA+W; data has been received; ACK has been
// returned
#define TWI_SRX_ADR_DATA_ACK       0x80
// Previously addressed with own SLA+W; data has been received; NOT ACK has
// been returned
#define TWI_SRX_ADR_DATA_NACK      0x88
// Previously addressed with general call; data has been received; ACK has
// been returned
#define TWI_SRX_GEN_DATA_ACK       0x90
// Previously addressed with general call; data has been received; NOT ACK has
// been returned
#define TWI_SRX_GEN_DATA_NACK      0x98
// A STOP condition or repeated START condition has been received while still
// addressed as Slave
#define TWI_SRX_STOP_RESTART       0xA0

/**
 * TWI Miscellaneous status codes
 */
// No relevant state information available; TWINT = 0
#define TWI_NO_STATE               0xF8
// Bus error due to an illegal START or STOP condition
#define TWI_BUS_ERROR              0x00

#endif //_I2C_H_

