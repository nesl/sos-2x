/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file spi.h
 * @brief Multimaster SPI SOS interface
 * @author Naim Busek
 *
 * This work is based on the Avr application notes avr151.
 **/

#ifndef _SPI_SYSTEM_H_
#define _SPI_SYSTEM_H_

#include <bus_cb.h>

#define MAX_SPI_READ_LEN (128+1)

// i2c flags shared across spi_system and spi driver
#define SPI_SYS_READ_PENDING_FLAG 0x80 // shared with spi driver
#define SPI_SYS_SHARED_MEM_FLAG   0x40 // shared with spi driver
#define SPI_SYS_RSVRD_1_FLAG      0x20
#define SPI_SYS_SEND_DONE_CB_FLAG 0x10
#define SPI_SYS_READ_DONE_CB_FLAG 0x08
#define SPI_SYS_CS_HIGH_FLAG      0x04
#define SPI_SYS_LOCK_BUS_FLAG     0x02
#define SPI_SYS_ERROR_FLAG        0x01 // shared with spi driver

#define SPI_SYS_NULL_FLAG    0x00

#define SPI_SYS_SHARED_FLAGS_MSK 0xC1

#define SPI_SYS_SHARED_TX_FLAGS_MSK 0x81
#define SPI_SYS_SHARED_RX_FLAGS_MSK 0x41
			   
/**
 * error types
 */
enum {
	SPI_READ_ERROR=1,
	SPI_BUS_ERROR,
};

// set the defaults to active low
#define spi_cs(addr) spi_cs_low(addr)
#define spi_cs_release(addr) spi_cs_release_low(addr)

static inline void spi_cs_toggle(spi_addr_t addr) { _MMIO_BYTE((uint16_t)addr.cs_reg) ^= (1 << addr.cs_bit); }

static inline void spi_cs_low(spi_addr_t addr) { _MMIO_BYTE((uint16_t)addr.cs_reg) &= ~(1 << addr.cs_bit); }
static inline void spi_cs_release_low(spi_addr_t addr) { _MMIO_BYTE((uint16_t)addr.cs_reg) |= (1 << addr.cs_bit); }

static inline void spi_cs_high(spi_addr_t addr) { _MMIO_BYTE((uint16_t)addr.cs_reg) |= (1 << addr.cs_bit); }
static inline void spi_cs_release_high(spi_addr_t addr) { _MMIO_BYTE((uint16_t)addr.cs_reg) &= ~(1 << addr.cs_bit); }


/**
 * Function definitions
 */
/** @brief general init function for SPI */
int8_t spi_system_init();

void spi_read_done(uint8_t *buff, uint8_t length, uint8_t status);
void spi_send_done(uint8_t length, uint8_t status);

#ifndef _MODULE_
/**
 * @brief Take or release control of the SPI bus
 */
int8_t ker_spi_reserve_bus(
		sos_pid_t calling_id,
		spi_addr_t addr,
		uint8_t flags);

int8_t ker_spi_register_read_cb(
		sos_pid_t calling_id,
		bus_read_done_cb_t func);
		
int8_t ker_spi_unregister_read_cb(
		sos_pid_t calling_id);

int8_t ker_spi_register_send_cb(
		sos_pid_t calling_id,
		bus_send_done_cb_t func);
		
int8_t ker_spi_unregister_send_cb(
		sos_pid_t calling_id);

int8_t ker_spi_release_bus(sos_pid_t calling_id);

/**
 * @brief Master send for interrupt controlled SPI
 * @param uint8_t cs_byte & cs_bit define addressed device
 * @param uint8_t *msg pointer to message data
 * @param uint8_t msg_size length of data
 * @param uint8_t calling module
 * @return int8_t SOS_OK if send started; -EBUSY if the SPI bus is busy;
 */
int8_t ker_spi_send_data(
		uint8_t *buf,
		uint8_t tx_len,
		sos_pid_t calling_id);

int8_t ker_spi_read_data(
		uint8_t *sharedBuf,
		uint8_t rx_len,
		uint8_t rx_cnt,
		sos_pid_t calling_id);

int8_t ker_spi_read_write_data(
		uint8_t *txBuf,
		uint8_t tx_len,
		uint8_t *sharedBuf,
		uint8_t rx_len,
		uint8_t rx_cnt,
		sos_pid_t calling_id);
#endif

#endif // _SPI_SYSTEM_H_
