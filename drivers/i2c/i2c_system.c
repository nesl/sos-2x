/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @file i2c_system.c
 * @brief SOS interface to I2C
 * @author Roy Shea
 * @author David Lee (modified 4/7/2005)
 **/

#include <net_stack.h>
#include <message.h>
#include <malloc.h>
#include <hdlc.h>
#include <crc.h>
#include <sos_error_types.h>

//#define LED_DEBUG
#include <led_dbg.h>

//#define UART_DEBUG
#include <uart_dbg.h>

#include <i2c.h>
#include <i2c_const.h>
#include <i2c_system.h>
#include <sos_i2c_mgr.h>


enum {
	I2C_SYS_INIT=0,
	I2C_SYS_IDLE,
	I2C_SYS_MASTER_WAIT,
	I2C_SYS_MASTER_TX,
	I2C_SYS_MASTER_RX,
	I2C_SYS_SLAVE_WAIT,
	I2C_SYS_SLAVE_TX,
	I2C_SYS_SLAVE_RX,
};


typedef struct i2c_system_state {
	uint8_t calling_mod_id; // ID of the calling module
	uint8_t state;          // Protocol state of i2c_system.
	uint8_t ownAddr;
	
	uint8_t addr;           // slave address
	Message *msgBuf;

	uint8_t *dataBuf;       // Transceiver buffer
	uint8_t rxLen;

	uint8_t flags;
} i2c_system_state_t;

static i2c_system_state_t i2c_sys;

/**
 ****************************************
 Initialize the I2C hardware on an AVR
 ****************************************
 */
int8_t i2c_system_init() {

	i2c_sys.state = I2C_SYS_INIT;

	i2c_hardware_init();
	
	i2c_sys.calling_mod_id = NULL_PID;
	i2c_sys.flags = I2C_SYS_NULL_FLAG;
	i2c_sys.msgBuf = NULL;
	i2c_sys.dataBuf = NULL;
	i2c_sys.rxLen = 0;
	i2c_sys.addr = I2C_BCAST_ADDRESS;

	i2c_sys.ownAddr = I2C_ADDRESS;
	i2c_initTransceiver(I2C_ADDRESS, I2C_SYS_MASTER_FLAG);

	i2c_sys.state = I2C_SYS_IDLE;

	return SOS_OK;
}


/**
 ****************************************
 SOS Specific Interface to the i2c
 ****************************************
 */
int8_t ker_i2c_reserve_bus(uint8_t calling_id, uint8_t ownAddress, uint8_t flags) {

	// can only reserve bus for raw use
	// all incoming sos_msgs will be handed off to the scheduler
	// this means that a module can not reserve the bus to read sos_msgs
	if ((flags & I2C_SYS_SOS_MSG_FLAG) && !(flags & I2C_SYS_TX_FLAG)) {
		return -EINVAL;
	}

	// if it is already reaserved AND
	// it was reserved by another module OR it is currently busy
	if ((i2c_sys.calling_mod_id != NULL_PID) &&
			((i2c_sys.calling_mod_id != calling_id) || ((i2c_sys.state != I2C_SYS_SLAVE_WAIT) && (i2c_sys.state != I2C_SYS_MASTER_WAIT)))) {
		LED_DBG(LED_RED_TOGGLE);
		return -EBUSY;
	}
	i2c_sys.calling_mod_id = calling_id;

	i2c_sys.ownAddr = ownAddress;
	i2c_sys.flags = flags;

	i2c_initTransceiver(ownAddress, i2c_sys.flags);

	if (i2c_sys.flags & I2C_SYS_MASTER_FLAG) {
		i2c_sys.state = I2C_SYS_MASTER_WAIT;
	} else {
		i2c_sys.state = I2C_SYS_SLAVE_WAIT;
	}

	LED_DBG(LED_GREEN_TOGGLE);
	return SOS_OK;
}


int8_t ker_i2c_release_bus(uint8_t calling_id) {

    // Always allow i2c_module to release i2c
    // Otherwise, check that the correct module is calling the resease and
    // that the i2c is not busy.
    if (calling_id != I2C_PID) {
			if (i2c_sys.calling_mod_id != calling_id) {
				return -EPERM;
			}
      if ((i2c_sys.state != I2C_SYS_IDLE) && (i2c_sys.state != I2C_SYS_SLAVE_WAIT) && (i2c_sys.state != I2C_SYS_MASTER_WAIT)) {
				return -EBUSY;
			}
		}
		// release the i2c
    i2c_sys.flags = 0;
		i2c_sys.rxLen = 0;
    i2c_sys.calling_mod_id = NULL_PID;
    i2c_sys.state = I2C_SYS_IDLE;

    return SOS_OK;
}


/**
 * Send data over the i2c
 */
