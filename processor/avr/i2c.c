/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @file i2c.c
 * @brief AVR specific I2C driver
 * @author AVR App Note 311 and 315
 * @author Modified by Roy Shea
 *
 * This work is directly from the Atmel AVR application notes AVR311
 * and AVR315.
 **/

#include <hardware.h>
#include <net_stack.h>
#include <sos_info.h>
#include <crc.h>
#include <measurement.h>
#include <malloc.h>

#include <hdlc.h>

#include <i2c_const.h>
#include <i2c.h>
#include <i2c_system.h>


#ifndef I2C_ADDRESS
#error I2C_ADDRESS must be defined
#endif

/**
 * most flags may need additional context to be used correctly:
 * 
 * I2C_SOS_MSG_FLAG && i2c.msgBuf != NULL means we are in sos_msg mode
 *   sos_msg mode is permited ONLY for Master Tx operation.  it is assumed
 *   that a device sending a sos_msg will have the ability to operate as a
 *   peer master on a multi master bus.  if this turns out not to be acceptable
 *   the driver will have to be rewritten in the future.
 * 
 * I2C_SOS_MSG_FLAG && i2c.msgBuf == NULL is the same as
 * I2C_SOS_CRC_FLAG && i2c.msgBuf == NULL and means we have a crc'ed raw packet
 *
 * I2C_RX_PEND_FLAG is used when the bus has been reserved for a write/read operation
 *   this allows the driver to maintain necessary state over the two operations and will
 *   be extended to perform this as a single operation.  for now the application will
 *   recieve a call back and WITHOUT releasing the bus will be able to initate the read.
 * 
 * I2C_BUFF_DIRTY_FLAG is interpereted in the context of the call back
 *   in a read done callback this can be interperated as data ready
 *   in a send done callback this means that a partial send occured and unsent
 *   data remains in the buffer.  for now the only thing the calling app can do
 *   is ker_free the buff.  in the future a compleate sending may be implimented.
 *   this is used to differentate from compleate send fails indicated by the
 *   I2C_ERROR_FLAG
 *
 * some of the flags are only set in the interrupt and can not be set by the user
 */

#define I2C_SOS_MSG_FLAG     I2C_SYS_SOS_MSG_FLAG  // sos_msg type or if msgBuf == NULL crc enabled
#define I2C_CRC_FLAG         I2C_SOS_MSG_FLAG      // overloaded flag
#define I2C_TX_FLAG          I2C_SYS_TX_FLAG       // msg tx
#define I2C_RX_PEND_FLAG     I2C_SYS_RX_PEND_FLAG  // tx with rx pending (no send done sent)
#define I2C_MASTER_FLAG      I2C_SYS_MASTER_FLAG   // bus used as master
#define I2C_BUFF_DIRTY_FLAG  0x08                  // data buff dirty 
#define I2C_GEN_ADDR_FLAG    0x04
#define I2C_BUFF_ERR_FLAG    0x02
#define I2C_ERROR_FLAG       I2C_SYS_ERROR_FLAG

#define I2C_NULL_FLAG        0x00

#define TW_STATUS_MASK  0xF8
// Unused since TWPS0=TWPS1=0

enum {
	I2C_IDLE=0,
	I2C_MASTER_IDLE,
	I2C_SLAVE_IDLE,
	I2C_MASTER_ARB,
	I2C_MASTER_TX,
	I2C_MASTER_RX,
	I2C_SLAVE_WAIT,
	I2C_SLAVE_TX,
	I2C_SLAVE_RX,
	I2C_SLAVE_BLOCKED,
	I2C_DATA_READY,
	I2C_TX_ERROR,
	I2C_RX_ERROR,
};


typedef struct i2c_state {
	uint8_t state;
	uint8_t msg_state;

	uint8_t ownAddr;
	uint8_t addr;

	uint16_t crc;        // shared msg values
	uint8_t msgLen;
	uint8_t idx;
	
	Message *msgBuf;  // used for master planed (tx/rx)
	uint8_t *dataBuf;
	uint8_t *rxDataBuf;
	uint8_t txPending;
	
	uint8_t rxStatus;    // status of last rx
	
	uint8_t flags;
} i2c_state_t;


