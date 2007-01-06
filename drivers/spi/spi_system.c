/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @file spi_system.c
 * @brief SOS interface to SPI
 * @author Naim Busek
 */

#include <sos.h>
#include <hardware.h>

#include "spi_system.h"

#include <uart_dbg.h>

/**
 * spi_system states
 */
enum {
	SPI_SYS_INIT=0,
	SPI_SYS_IDLE,
	SPI_SYS_WAIT,  // reserved no tx/rx call
	SPI_SYS_TX,    // tx initiated
	SPI_SYS_TX_RX, // tx/rx sequenced initated
	SPI_SYS_RX_WAIT, // tx finished READ_PENDING flag set, waiting for rx
	SPI_SYS_RX,    // rx initated
	SPI_SYS_DMA_WAIT,  // using DMA waiting for next tx/rx call
	SPI_SYS_ERROR,
};


// Protocol state of spi_system
typedef struct {
	uint8_t calling_mod_id;
	uint8_t state;

	bus_read_done_cb_t read_done_cb;
	bus_send_done_cb_t send_done_cb;
	
	uint8_t *usrBuf;
	uint8_t *bufPtr;
	
	uint8_t len;
	uint8_t cnt;
	
	spi_addr_t addr;
	
	uint8_t flags;
} spi_system_state_t;

static spi_system_state_t s;


/**
 * Initalize the SPI system
 */
int8_t spi_system_init() {

	s.state = SPI_SYS_INIT;
	spi_init();

	s.calling_mod_id = NULL_PID;
	s.state = SPI_SYS_IDLE;

	s.read_done_cb = NULL;
	s.send_done_cb = NULL;

	s.usrBuf = NULL;
	s.bufPtr = NULL;
	s.len = 0;

	return SOS_OK;
}