int8_t ker_i2c_send_data(
		uint8_t dest_addr,
		uint8_t *buff,
		uint8_t msg_size,
		uint8_t calling_id) {

	// Check if others are using the i2c
	if (i2c_sys.state == I2C_SYS_IDLE) {  // bus not initialized??
		return -EINVAL;
	}
	if ((i2c_sys.calling_mod_id != calling_id) ||
			((i2c_sys.state != I2C_SYS_SLAVE_WAIT) && (i2c_sys.state != I2C_SYS_MASTER_WAIT))) {
		return -EBUSY;
	}
	
	// get a handle to the outgoing data
	i2c_sys.addr = dest_addr;
	if (i2c_sys.flags & I2C_SYS_SOS_MSG_FLAG) {
		i2c_sys.msgBuf = (Message*)buff;
		if (i2c_sys.msgBuf->len != msg_size) { // invalid msg packet
			i2c_sys.msgBuf = NULL;
			return -EINVAL;
		}
		i2c_sys.dataBuf = i2c_sys.msgBuf->data;
	} else {
		i2c_sys.dataBuf = buff;
	}

	if (i2c_sys.state == I2C_SYS_MASTER_WAIT) {
		if (i2c_startTransceiverTx(i2c_sys.addr, buff, msg_size, i2c_sys.flags) == SOS_OK) {
			i2c_sys.state = I2C_SYS_MASTER_TX;
			return SOS_OK;
		}
	} else {
		if (i2c_startTransceiverTx(i2c_sys.addr, buff, msg_size, i2c_sys.flags) == SOS_OK) {
			i2c_sys.state = I2C_SYS_SLAVE_TX;
			return SOS_OK;
		}
	}
	// lower layer busy (async slave recieve?)
	return -EBUSY;
}


/**
 * Read data from the i2c.
 */
int8_t ker_i2c_read_data(uint8_t dest_addr, uint8_t read_size, uint8_t calling_id) {

	if (i2c_sys.state == I2C_SYS_IDLE) {  // bus not initialized??
		return -EINVAL;
	}
	// Check if others are using the i2c
	if ((i2c_sys.calling_mod_id != calling_id) ||
			((i2c_sys.state != I2C_SYS_SLAVE_WAIT) && (i2c_sys.state != I2C_SYS_MASTER_WAIT))) {
		return -EBUSY;
	}
	i2c_sys.addr = dest_addr;
	i2c_sys.rxLen = read_size;

	if (i2c_sys.state == I2C_SYS_MASTER_WAIT) {
		if (i2c_startTransceiverRx(i2c_sys.addr, read_size, i2c_sys.flags) == SOS_OK) {
			i2c_sys.state = I2C_SYS_MASTER_RX;
			return SOS_OK;
		}
	} else {
		if (i2c_startTransceiverRx(i2c_sys.addr, read_size, i2c_sys.flags) == SOS_OK) {
			i2c_sys.state = I2C_SYS_SLAVE_RX;
			return SOS_OK;
		}
	}
	// lower layer busy (async slave recieve?)
	return -EBUSY;
}


static inline void resetTransceiver() {
	i2c_initTransceiver(i2c_sys.ownAddr, i2c_sys.flags);
	if(i2c_sys.flags & I2C_SYS_MASTER_FLAG) {
		i2c_sys.state = I2C_SYS_MASTER_WAIT;
	} else {
		i2c_sys.state = I2C_SYS_SLAVE_WAIT;
	}
}


/**
 * Send MSG_I2C_SEND_DONE
 */
void i2c_send_done(uint8_t status) {

	// figure out if we have someone to send this message to
	if ((i2c_sys.calling_mod_id != NULL_PID) &&
			(((i2c_sys.state == I2C_SYS_MASTER_TX) && (status & I2C_SYS_MASTER_FLAG)) || ((i2c_sys.state == I2C_SYS_SLAVE_TX)))) {
		/* bus was reserved by someone AND
		 * bus reserved as master and a send was requested, packet transmited as a master
		 * OR bus reserved as slave and a send was requested */
		
		if (status & I2C_SYS_ERROR_FLAG) {
			post_short(i2c_sys.calling_mod_id, I2C_PID, MSG_I2C_SEND_DONE, 0, 0, SOS_MSG_SEND_FAIL|SOS_MSG_HIGH_PRIORITY);
		} else if(status & I2C_SYS_RX_PEND_FLAG) {
			// This is the result of a read request done sending the destination
			// address and now waiting for the read done
			i2c_sys.state = (i2c_sys.flags & I2C_SYS_MASTER_FLAG)?I2C_SYS_MASTER_RX:I2C_SYS_SLAVE_RX;
			return;
		} else {
			post_short(i2c_sys.calling_mod_id, I2C_PID, MSG_I2C_SEND_DONE, 0, 0, SOS_MSG_HIGH_PRIORITY);
		}
		i2c_sys.state = (i2c_sys.flags & I2C_SYS_MASTER_FLAG)?I2C_SYS_MASTER_WAIT:I2C_SYS_SLAVE_WAIT;
	}
}


/**
 * Send MSG_I2C_READ_DONE
 */
