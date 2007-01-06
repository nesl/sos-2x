/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file spi.h
 * @brief Multimaster SPI SOS interface
 * @author AVR App Note 151
 * @author Naim Busek
 *
 * This work is based on the Avr application notes avr151.
 **/

#ifndef _SPI_HAL_H_
#define _SPI_HAL_H_

#include <hardware_types.h>

/**
 * status register test functions
 */
static inline int8_t spi_bus_error() { return (SPSR & (1<<WCOL)); }
static inline int8_t spi_is_master() { return (SPCR & (1<<MSTR)); }

/**
 * interrupt register test and set functions
 */
#define spi_interrupt() SIGNAL (SIG_SPI)
static inline uint8_t spi_busy() { return bit_is_clear(SPSR, SPIF); }
static inline void spi_maskIntr() { SPCR &= ~(1<<SPIE); }
static inline void spi_enableIntr() { SPCR = (1<<SPIE)|(1<<SPE); SET_SS_DD_IN(); }
static inline void spi_disableIntr() { SPCR &= ~(1<<(SPIE)); SET_SS_DD_OUT(); CLR_SS(); }

/**
 * byte IO functions
 */
static inline void spi_setByte(uint8_t byte) { SPDR = byte; }

// read is a two step operation
// first you write any value to SPDR to cause the clock to run
// then use getByte in the interrupt to get the value that is read in
// for not full duplex is not supported
static inline void spi_rxByte() { SPDR = 0x00; }
static inline uint8_t spi_getByte() { return SPDR; }

/**
 * pin config functions
 */
static inline void spi_txMode() { SET_MISO_DD_OUT(); SET_MOSI_DD_OUT(); }
static inline void spi_rxMode() { SET_MISO_DD_IN(); SET_MOSI_DD_IN(); }

/**
 * Table 3. SPI Mode Configuration
 * SPI Mode CPOL CPHA Shift SCK-edge Capture SCK-edge
 *       0    0    0  Falling        Rising
 *       1    0    1  Rising         Falling
 *       2    1    0  Rising         Falling
 *       3    1    1  Falling        Rising
 */

#define SPI_MODE_0 0
#define SPI_MODE_1 1
#define SPI_MODE_2 2
#define SPI_MODE_3 3

static inline void spi_set_mode(uint8_t mode) {
	SPCR &= (~(0x3 << CPHA)); 
	SPCR |= ((mode & 0x3) << CPHA);
}

/**
 * @brief hardware init functions for SPI
 */
int8_t spi_hardware_init(void);
int8_t spi_hardware_initMaster(void);


#endif // _SPI_HAL_H_