static i2c_state_t i2c;


/**
 * helper functions to save and restore system state
 */
static uint8_t priorState = I2C_IDLE;
static inline void saveState(uint8_t currentState) {
	priorState = currentState;
}
static inline uint8_t restoreState(void) {
	switch (priorState) {
		case I2C_MASTER_ARB:
		case I2C_SLAVE_WAIT:
		default:
			return priorState;
	}
}


/*****************************************
 Initialize the I2C hardware on an AVR
 *****************************************/
static bool i2c_initialized = false;

int8_t i2c_hardware_init() {
	HAS_CRITICAL_SECTION;

	if (i2c_initialized == false) {
		ENTER_CRITICAL_SECTION();

		PORTD |= 0x03; // Enable the internal TWI pull up registers
		i2c_setCtrlReg(0); // TWI Interface disabled

		// need to fix this to be dynamic
		TWBR = TWI_TWBR; // Set bit rate register (Baudrate). Defined in header file.
		//TWSR = TWI_TWPS; // Not used. Driver presumes prescaler to be 00.
		LEAVE_CRITICAL_SECTION();

		i2c_initialized = true;
	}

    return SOS_OK;
}


int8_t i2c_initTransceiver(uint8_t ownAddress, uint8_t flags) {
	HAS_CRITICAL_SECTION;

    // Set own TWI slave address. Accept TWI General Calls.
	i2c.ownAddr = ((ownAddress<<1)&0xFE);

	if (ownAddress != 0x7f) {
		// 1111xxx is reserved addres space so 0x7f is an
		// invalid address and it is safe to use it as a flag
		// we will also enable the general call recognition bit
		TWAR = i2c.ownAddr|(1<<TWGCE);
	} else {
		if (!(flags&I2C_MASTER_FLAG)){
			// can not give a slave an invalid address
			return -EINVAL;
		}
	}

	// get flag settings from the upper layer
	i2c.flags = I2C_SYS_SHARED_FLAGS_MSK & flags;

	// do some clean up
	i2c.msgLen = 0;
	i2c.txPending = 0;
	i2c.idx = 0;

	// free all allocated buffers
	if (i2c.msgBuf != NULL) {
		i2c.dataBuf = NULL;
		i2c.msgBuf = NULL;
	}
	i2c.msg_state = SOS_MSG_NO_STATE;
	

  /**
   * \bug I2C system may want to NULL the i2c.dataBuf after i2c_send_done,
   * i2c_read_done, and any error.  We could then verify that i2c.dataBuf is
   * null in i2c_initTranceiver and call ker_panic if it is not null.  Same
   * goes for the rxDataBuf
   */
  // Roy: This results in a hard to track bug.  If the user frees I2C data
  // after the I2C sends a MSG_I2C_SEND_DONE, this will free it a second time.
  // Kernel messaging avoids this by explitily setting i2c.dataBuff to null.
  // Well, maybe.  Maybe not...  Regardless, this is bad!  
  //
  // if (i2c.dataBuf != NULL) {
  //  ker_free(i2c.dataBuf);
	//}

  // The problem described abave is NOT a problem with the i2c.rxDataBuf.  A
  // call to i2c_read_done creates a deep copy of the rxDataBuf that is
  // SOS_MSG_RELEASE'ed.  Thus, the rxDataBuff can hang around.  A down side
  // to the current implementation is that the rxDataBuf is not released with
  // when the buffer is released.  We do not leak this data, but it is not
  // made availible to the system. 
  //
  // pre allocate recieve buffer
  if (i2c.rxDataBuf != NULL) {
    ker_free(i2c.rxDataBuf);
	}
  
	i2c.rxDataBuf = ker_malloc(I2C_MAX_MSG_LEN, I2C_PID);
	if (i2c.rxDataBuf == NULL) {
		return -ENOMEM;
	}

	ENTER_CRITICAL_SECTION();
	if (i2c.flags & I2C_MASTER_FLAG) {
		i2c.state = I2C_MASTER_IDLE;
	} else {
		i2c.state = I2C_SLAVE_IDLE;
	}
	// enable TWI interrupt and ack
	i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t i2c_startTransceiverTx(
		uint8_t addr,
		uint8_t *msg,
		uint8_t msg_len,
		uint8_t flags) {

	HAS_CRITICAL_SECTION;

	uint8_t alt_state = (i2c.flags & I2C_MASTER_FLAG)?I2C_MASTER_IDLE:I2C_SLAVE_IDLE;

	if (!((i2c.state == I2C_IDLE) || (i2c.state == alt_state))) {
		return -EBUSY;
	}
	ENTER_CRITICAL_SECTION();
	i2c.flags = (I2C_SYS_SHARED_FLAGS_MSK & flags)|I2C_BUFF_DIRTY_FLAG;
    i2c.addr = (addr<<1);  // shift destination address once (and only once!)

	if (i2c.flags & I2C_SOS_MSG_FLAG) {
		i2c.msgBuf = (Message*)msg;
		i2c.msgLen = i2c.msgBuf->len;  // expected msg len
		i2c.txPending = SOS_MSG_HEADER_SIZE + i2c.msgLen + SOS_MSG_CRC_SIZE;
		i2c.dataBuf = i2c.msgBuf->data;
	} else {
		i2c.dataBuf = msg;
		i2c.msgLen = msg_len;
		i2c.txPending = i2c.msgLen;
	}
	i2c.msg_state = SOS_MSG_WAIT;
	i2c.idx = 0;

    // TWI Interface enabled.
    // Enable TWI Interupt and clear the flag.
    // Initiate a START condition.
	saveState(i2c.state);
	if (i2c.flags & I2C_MASTER_FLAG) {
		i2c.state = I2C_MASTER_ARB;
		i2c_setCtrlReg((1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE));
	} else { // else sit and wait
		i2c.state = I2C_SLAVE_WAIT;
		i2c_setCtrlReg((1<<TWEA)|(1<<TWEN)|(1<<TWIE));
	}
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


int8_t i2c_startTransceiverRx(uint8_t addr,
		uint8_t rx_msg_len,
		uint8_t flags) {

	HAS_CRITICAL_SECTION;

	uint8_t alt_state = (i2c.flags & I2C_MASTER_FLAG)?I2C_MASTER_IDLE:I2C_SLAVE_IDLE;

	while (!((i2c.state == I2C_IDLE) || (i2c.state == alt_state))) {
		return -EBUSY;
	}
	if (flags & I2C_SOS_MSG_FLAG) {
		return -EINVAL;
	}
	i2c.flags = I2C_SYS_SHARED_FLAGS_MSK & flags;

	if (rx_msg_len >= I2C_MAX_MSG_LEN) {
		return -ENOMEM;
	}
 
  /** 
   * \bug Um, maybe this check is not such a good idea.  i2c.dataBuf needs to
   * be null.  This should be stronger and take the form of an assert such as:
   *
   * assert(i2c.dataBuf == NULL);
   *
   * But no asserts for us :-(
   */
  //if (i2c.dataBuf == NULL) {
    if ((i2c.dataBuf = ker_malloc(rx_msg_len, I2C_PID)) == NULL) { 
      return -ENOMEM; 
    }
  //}
  i2c.msgLen = rx_msg_len; i2c.idx = 0;

	ENTER_CRITICAL_SECTION();
	i2c.addr = (addr<<1);  // shift destination address once (and only once!)

	i2c.msg_state = SOS_MSG_WAIT;

	saveState(i2c.state);
	if (i2c.flags & I2C_MASTER_FLAG) {
		i2c.state = I2C_MASTER_ARB;
		i2c_setCtrlReg((1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE));
	} else { // else sit and wait
		i2c.state = I2C_SLAVE_WAIT;
		i2c_setCtrlReg((1<<TWEA)|(1<<TWEN)|(1<<TWIE));
	}
	i2c.msg_state = SOS_MSG_RX_START;
	LEAVE_CRITICAL_SECTION();

	return SOS_OK;
}


// ********* Interrupt Handlers ********** //

/**
 * This function is the Interrupt Service Routine (ISR), and called when the
 * TWI interrupt is triggered; that is whenever a TWI event has occurred. This
 * function should not be called directly from the main application.
 */
#define MAX_ADDR_FAIL 3

i2c_interrupt() {

	static uint8_t addrFailCnt;

    SOS_MEASUREMENT_IDLE_END();

		// TWSR & TW_STATUS_MASK
		switch (TWSR) {
			/*************************
			 * Master General States *
			 *************************/
			case TWI_START: /* 0x08 */
				addrFailCnt = 0;
				i2c.idx = 0;
        // Fall through!

			case TWI_REP_START: /* 0x10 */
				i2c_setByte(i2c.addr|((i2c.flags & I2C_TX_FLAG)?0:1));  // only set R/W bit if reading
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEN)|(1<<TWIE));
				break;

			case TWI_ARB_LOST: /* 0x38 */
				i2c_setCtrlReg((1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE)); // Initate a (RE)START condition
				// TWI hardware will resend start when bus is free and signal TWI_RESTART
				break;


		
				/*****************************
				 * Master Transmitter States *
				 *****************************/
			case TWI_MTX_ADR_ACK: /* 0x18 */
				i2c.state = I2C_MASTER_TX;
				if (i2c.flags & I2C_SOS_MSG_FLAG) {
					// this is a difference between the i2c and uart
					// uart uses a protocol byte for BOTH raw and sos_msgs
					// i2c ONLY sends a protocol byte in the case of a sos_msg
					i2c_setByte(HDLC_SOS_MSG);
					i2c.msg_state = SOS_MSG_TX_HDR;
					// this is a sos_msg so a crc is required
					i2c.crc = crcByte(0, HDLC_SOS_MSG);
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEN)|(1<<TWIE));
					break;
				} else {
					i2c.msg_state = SOS_MSG_TX_RAW;
				} // fall through

			case TWI_MTX_DATA_ACK: /* 0x28 */
				switch (i2c.msg_state) {
					case SOS_MSG_TX_HDR:
						i2c_setByte(((uint8_t*)(i2c.msgBuf))[i2c.idx]);
						// this is a sos_msg so a crc is required
						i2c.crc = crcByte(i2c.crc, ((uint8_t*)(i2c.msgBuf))[i2c.idx]);
						i2c.idx++;
						if (i2c.idx == SOS_MSG_HEADER_SIZE) {
							i2c.idx = 0;
							i2c.txPending = i2c.msgLen + SOS_MSG_CRC_SIZE;
							i2c.msg_state = SOS_MSG_TX_DATA;
						}
						break;

					case SOS_MSG_TX_RAW:
					case SOS_MSG_TX_DATA:
						i2c_setByte(i2c.dataBuf[i2c.idx]);
						if (i2c.flags & I2C_CRC_FLAG) {
							i2c.crc = crcByte(i2c.crc, i2c.dataBuf[i2c.idx]);
						}
						i2c.idx++;
						if (i2c.idx == i2c.msgLen) {
							if (!(i2c.flags & I2C_CRC_FLAG)) {
								// send stop bit and reset interface to ready state
								i2c.txPending = 0;
								i2c.msg_state = SOS_MSG_TX_END;
							} else {
								i2c.txPending = SOS_MSG_CRC_SIZE;  // no unsent bytes
								i2c.msg_state = SOS_MSG_TX_CRC_LOW;
							}
						}
						break;

					case SOS_MSG_TX_CRC_LOW:
						i2c_setByte((uint8_t)(i2c.crc));
						i2c.txPending--;
						i2c.msg_state = SOS_MSG_TX_CRC_HIGH;
						break;

					case SOS_MSG_TX_CRC_HIGH:
						i2c_setByte((uint8_t)(i2c.crc>>8));
						i2c.txPending--;
						i2c.msg_state = SOS_MSG_TX_END;
						break;

					case SOS_MSG_TX_END:
						// send stop bit and reset interface to ready state
						i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE));
						i2c_send_done(i2c.flags);
						i2c.state = restoreState();
						return;
					default:
						break;
				}
				// normal send byte all stop conditions must return and not get here
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEN)|(1<<TWIE));
				break;

			case TWI_MTX_ADR_NACK: /* 0x20 */
				if (addrFailCnt++ < MAX_ADDR_FAIL) {
					// try restarting MAX_ADDR_FAIL times then fail
					i2c_setCtrlReg((1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE));
				} else {
					// reset i2c and send msg fail to process
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE));
					i2c_send_done(i2c.flags|I2C_ERROR_FLAG);
					i2c.state = restoreState();
				}
				break;

			case TWI_MTX_DATA_NACK: /* 0x30 */
				// reset i2c and send msg fail to process
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE));
				i2c.txPending = i2c.txPending - i2c.idx + 1; // last byte failed
				i2c_send_done(i2c.flags|I2C_BUFF_ERR_FLAG);
				i2c.state = restoreState();
				break;


			
				/***************************
				 * Master Receiver  States *
				 ***************************/
			case TWI_MRX_ADR_ACK: /* 0x40 */
				i2c.state = I2C_MASTER_RX;
				i2c.rxStatus = i2c.flags;
				// all master rx are done in raw mode with NO protocol byte
				i2c.msg_state = SOS_MSG_RX_RAW;
				// a sos message will never be recieved as a manster
				if (i2c.msgLen == 1) {
					i2c.msg_state = SOS_MSG_RX_END;
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEN)|(1<<TWIE));
				} else {
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				}
				break;

			case TWI_MRX_DATA_ACK: /* 0x50 */
				i2c.dataBuf[i2c.idx++] = i2c_getByte();

				if (i2c.idx < (i2c.msgLen-1)) {
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				} else { // unset TWEA (Send NACK after next/last byte)
					i2c.msg_state = SOS_MSG_RX_END;
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEN)|(1<<TWIE));
				}
				break;

			case TWI_MRX_ADR_NACK: /* 0x48 */
				if (addrFailCnt++ < MAX_ADDR_FAIL) {
					// tryrestarting MAX_ADDR_FAIL times then fail
					i2c_setCtrlReg((1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE));
					break;
				}
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE));

				// return data
				i2c_read_done(i2c.dataBuf, i2c.idx, i2c.rxStatus);
				i2c.idx = 0;
				i2c.rxStatus = 0;

				i2c.state = restoreState();
				break;

			case TWI_MRX_DATA_NACK: /* 0x58 */
				i2c.dataBuf[i2c.idx++] = i2c_getByte();

				if (i2c.idx < i2c.msgLen) {
					// nack from master indication rx done
					// send stop bit, clear interrupt and reset interface to ready state
					i2c.rxStatus |= I2C_BUFF_ERR_FLAG;
				}

				// set flags and return data
				i2c.rxStatus |= I2C_BUFF_DIRTY_FLAG;
				
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE));
				
				i2c_read_done(i2c.dataBuf, i2c.idx, i2c.rxStatus);
				i2c.idx = 0;
				i2c.rxStatus = 0;
				
				i2c.state = restoreState();
				break;


			
				/****************************
				 * Slave Transmitter States *
				 ****************************/
			case TWI_STX_ADR_ACK: /* 0xA8 */
			case TWI_STX_ADR_ACK_M_ARB_LOST: /* 0xB0 */
				if (i2c.state != I2C_SLAVE_WAIT) {
					i2c_send_done(i2c.flags|I2C_ERROR_FLAG);
					break;
				} else {
					saveState(i2c.state);
					i2c.state = I2C_SLAVE_TX;
					i2c.msg_state = SOS_MSG_TX_RAW;
				}
				// fall through

			case TWI_STX_DATA_ACK: /* 0xB8 */
				if (i2c.msg_state == SOS_MSG_TX_RAW) {
					i2c_setByte(i2c.dataBuf[i2c.idx++]);

					// unset TWEA (Send NACK after next/last byte)
					if (i2c.msgLen == i2c.idx) {
						i2c_setCtrlReg((1<<TWINT)|(1<<TWEN)|(1<<TWIE));
						i2c.txPending = 1; // last byte failed
						i2c.msg_state = SOS_MSG_TX_END;
					} else { // Reset the TWI Interupt to wait to ack next byte
						i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
					}
				}
				break;

			case TWI_STX_DATA_NACK: /* 0xC0 */
				// Master has sent a NACK before expected amount of data was sent.
				// set dirty bit on send buffer go to end state and issue send done
				if (i2c.msg_state != SOS_MSG_TX_END) { 
					i2c.txPending = i2c.txPending - i2c.idx + 1; // last byte failed
					i2c.msg_state = SOS_MSG_TX_END;
				} else {
					i2c.txPending = 0;
				}

				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));

				if (i2c.msg_state == SOS_MSG_TX_END) {
					i2c.msg_state = SOS_MSG_NO_STATE;
					i2c_send_done(i2c.flags);
				}
				break;

			case TWI_STX_DATA_ACK_LAST_BYTE: /* 0xC8 */
				i2c.msg_state = SOS_MSG_NO_STATE;
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				i2c_send_done(i2c.flags|I2C_BUFF_ERR_FLAG);
				break;



				/*************************
				 * Slave Receiver States *
				 *************************/
				// all receptions are done in a raw mode
				// if it is a sos message it it will be packed later
			case TWI_SRX_GEN_ACK: /* 0x70 */
			case TWI_SRX_GEN_ACK_M_ARB_LOST: /* 0x78 */
				i2c.rxStatus = I2C_GEN_ADDR_FLAG;
				// fall through

			case TWI_SRX_ADR_ACK: /* 0x60 */
			case TWI_SRX_ADR_ACK_M_ARB_LOST: /* 0x68 */
				saveState(i2c.state);
				i2c.state = I2C_SLAVE_RX;
				i2c.msg_state = SOS_MSG_RX_RAW;
				i2c.idx = 0;
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				break;

			case TWI_SRX_ADR_DATA_ACK: /* 0x80 */
			case TWI_SRX_GEN_DATA_ACK: /* 0x90 */
				if (i2c.msg_state == SOS_MSG_RX_RAW) {
					i2c.rxDataBuf[i2c.idx++] = i2c_getByte();
					if (i2c.idx >= I2C_MAX_MSG_LEN) { // buffer overflow
						i2c.msg_state = SOS_MSG_RX_END;
						
						// set flags and return data
						i2c.rxStatus |= (I2C_BUFF_DIRTY_FLAG|I2C_BUFF_ERR_FLAG);
						i2c_read_done(i2c.rxDataBuf, i2c.idx, i2c.rxStatus);
						i2c.idx = 0;
						i2c.rxStatus = 0;
					}
				}
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				break;

			case TWI_SRX_ADR_DATA_NACK: /* 0x88 */
			case TWI_SRX_GEN_DATA_NACK: /* 0x98 */
				// switch to not addressed mode
				if (i2c.msg_state == SOS_MSG_RX_RAW) {
					
					// set flags and return data
					i2c.rxStatus |= I2C_BUFF_DIRTY_FLAG;
					i2c_read_done(i2c.rxDataBuf, i2c.idx, i2c.rxStatus);
					i2c.idx = 0;
					i2c.rxStatus = 0;
					i2c.msg_state = SOS_MSG_RX_END;
				}
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				break;

			case TWI_SRX_STOP_RESTART: /* 0xA0 */
				// reset reciever
				i2c.msg_state = SOS_MSG_NO_STATE;
				if (i2c.idx > 0) {
					// need to make sure data has been read
					i2c.rxStatus |= I2C_BUFF_DIRTY_FLAG;
					i2c_read_done(i2c.rxDataBuf, i2c.idx, i2c.rxStatus);
					i2c.idx = 0;
					i2c.rxStatus = 0;
				}
				i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
				i2c.state = restoreState();
				break;

				
			
				/***************
				 * Misc States *
				 ***************/
				//case TWI_NO_STATE: /* 0xF8 */
			case TWI_BUS_ERROR: /* 0x00 */
			default:
				{
					uint8_t twiStatus=0;
					// Store TWSR and automatically sets clears noErrors bit.
					twiStatus = TWSR;
					// Clear TWINT and reset TWI Interface
					i2c_setCtrlReg((1<<TWINT)|(1<<TWEA)|(1<<TWEN)|(1<<TWIE));
					i2c.state = I2C_IDLE;
					// should really be i2c_error
					i2c_send_done(I2C_NULL_FLAG);
					break;
				}
		}
}