int8_t ker_spi_reserve_bus(uint8_t calling_id, spi_addr_t addr, uint8_t flags) {
	HAS_CRITICAL_SECTION;

	// Make sure that spi is not busy
	if ((s.calling_mod_id != NULL_PID) && 
			((s.calling_mod_id != calling_id) || ((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT)))) {
		return -EBUSY;
	}

	s.calling_mod_id = calling_id;
	ENTER_CRITICAL_SECTION();
	s.flags = flags;
	s.addr.cs_reg = addr.cs_reg;
	s.addr.cs_bit = addr.cs_bit;

	s.state = SPI_SYS_WAIT;
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t ker_spi_release_bus(uint8_t calling_id) {
	// Allow calling_mod_id = SPI_PID to always release spi
	// Otherwise, check that the correct module is callling the resease and
	// that the spi is not busy.
	if (calling_id != SPI_PID) {
		if (s.calling_mod_id != calling_id) {
			return -EPERM;
		}
		if ((s.state != SPI_SYS_IDLE) && (s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT)) {
			return -EBUSY;
		}
	}

	// Release the spi bus
	if (s.flags & SPI_SYS_CS_HIGH_FLAG) {
		spi_cs_release_high(s.addr);
	} else {
		spi_cs_release_low(s.addr);
	}

	if (s.flags & SPI_SYS_READ_DONE_CB_FLAG) {
		ker_spi_unregister_read_cb(s.calling_mod_id);
	}
	if (s.flags & SPI_SYS_SEND_DONE_CB_FLAG) {
		ker_spi_unregister_send_cb(s.calling_mod_id);
	}
	// reset spi sub system
	spi_system_init();

	return SOS_OK;
}


int8_t ker_spi_register_read_cb(sos_pid_t calling_id, bus_read_done_cb_t func) {
	HAS_CRITICAL_SECTION;
	
	if (s.state == SPI_SYS_IDLE) {
		return -EINVAL;
	}	
	if ((s.calling_mod_id != calling_id) || ((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT))) {
		return -EBUSY;
	}

	ENTER_CRITICAL_SECTION();
	s.read_done_cb = func;
	s.flags |= SPI_SYS_READ_DONE_CB_FLAG;
	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}


int8_t ker_spi_unregister_read_cb(sos_pid_t calling_id) {
	HAS_CRITICAL_SECTION;
	
	if (s.state == SPI_SYS_IDLE) {
		return -EINVAL;
	}	
	if ((s.calling_mod_id != calling_id) || ((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT))) {
		return -EBUSY;
	}

	ENTER_CRITICAL_SECTION();
	s.read_done_cb = NULL;
	s.flags &= ~SPI_SYS_READ_DONE_CB_FLAG;
	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}


int8_t ker_spi_register_send_cb(sos_pid_t calling_id, bus_send_done_cb_t func) {
	HAS_CRITICAL_SECTION;
	
	if (s.state == SPI_SYS_IDLE) {
		return -EINVAL;
	}	
	if ((s.calling_mod_id != calling_id) || ((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT))) {
		return -EBUSY;
	}

	ENTER_CRITICAL_SECTION();
	s.send_done_cb = func;
	s.flags |= SPI_SYS_SEND_DONE_CB_FLAG;
	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}


int8_t ker_spi_unregister_send_cb(sos_pid_t calling_id) {
	HAS_CRITICAL_SECTION;
	
	if (s.state == SPI_SYS_IDLE) {
		return -EINVAL;
	}	
	if ((s.calling_mod_id != calling_id) || ((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT))) {
		return -EBUSY;
	}

	ENTER_CRITICAL_SECTION();
	s.send_done_cb = NULL;
	s.flags &= ~SPI_SYS_SEND_DONE_CB_FLAG;
	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}


/**
 * Send data over the spi
 */
int8_t ker_spi_send_data(
		uint8_t *msg,
		uint8_t msg_size,
		uint8_t calling_id) {
	HAS_CRITICAL_SECTION;

	if (s.state == SPI_SYS_IDLE) {
		return -EINVAL;
	}	
	if ((s.calling_mod_id != calling_id) || ((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT))) {
		return -EBUSY;
	}

	// ensure calling app gave us a message
	if (NULL != msg) {
		s.usrBuf = s.bufPtr = msg;
	} else {
		return -EINVAL;
	}
	
	// need to assert CS pin
	if (s.flags & SPI_SYS_CS_HIGH_FLAG) {
		spi_cs_high(s.addr);
	} else {
		spi_cs_low(s.addr);
	}

	ENTER_CRITICAL_SECTION();
	s.len = msg_size;
	s.state = SPI_SYS_TX;
	LEAVE_CRITICAL_SECTION();
	UART_DBG(a, 0x22, s.calling_mod_id, 0x01, 0x02, SPI_PID);
	
  return spi_masterTxData(s.bufPtr, s.len, s.flags);
}


/**
 * Read data from the spi
 */
int8_t ker_spi_read_data(
		uint8_t *sharedBuf,
		uint8_t rx_len,
		uint8_t rx_cnt,
		uint8_t calling_id) {
	HAS_CRITICAL_SECTION;

	if (s.state == SPI_SYS_IDLE) {  // not reserved
		return -EINVAL;
	}	

	if ((s.calling_mod_id != calling_id) || 
			((s.state != SPI_SYS_WAIT) && (s.state != SPI_SYS_DMA_WAIT) && (s.state != SPI_SYS_RX_WAIT))) {
		return -EBUSY;
	}

	// get a handle to users buffer
	if (rx_len >= MAX_SPI_READ_LEN) {
		return -EINVAL;
	} else {
		s.len = rx_len;
	}

	// need to assert CS pin
	if (s.flags & SPI_SYS_CS_HIGH_FLAG) {
		spi_cs_high(s.addr);
	} else {
		spi_cs_low(s.addr);
	}

	// only get/malloc a buffer if we are not currently in a DMA sequence
	if ((s.flags & SPI_SYS_SHARED_MEM_FLAG)) {
		if (NULL == sharedBuf) {
			return -EINVAL;
		} else {
			s.cnt = rx_cnt;
			s.bufPtr = sharedBuf;
			if (!(s.state == SPI_SYS_DMA_WAIT)) {
				s.usrBuf = sharedBuf;
			}
		}
	} else {
		// ignore value of sharedBuf
		if (NULL == (s.bufPtr = s.usrBuf = ker_malloc(s.len, SPI_PID))) {
			return -ENOMEM;
		} else {
			s.cnt = 1;
		}
	}

	ENTER_CRITICAL_SECTION();
	s.state = SPI_SYS_RX;
	LEAVE_CRITICAL_SECTION();
	
	return spi_masterRxData(s.usrBuf, s.len, s.flags);
}


/**
 * Send MSG_SPI_SEND_DONE
 */
void spi_send_done(uint8_t length, uint8_t status) {

	// always un chip select 
	if (s.flags & SPI_SYS_CS_HIGH_FLAG) {
		spi_cs_release_high(s.addr);
	} else {
		spi_cs_release_low(s.addr);
	}

	// read will be initiated async. by driver or in callback
	if (s.flags & SPI_SYS_READ_PENDING_FLAG) {
		s.state = SPI_SYS_RX_WAIT;
		if (s.flags & SPI_SYS_SEND_DONE_CB_FLAG) {
			s.send_done_cb(length, s.cnt, status);
		}
	} else {
		if (s.flags & SPI_SYS_SEND_DONE_CB_FLAG) {
			s.send_done_cb(length, s.cnt, status);
		} else {
			post_short(s.calling_mod_id, SPI_PID, MSG_SPI_SEND_DONE, 0, 0,  SOS_MSG_HIGH_PRIORITY);
		}
		ker_spi_release_bus(SPI_PID);
	}
}


/**
 * Send MSG_SPI_READ_DONE
 */
void spi_read_done(uint8_t *buff, uint8_t length, uint8_t status) {
	
	if (s.flags & SPI_SYS_CS_HIGH_FLAG) {
		spi_cs_release_high(s.addr);
	} else {
		spi_cs_release_low(s.addr);
	}

	if((NULL == buff) && (NULL_PID != s.calling_mod_id)) {
		post_short(
				s.calling_mod_id,
				SPI_PID,
				MSG_ERROR,
				SPI_READ_ERROR,
				0,
				SOS_MSG_HIGH_PRIORITY);
		return;
	}

	if (NULL == buff) {
		return;
	}

	if (s.len == length) {
		s.cnt--;
		if (s.cnt > 0) {
			s.bufPtr += s.len;
			s.state = SPI_SYS_DMA_WAIT;
		} else {
			// read done send response
			if (s.flags & SPI_SYS_READ_DONE_CB_FLAG) {
				s.read_done_cb(s.usrBuf, length, s.cnt, status);
			} else {

				if (s.flags & SPI_SYS_SHARED_MEM_FLAG) {
					post_long(
							s.calling_mod_id,
							SPI_PID,
							MSG_SPI_READ_DONE,
							length, // read length
							s.usrBuf, // return the buffer pointer
							SOS_MSG_HIGH_PRIORITY);

				} else {
					// dont trust the user to free the memory to message only if not shared
					post_long(
							s.calling_mod_id,
							SPI_PID,
							MSG_SPI_READ_DONE,
							length, // byte length
							s.usrBuf,
							SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
				}
			}
			/*if (s.flags & SPI_SYS_SHARED_MEM_FLAG) {
				ker_free(s.usrBuf);
			}*/
			s.usrBuf = NULL;
			s.bufPtr = NULL;
		}
		s.state = SPI_SYS_WAIT;
	} else { // short message
		post_short(
				s.calling_mod_id,
				SPI_PID,
				MSG_ERROR,
				SPI_READ_ERROR,
				0,
				SOS_MSG_HIGH_PRIORITY);

		if (!(s.flags & SPI_SYS_SHARED_MEM_FLAG)) {
			ker_free(s.usrBuf);
		}
		spi_system_init();
	}

	if (!(s.flags & SPI_SYS_LOCK_BUS_FLAG)) {
		ker_spi_release_bus(s.calling_mod_id);
	}
}

