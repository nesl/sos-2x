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

#ifndef _SPI_H_
#define _SPI_H_

#include <hardware_types.h>

uint8_t* spi_getState();

/**
 * @brief general init function for SPI
 */
int8_t spi_init();

/**
 * @brief Master send for interrupt controlled SPI
 * @param uint8_t addr destination address (need to define this)
 * @param uint8_t *msg pointer to message data
 * @param uint8_t msg_size length of data
 * @return int8_t SOS_OK if send started; -EBUSY if the SPI bus is busy;
 */

int8_t spi_masterTxData(
		uint8_t *tx_buf,
		uint8_t tx_len,
		uint8_t flags);

int8_t spi_masterRxData(
		uint8_t *sharedBuf,
		uint8_t rx_len,
		uint8_t flags);

//uint8_t *spi_swapBufs(
//		uint8_t *emptyBuf);

int8_t spi_masterTxRxData(
		uint8_t *txBuf,
		uint8_t tx_len,
		uint8_t *sharedBuf,
		uint8_t rx_len,
		uint8_t flags);

int8_t spi_masterReSend(uint8_t flags);
#endif // _SPI_H_
