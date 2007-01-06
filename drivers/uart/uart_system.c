/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @file uart_system.c
 * @brief SOS interface to UART
 * @author Roy Shea
 * @author David Lee (modified 4/7/2005)
 **/
#include <hardware.h>
#include <net_stack.h>
#include <message.h>
#include <malloc.h>
#include <sos_error_types.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include "hdlc.h"  // for hdlc msg types
#include "uart_hal.h"
#include "uart.h"
#include "uart_system.h"
#ifndef NO_SOS_UART
#include <sos_uart.h>
#endif


enum {
	UART_SYS_INIT=0,
	UART_SYS_IDLE,
	UART_SYS_WAIT,
	UART_SYS_BUSY,
};


typedef struct uart_system_state {
	uint8_t system_state;
	uint8_t calling_mod_id[2]; // ID of the calling module
	uint8_t state[2]; // Protocol state of uart.
	uint8_t flags[2];
	uint8_t senddone_status;
	uint8_t *txBuf; // Transceiver buffer
} uart_system_state_t;

#define TX 0
#define RX 1

static uart_system_state_t s;

/**
 ****************************************
 Initialize the UART hardware on an AVR
 ****************************************
 */
int8_t uart_system_init() {
	uint8_t i=0;

	s.state[TX] = s.state[RX] = UART_SYS_INIT;
	s.system_state = UART_SYS_INIT;
	
	uart_init();

	for (i=0;i<2;i++) { 
    s.calling_mod_id[i] = NULL_PID;
    s.flags[i] = 0; 
	}
	s.txBuf = NULL;

	s.system_state = UART_SYS_IDLE;
	s.state[TX] = s.state[RX] = UART_SYS_IDLE;

	return SOS_OK;
}    


/**
 ****************************************
 SOS Specific Interface to the uart
 ****************************************
 */
int8_t ker_uart_reserve_bus(uint8_t calling_id, uint8_t flags) {

	// allow module currently reserving the bus to change reservation type
	uint8_t mode = RX;

	//DEBUG("in reserve bus\n");
	if (flags & UART_SYS_TX_FLAG) {
		mode = TX;
	}

	// if it is already reaserved AND
	// it was reserved by another module OR it is currently busy
	if ((s.calling_mod_id[mode] != NULL_PID) &&
			((s.calling_mod_id[mode] != calling_id) || (s.state[mode] != UART_SYS_WAIT))) {
		LED_DBG(LED_RED_TOGGLE);
		return -EBUSY;
	}
	s.calling_mod_id[mode] = calling_id;

	s.system_state = UART_SYS_BUSY;
	s.flags[mode] = flags;
	s.state[mode] = UART_SYS_WAIT;
	
	LED_DBG(LED_GREEN_TOGGLE);
	//DEBUG("end reserve bus\n");
	return SOS_OK;
}


int8_t ker_uart_release_bus(uint8_t calling_id) {
	uint8_t i=0;

	// Always allow uart_module to release uart
	// Otherwise, check that the correct module is calling the resease and
	// that the uart is not busy
	if (calling_id != UART_PID) {
		if ((s.calling_mod_id[TX] != calling_id) && (s.calling_mod_id[RX] != calling_id)) {
			return -EPERM;
		}

		if (((s.calling_mod_id[TX] == calling_id) && (s.state[TX] != UART_SYS_IDLE) && (s.state[TX] != UART_SYS_WAIT)) &&
				((s.calling_mod_id[RX] == calling_id) && (s.state[RX] != UART_SYS_IDLE) && (s.state[RX] != UART_SYS_WAIT))) {
			return -EBUSY;
		}
	}
	// Release the uart
	for (i=0;i<2;i++) {	
		if ((s.calling_mod_id[i] == calling_id) && ((s.state[i] == UART_SYS_IDLE) || (s.state[i] == UART_SYS_WAIT))) {
			s.flags[i] = 0; 
			s.calling_mod_id[i] = NULL_PID;
			s.state[i] = UART_SYS_IDLE;
		}
	}

	if ((s.state[TX] == UART_SYS_IDLE) && (s.state[RX] == UART_SYS_IDLE)) {
		s.system_state = UART_SYS_IDLE;
	}

	return SOS_OK;
}


/**
 * Send data over the uart
 *
 * if called with a sos msg (after reserving with the UART_SYS_SOS_MSG flag)
 * type cast the msg pointer to a (uint8_t*) the lower layers will handle it
 * correctly based on the flags that are set.
 */

