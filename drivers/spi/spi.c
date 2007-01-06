/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/* 
 * Authors: Naim Busek <ndbusek@gmail.com>
 *
 */

/**
 * @author Naim Busek
 */
#include <sos.h>
#include <hardware.h>

#include <spi_system.h>

#define SPI_READ_PENDING_FLAG SPI_SYS_READ_PENDING_FLAG
#define SPI_SHARED_MEM_FLAG   SPI_SYS_SHARED_MEM_FLAG
#define SPI_RSVRD_5_FLAG      0x20
#define SPI_RSVRD_4_FLAG      0x10
#define SPI_RSVRD_3_FLAG      0x08
#define SPI_RSVRD_2_FLAG      0x04
#define SPI_RSVRD_1_FLAG      0x02
#define SPI_ERROR_FLAG        0x01

#include <spi_hal.h>
#include "spi.h"

/**
 * SPI hardware states
 */
enum {
	SPI_INIT=0,
	SPI_IDLE,
	SPI_MASTER_TX,
	SPI_MASTER_TX_RX,
	SPI_MASTER_RX,
	SPI_DATA_RDY,
	SPI_SLAVE,	// slave modes reserved for multi master
	SPI_SLAVE_TX,
	SPI_SLAVE_RX
};

/**
 * put comments about registers here
 */
// status byte holding flags
typedef struct spi_state {
	uint8_t state;
	uint8_t flags;

	uint8_t *txBuf;
	uint8_t txLen;
	uint8_t txByteCnt;
	uint8_t tx_byte_idx;
	
	uint8_t *rxBuf;
	uint8_t rxLen;
	uint8_t rxByteCnt;
	uint8_t rx_byte_idx;
} spi_state_t;


#define MAX_LEN (128+1)

// Hardware state of the SPI bus 
static spi_state_t spi;

static inline void spi_rx_reset(void);
static inline void spi_tx_reset(void);


int8_t spi_init() {
	HAS_CRITICAL_SECTION;

	spi.flags = 0;;

	spi.txBuf = NULL;
	spi.txLen = 0;
	spi.txByteCnt = 0;
	spi.tx_byte_idx = 0;
	
	spi.rxBuf = NULL;
	spi.rxLen = 0;
	spi.rxByteCnt = 0;
	spi.rx_byte_idx = 0;

	ENTER_CRITICAL_SECTION();
	spi.state = SPI_IDLE;
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t spi_masterTxData(uint8_t *tx_buf, uint8_t tx_len, uint8_t flags) {
	HAS_CRITICAL_SECTION;

	// initalize hardware into master mode
	spi_hardware_initMaster();

	// do some clean up
	spi.rxLen = 0;
	spi.rx_byte_idx = 0;
	
	spi.flags = SPI_SYS_SHARED_FLAGS_MSK & flags;

	spi.txBuf = tx_buf;
	spi.txLen = tx_len;
	spi.tx_byte_idx = 0;

	ENTER_CRITICAL_SECTION();
	spi.state = SPI_MASTER_TX;
	LEAVE_CRITICAL_SECTION();

	// send the first byte
	spi_setByte(spi.txBuf[0]);

	return SOS_OK;
}


int8_t spi_masterRxData(uint8_t *sharedBuf, uint8_t rx_len, uint8_t flags) {
	HAS_CRITICAL_SECTION;

	if (rx_len >= MAX_LEN) {
		return -EINVAL;
	}

	if (NULL == sharedBuf) {
		return -EINVAL;
	} else {
		// malloc done by spi_system
		spi.rxBuf = sharedBuf;
	}

	// initalize hardware into master mode
	spi_hardware_initMaster();

	// do some clean up
	spi.txLen = 0;
	spi.tx_byte_idx = 0;
	
	spi.flags = SPI_SYS_SHARED_FLAGS_MSK & flags;
	// get handle to incoming data
	spi.rxLen = rx_len;
	spi.rx_byte_idx = 0;

	ENTER_CRITICAL_SECTION();
	spi.state = SPI_MASTER_RX;
	LEAVE_CRITICAL_SECTION();

	spi_rxByte();

	return SOS_OK;
}	


/* ********** Interrupt Handlers ********** */
static inline void spi_tx_reset() {
	// do some clean up
	spi.txLen = 0;
	spi.tx_byte_idx = 0;
}

static inline void spi_tx_interrupt_handler() {

	spi.tx_byte_idx++;
	
	if (spi.tx_byte_idx < spi.txLen) {
		// send next byte
		spi_setByte(spi.txBuf[spi.tx_byte_idx]);
	} else if (spi.rxLen > 0) {
		spi.state = SPI_MASTER_RX;
		spi_rxByte();
	} else {
		// done post message to module
		spi_maskIntr(); // mask SPI interrupts
		spi_send_done(spi.tx_byte_idx, spi.flags);
		spi_tx_reset();
	}
}


static inline void spi_rx_reset() {
	// do some clean up
	spi.rxLen = 0;
	spi.rx_byte_idx = 0;
}

static inline void spi_rx_interrupt_handler(uint8_t byte) {

	spi.rxBuf[spi.rx_byte_idx++] = byte;

	if (spi.rx_byte_idx < spi.rxLen) {
		spi_rxByte();
	} else { // finished with current read
		if (spi.flags & SPI_SHARED_MEM_FLAG) {
			// DMA more data pending
			spi.state = SPI_IDLE;
		} else { // rx done post message
			spi.state = SPI_DATA_RDY;
		}
		spi_maskIntr(); // mask SPI interrupts
		spi_read_done(spi.rxBuf, spi.rx_byte_idx, spi.flags);
		spi_rx_reset();
	}
}


/**
 * This function is the Interrupt Service Routine (ISR), and is called when the
 * SPI interrupt is triggered;  that is whenever a SPI enven has occured.  This
 * function should never be called directly from the application.
 */
spi_interrupt() {
	uint8_t byte;

	// shift read byte in
	byte = spi_getByte();
	
	if (spi_bus_error()){
		// handle write collision error
	} else {
		switch(spi.state) {
			// error: invalid states when interrupts are enabled
			case (SPI_IDLE):
				// disable and clean up
				spi_maskIntr(); // mask SPI interrupts
				spi_tx_reset();
				spi_rx_reset();
				break;

			case (SPI_MASTER_TX):
				if (spi_is_master()) {
					spi_tx_interrupt_handler();
				} else {
					spi_maskIntr(); // mask SPI interrupts
					spi_tx_reset();
					spi.state = SPI_IDLE;
				}
				break;

			case (SPI_MASTER_RX):
				spi_rx_interrupt_handler(byte);
				break;

			default:
				spi_maskIntr(); // mask SPI interrupts
				spi_rx_reset();
				spi_tx_reset();
				spi.state = SPI_IDLE;
				break;
		}
	}
}