void i2c_read_done(uint8_t *buff, uint8_t len, uint8_t status) {

	uint8_t *bufPtr = NULL;
	Message *msgPtr = NULL;

	// this is a problem that should be handled
	if (buff == NULL) {
		return;
	}

	if ((i2c_sys.calling_mod_id != NULL_PID) && (status & I2C_SYS_ERROR_FLAG)) {
		goto post_error_msg;
	}

	// the bus was reserved the read was of the length we requested
	if ((i2c_sys.calling_mod_id != NULL_PID) && (len == i2c_sys.rxLen) && 
			// the data was recieved in the correct mode
			(((i2c_sys.state == I2C_SYS_MASTER_RX) && (I2C_SYS_MASTER_FLAG & status)) ||
			 ((i2c_sys.state == I2C_SYS_SLAVE_RX) && !(I2C_SYS_MASTER_FLAG & status)))) {

		// reserved read done will only be raw reads, wrap in message and send
		post_long(
				i2c_sys.calling_mod_id,
				I2C_PID,
				MSG_I2C_READ_DONE,
				len,
				buff,
				SOS_MSG_RELEASE|SOS_MSG_HIGH_PRIORITY);
		i2c_sys.state = (i2c_sys.flags & I2C_SYS_MASTER_FLAG)?I2C_SYS_MASTER_WAIT:I2C_SYS_SLAVE_WAIT;
		return;
	}

	// there appers to be a bug in avr-gcc this a work around to make sure
	// the cast to message works correctly
	// the following code seems to cat the value 0x800008 independent of what
	// buff actually ii2c_sys.  this has no correlation to the data in buff or the 
	// value of the pointer the cast (along with many others that should) works
	// in gdb but fail to execute correctly when compilied
	/* bufPtr = &buff[HDLC_PROTOCOL_SIZE]; */

	// DO NOT CHANGE THIS SECTION OF CODE
	// start of section
	bufPtr = buff+1;
	// end of DO NOT CHANGE SECTION
		
	// if it passes sanity checks give it to the scheduler
	if (!(I2C_SYS_MASTER_FLAG & status) && (len >= SOS_MSG_HEADER_SIZE) && (buff[0] == HDLC_SOS_MSG)) {

		if ((len >= (HDLC_PROTOCOL_SIZE + SOS_MSG_HEADER_SIZE + ((Message*)bufPtr)->len + SOS_MSG_CRC_SIZE)) &&
				(len <= I2C_MAX_MSG_LEN)) {

			// please do not edit the next line
			bufPtr = buff;
			// we have enough bytes for it to be a message, lets start the copy out
			// XXX msgPtr = (Message*)ker_malloc(sizeof(Message), I2C_PID);
			msgPtr = msg_create();
			if (msgPtr !=NULL) {
				uint8_t i=0;
				uint16_t runningCRC=0, crc_in=0;

				// extract the protocol field
				for (i=0; i<HDLC_PROTOCOL_SIZE; i++) {
					runningCRC = crcByte(runningCRC, bufPtr[i]);
				}

				// extract the header
				bufPtr = &buff[HDLC_PROTOCOL_SIZE];
				for (i=0; i<SOS_MSG_HEADER_SIZE; i++) {
					((uint8_t*)msgPtr)[i] = bufPtr[i];
					runningCRC = crcByte(runningCRC, bufPtr[i]);
				}

				// extract the data if it exists
				if (msgPtr->len != 0) {
					uint8_t *dataPtr;
					dataPtr = ker_malloc(((Message*)msgPtr)->len, I2C_PID);

					if (dataPtr != NULL) {

						msgPtr->data = dataPtr;

						bufPtr = &buff[HDLC_PROTOCOL_SIZE+SOS_MSG_HEADER_SIZE];
						for (i=0; i<msgPtr->len; i++) {
							msgPtr->data[i] = bufPtr[i];
							runningCRC = crcByte(runningCRC, bufPtr[i]);
						}
					} else { // -ENOMEM
						ker_free(msgPtr);
						goto post_error_msg;
					}
				} else {
					msgPtr->data = NULL;
				}

				// get the CRC and check it
				bufPtr = &buff[HDLC_PROTOCOL_SIZE+SOS_MSG_HEADER_SIZE+msgPtr->len];
				crc_in = bufPtr[0] | (bufPtr[1]<<8);

				if (crc_in == runningCRC) {
					// message passed all sanity checks including crc
					LED_DBG(LED_YELLOW_TOGGLE);
					if(msgPtr->data != NULL ) {
						msgPtr->flag = SOS_MSG_RELEASE;
					}
					handle_incoming_msg(msgPtr, SOS_MSG_I2C_IO);
					return;
				} else { // clean up
					ker_free(msgPtr->data);
					ker_free(msgPtr);
				}
			}
		}
	}

	// if we make it to here return error message
post_error_msg:
	post_short(
			i2c_sys.calling_mod_id,
			I2C_PID,
			MSG_ERROR,
			READ_ERROR,
			0,
			SOS_MSG_HIGH_PRIORITY);
}