int8_t ker_uart_send_data(
        uint8_t *buff, 
        uint8_t msg_size, 
        uint8_t calling_id) {

	// uart has not been reserved or has been researved for reading
	if (s.state[TX] == UART_SYS_IDLE) {
		//DEBUG("tx Idle\n");
		return -EINVAL;
	}
	if ((s.calling_mod_id[TX] != calling_id) || (s.state[TX] != UART_SYS_WAIT)) {
		//DEBUG("Wait \n");
		return -EBUSY;
	}
	// get a handle to the outgoing data
	s.txBuf = buff;

	if (uart_startTransceiverTx(s.txBuf, msg_size, s.flags[TX]) == SOS_OK) {
		s.state[TX] = UART_SYS_BUSY;
		s.system_state = UART_SYS_BUSY;
		return SOS_OK;
	}
	//DEBUG("stat Transceiver failed\n");
	// lower layer busy (async recieve?)
	return -EBUSY;
}


/**
 * Read data from the uart.
 */
int8_t ker_uart_read_data(
		uint8_t read_size,
		uint8_t calling_id) {

	// uart has not been reserved or reserved for writing
	if (s.state[RX] == UART_SYS_IDLE) {
		return -EINVAL;
	}
	// Check if uart rx is currently in use
	if ((s.calling_mod_id[RX] != calling_id) || (s.state[RX] != UART_SYS_WAIT)) {
		return -EBUSY;
	}
	
	if (uart_startTransceiverRx(read_size, s.flags[RX]) == SOS_OK) {
		s.state[RX] = UART_SYS_BUSY; 
		s.system_state = UART_SYS_BUSY; 
		return SOS_OK;
	}
	// lower layer busy (async recieve?)
	return -EBUSY;
}

/**
 * Send MSG_UART_SEND_DONE 
 */
void uart_send_done(uint8_t status) 
{
	// bus was reserved by someone and they sent something
	if ((s.calling_mod_id[TX] != NULL_PID) && (s.state[TX] == UART_SYS_BUSY)) {
		s.state[TX] = UART_SYS_WAIT;
		if (status & UART_SYS_ERROR_FLAG) {
				post_short(s.calling_mod_id[TX], UART_PID, MSG_UART_SEND_DONE, 0, 0, SOS_MSG_SEND_FAIL|SOS_MSG_HIGH_PRIORITY);
		} else {
			post_short(s.calling_mod_id[TX], UART_PID, MSG_UART_SEND_DONE, 0, 0, SOS_MSG_HIGH_PRIORITY);
		}
	}
}


/**
 * Send MSG_UART_READ_DONE 
 */
void uart_read_done(uint8_t length, uint8_t status) {

	uint8_t *buff = NULL;
	
	if (status & UART_SYS_ERROR_FLAG  && (s.calling_mod_id[RX] != NULL_PID)) {
		goto post_error_msg;
	} else {
		return;
	}

	buff = uart_getRecievedData();

	// if it passes sanity checks give it to the scheduler
	/*
	 * with a correct protocol byte this should never happen
	 *
	if ((buff != NULL) && (s.calling_mod_id[RX] == NULL_PID) && (length >= (SOS_MSG_HEADER_SIZE))) {
		Message *rxd_msg = (Message*)buff;
		if (length == (SOS_MSG_HEADER_SIZE + rxd_msg->len)) {
			rxd_msg->flag &= ~(SOS_MSG_PORT_MSK);
			handle_incoming_msg(rxd_msg, SOS_MSG_UART_IO);
			LED_DBG(LED_YELLOW_TOGGLE);
			return;
		}
	}
*/
	// got data but no one cares, trash buffer and return
	if (s.calling_mod_id[RX] == NULL_PID) {
		ker_free(buff);
		return;
	}

	// no data, tell it to someone who cares
	if (buff == NULL) {
		goto post_error_msg;
	}

	// verify message type and dispatch
	switch (buff[0]) {  // protocol byte
		case HDLC_SOS_MSG:
			{
				if (s.flags[RX] & UART_SYS_SOS_MSG_FLAG) {
					post_long(
							s.calling_mod_id[RX],
							UART_PID,
							MSG_UART_READ_DONE,
							length,
							buff,
							SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
					s.state[RX] = UART_SYS_WAIT;
					return;
				} else {
					ker_free(buff);
					goto post_error_msg;
				}
			}
		case HDLC_RAW:
			{
				// wrap in message and send
				post_long(
						s.calling_mod_id[RX],
						UART_PID,
						MSG_UART_READ_DONE,
						length, 
						buff,   
						SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
				s.state[RX] = UART_SYS_WAIT;
				return;
			}
		default:
			ker_free(buff);
			break;
	}

post_error_msg:
	post_short(
			s.calling_mod_id[RX],
			UART_PID,
			MSG_ERROR,
			READ_ERROR,
			0,
			SOS_MSG_HIGH_PRIORITY);
}

