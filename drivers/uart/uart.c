/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief    sos_uart messaging layer
 * @author	 Naim Busek <ndbusek@gmail.com>
 *
 */
#include <hardware.h>
#include <message_queue.h>
#include <net_stack.h>
#include <sos_info.h>
#include <crc.h>
#include <measurement.h>
#include <malloc.h>
#include <sos_timer.h>
#include <uart_system.h>
#include <sos_uart_mgr.h>

#include <hdlc.h>
#include <uart_hal.h>
#include <uart.h>
#include <sos_uart.h>

//#define LED_DEBUG
#include <led_dbg.h>

// flags for uart_system
#define UART_SOS_MSG_FLAG     UART_SYS_SOS_MSG_FLAG
#define UART_CRC_FLAG         UART_SYS_SOS_MSG_FLAG
#define UART_TX_FLAG          UART_SYS_TX_FLAG
#define UART_DATA_RDY_FLAG    0x20
#define UART_RSVRD_3_FLAG     0x10
#define UART_RSVRD_2_FLAG     0x08
#define UART_RSVRD_1_FLAG     0x04
#define UART_RSVRD_0_FLAG     0x02
#define UART_ERROR_FLAG       UART_SYS_ERROR_FLAG

#define UART_NULL_FLAG        0x00


enum {
	UART_INIT=0,
	UART_IDLE,        //! enabled waiting
	UART_HDLC_START,	//! hdlc start framing byte
	UART_PROTOCOL,	  //! hdlc protocol byte
	UART_DATA,				//! pkt payload
	UART_HDLC_STOP,   //! hdlc stop/inter frame byte
	UART_END,         //! hdlc stop send
};


typedef struct uart_state {
	uint8_t state;
	uint8_t msg_state;
	uint8_t hdlc_state;

	Message *msgHdr;      //!< sosMsg handle
	uint8_t *buff;        //!< buff/sosMsg data
	uint16_t crc;         //!< running crc
	uint8_t msgLen;       //!< buff/sosMsg data len
	uint8_t idx;          //!< byte index

	uint8_t flags;
} uart_state_t;

#define TX 0
#define RX 1
static uart_state_t state[2];

void uart_init(void) {
	
	uint8_t i=0;

	state[TX].state = UART_INIT;
	state[RX].state = UART_INIT;

	uart_hardware_init();

	for (i=0;i<2; i++) {
		state[i].msgHdr = NULL;
		state[i].crc = 0;
		state[i].buff = NULL;
		state[i].idx = 0;

		state[i].flags = 0;
		
		state[i].msg_state = SOS_MSG_NO_STATE;
		state[i].hdlc_state = HDLC_IDLE;
		state[i].state = UART_IDLE;
	}
}

static inline void uart_send_byte(uint8_t byte) {
	static uint8_t saved_state;

	if ((state[TX].flags & UART_CRC_FLAG) && (state[TX].hdlc_state == HDLC_DATA)) {
		state[TX].crc = crcByte(state[TX].crc, byte);
	}
	if (state[TX].hdlc_state == HDLC_ESCAPE) {
		uart_setByte(0x20 ^ byte);
		state[TX].hdlc_state = saved_state;
	} else if ((byte == HDLC_FLAG) || (byte == HDLC_CTR_ESC) || (byte == HDLC_EXT)) {
		saved_state = state[TX].hdlc_state;
		state[TX].hdlc_state = HDLC_ESCAPE;
		uart_setByte(HDLC_CTR_ESC);
		return;
	} else {
		uart_setByte(byte);
	}
	state[TX].idx++;
}


uint8_t uart_getState(uint8_t flags) {
	if (flags & UART_SYS_TX_FLAG) {
		return state[TX].state;
	}
	return state[RX].state;
}


int8_t uart_startTransceiverTx( uint8_t *msg, uint8_t msg_len, uint8_t flags) {

	if (state[TX].state != UART_IDLE) {
		//DEBUG("uart_startTransceiverTx Fail!!! ***\n");
		return -EBUSY;
	}

	state[TX].flags = UART_SYS_SHARED_FLAGS_MSK & flags;  // get shared flags
	if (state[TX].flags & UART_SOS_MSG_FLAG) {
		state[TX].msgHdr = (Message*)msg;
		state[TX].msgLen = state[TX].msgHdr->len; // if msg->len != msg_len ???
		state[TX].buff = state[TX].msgHdr->data;
	} else {
		state[TX].buff = msg;
		state[TX].msgLen = msg_len;
	}
	state[TX].idx = 0;
	state[TX].state = UART_HDLC_START;
	state[TX].hdlc_state = HDLC_START;

	uart_enable_tx();
	uart_setByte(HDLC_FLAG);
	//DEBUG("uart_startTransceiverTx Start!!! ***\n");

	return SOS_OK;
}


int8_t uart_startTransceiverRx( uint8_t rx_len, uint8_t flags) {

	if (state[RX].state != UART_IDLE) {
		return -EBUSY;
	}

	state[RX].flags = UART_SYS_SHARED_FLAGS_MSK & flags;  // get shared flags
	if (flags & UART_SOS_MSG_FLAG) {
		state[RX].msgHdr = msg_create();
		if (state[RX].msgHdr == NULL) {
			return -ENOMEM;
		}
	} else {
		state[RX].buff = ker_malloc(rx_len, UART_PID);
		if (state[RX].buff == NULL) {
			return -ENOMEM;
		}
	}
	state[RX].msgLen = rx_len;  // expected rx_len

	state[RX].idx = 0;
	state[RX].crc = 0;
	state[RX].state = UART_HDLC_START;
	state[RX].hdlc_state = HDLC_START;
	
	uart_enable_rx();
	
	return SOS_OK;
}


uint8_t *uart_getRecievedData(void) {
	HAS_CRITICAL_SECTION;

	if (state[RX].flags & UART_DATA_RDY_FLAG) {
		ENTER_CRITICAL_SECTION();
		state[RX].flags &= ~UART_DATA_RDY_FLAG;
		LEAVE_CRITICAL_SECTION();

		return (state[RX].flags & UART_SOS_MSG_FLAG)?(uint8_t*)state[RX].msgHdr:state[RX].buff;
	}

	return NULL;
}


/* ISR for transmittion */
#ifndef DISABLE_UART
uart_send_interrupt() {
	SOS_MEASUREMENT_IDLE_END();
	LED_DBG(LED_GREEN_TOGGLE);

	//DEBUG("uart_send_interrupt %d %d %d\n", state[TX].state, state[TX].msg_state,
	//		state[TX].hdlc_state);
	switch (state[TX].state) {
		case UART_HDLC_START:
			uart_setByte((state[TX].flags & UART_SOS_MSG_FLAG)?HDLC_SOS_MSG:HDLC_RAW);
			state[TX].hdlc_state = HDLC_DATA;
			state[TX].state = UART_PROTOCOL;
			if (state[TX].flags & UART_CRC_FLAG) {
				state[TX].crc = crcByte(0, (state[TX].flags & UART_SOS_MSG_FLAG)?HDLC_SOS_MSG:HDLC_RAW);
			}
			break;

		case UART_PROTOCOL:
			state[TX].state = UART_DATA;
			if (state[TX].flags & UART_SOS_MSG_FLAG) {
				state[TX].msg_state = SOS_MSG_TX_HDR;
			} else {
				state[TX].msg_state = SOS_MSG_TX_RAW;
			}
			// set state and fall through
		case UART_DATA:
			switch (state[TX].msg_state) {
				case SOS_MSG_TX_HDR:
					uart_send_byte(((uint8_t*)(state[TX].msgHdr))[state[TX].idx]);
					if ((state[TX].idx == SOS_MSG_HEADER_SIZE) && (state[TX].hdlc_state != HDLC_ESCAPE)) {
						state[TX].idx = 0;
						if (state[TX].msgHdr->len != 0) {
							state[TX].msg_state = SOS_MSG_TX_DATA;
						} else {
							state[TX].hdlc_state = HDLC_CRC;
							state[TX].msg_state = SOS_MSG_TX_CRC_LOW;
						}
					}
					break;

				case SOS_MSG_TX_DATA:
				case SOS_MSG_TX_RAW:
					uart_send_byte(state[TX].buff[state[TX].idx]);
					if ((state[TX].idx == state[TX].msgLen) && (state[TX].hdlc_state != HDLC_ESCAPE)) {
						if (state[TX].flags & UART_CRC_FLAG) {
							state[TX].hdlc_state = HDLC_CRC;
							state[TX].msg_state = SOS_MSG_TX_CRC_LOW;
						} else { // no crc
							state[TX].state = UART_END;
							uart_setByte(HDLC_FLAG);
						}
					}
					break;

				case SOS_MSG_TX_CRC_LOW:
					uart_send_byte((uint8_t)(state[TX].crc));
					if (state[TX].hdlc_state != HDLC_ESCAPE) { //! crc was escaped, resend
						state[TX].msg_state = SOS_MSG_TX_CRC_HIGH;
					}
					break;

				case SOS_MSG_TX_CRC_HIGH:
					uart_send_byte((uint8_t)(state[TX].crc >> 8));
					if (state[TX].hdlc_state != HDLC_ESCAPE) { //! resend low byte
						state[TX].msg_state = SOS_MSG_TX_END;
						state[TX].state = UART_HDLC_STOP;
					}
					break;
				default:
					break;
			}
			break;

		case UART_HDLC_STOP:
			uart_setByte(HDLC_FLAG);
			state[TX].state = UART_END;
			state[TX].msg_state = SOS_MSG_NO_STATE;
			break;

		case UART_END:
			//DEBUG("disable Tx in uart.c\n");
			uart_disable_tx();
			state[TX].state = UART_IDLE;
			state[TX].hdlc_state = HDLC_IDLE;
			uart_send_done(state[TX].flags & ~UART_ERROR_FLAG);
			//DEBUG("uart_disable_tx\n");
			break;

		default:
			//DEBUG("In Default...\n");
			//DEBUG("disable Tx in uart.c\n");
			uart_disable_tx();
			state[TX].flags |= UART_ERROR_FLAG;
			state[TX].state = UART_IDLE;
			state[TX].hdlc_state = HDLC_IDLE;
			uart_send_done(state[TX].flags & ~UART_ERROR_FLAG);
			break;
	}
	//DEBUG("end uart_send_interrupt %d %d %d\n", state[TX].state, state[TX].msg_state,
	//		state[TX].hdlc_state);
}

/*
 * should be doing the senddone at this layer
	 Message *msg_txed;   //! message just transmitted
	 msg_txed = s.msgHdr;
	 msg_send_senddone(msg_txed, true, UART_PID);
	 s.msgHdr = mq_dequeue(&uartpq);
	 uart_setByte(HDLC_FLAG);
	 if(s.msgHdr){
	 s.idx = 0;
	 s.crc = 0;
	 s.state = UART_HDLC_START;
	 } else { //! stop and disable interrupt, if buffer is empty
	 s.state = UART_END;
	 }
	 */

static inline void uart_reset_recv() {

	if(state[RX].msgHdr != NULL) {
		msg_dispose(state[RX].msgHdr);
		state[RX].msgHdr = NULL;
	}
	state[RX].state = UART_IDLE;
	state[RX].msg_state = SOS_MSG_NO_STATE;
	state[RX].hdlc_state = HDLC_IDLE;
}
	
/**
 * @brief ISR for reception
 * This is the writer of rx_queue.
 */
uart_recv_interrupt() {
	
	uint8_t err;
	uint8_t byte_in;
	static uint16_t crc_in;
	static uint8_t saved_state;
	SOS_MEASUREMENT_IDLE_END()
	LED_DBG(LED_YELLOW_TOGGLE);
	//! NOTE that the order has to be this in AVR
	err = uart_checkError();
	byte_in = uart_getByte();

	//DEBUG("uart_recv_interrupt... %d %d %d %d %d\n", byte_in, err, 
	//		state[RX].state, state[RX].msg_state, state[RX].hdlc_state);
	switch (state[RX].state) {
		case UART_IDLE:
			if ((err != 0) || (byte_in != HDLC_FLAG)) {
				break;
			}
			state[RX].state = UART_HDLC_START;
			break;

		case UART_HDLC_START:
		case UART_PROTOCOL:
			if (err != 0) {
				uart_reset_recv();
				break;
			}

			switch (byte_in) {
				//! ignore repeated start symbols
				case HDLC_FLAG:
					state[RX].state = UART_HDLC_START;
					break;

				case HDLC_SOS_MSG:
					if(state[RX].msgHdr == NULL) {
						state[RX].msgHdr = msg_create();
					} else {
						if((state[RX].msgHdr->data != NULL) &&
								(flag_msg_release(state[RX].msgHdr->flag))){
							ker_free(state[RX].msgHdr->data);
							state[RX].msgHdr->flag &= ~SOS_MSG_RELEASE;
						}
					}
					if(state[RX].msgHdr != NULL) {
						state[RX].msg_state = SOS_MSG_RX_HDR;
						state[RX].crc = crcByte(0, byte_in);
						state[RX].flags |= UART_SOS_MSG_FLAG;
						state[RX].idx = 0;
						state[RX].state = UART_DATA;
						state[RX].hdlc_state = HDLC_DATA;
					} else {
						// need to generate no mem error
						uart_reset_recv();
					}
					break;

				case HDLC_RAW:
					if ((state[RX].buff = ker_malloc(UART_MAX_MSG_LEN, UART_PID)) != NULL) {
						state[RX].msg_state = SOS_MSG_RX_RAW;
						if (state[RX].flags & UART_CRC_FLAG) {
							state[RX].crc = crcByte(0, byte_in);
						}
						state[RX].state = UART_DATA;
						state[RX].hdlc_state = HDLC_DATA;
					} else {
						uart_reset_recv();
					}
					state[RX].idx = 0;
					break;

				default:
					uart_reset_recv();
					break;
				}
				break;

		case UART_DATA:
				if (err != 0) {
					uart_reset_recv();
					break;
				}

				// recieve an escape byte, wait for next byte
				if (byte_in  == HDLC_CTR_ESC) {
					saved_state = state[RX].hdlc_state;
					state[RX].hdlc_state = HDLC_ESCAPE;
					break;
				}

				if (byte_in == HDLC_FLAG) { // got an end of message symbol
					/*
					if (state[RX].msg_state == SOS_MSG_RX_RAW) {
						// end of raw recieve
						// should bundle and send off
						// trash for now
							 state[RX].hdlc_state = HDLC_IDLE;
							 state[RX].state = UART_IDLE;
							 state[RX].flags |= UART_DATA_RDY_FLAG;
							 uart_read_done(state[RX].idx, 0);
					} else {
						// got an end of message symbol early
						*/
						uart_reset_recv();
					//}
					break;
				}

				if (state[RX].hdlc_state == HDLC_ESCAPE) {
					byte_in ^= 0x20;
					state[RX].hdlc_state = saved_state;
				}

				switch (state[RX].msg_state) {
					case SOS_MSG_RX_HDR:
						if (byte_in == HDLC_FLAG) {  // got an end of message symbol
							uart_reset_recv();
							break;
						}
						uint8_t *tmpPtr = (uint8_t*)(state[RX].msgHdr);
						tmpPtr[state[RX].idx++] = byte_in;

						state[RX].crc = crcByte(state[RX].crc, byte_in);

						if (state[RX].idx == SOS_MSG_HEADER_SIZE) {
							// if (state[RX].msgLen != state[RX].msgHdr->len) ????????
							state[RX].msgLen = state[RX].msgHdr->len;

							if (state[RX].msgLen < UART_MAX_MSG_LEN) {
								if (state[RX].msgLen != 0) {
									state[RX].buff = (uint8_t*)ker_malloc(state[RX].msgLen, UART_PID);
									if (state[RX].buff != NULL) {
										state[RX].msgHdr->data = state[RX].buff;
										state[RX].msgHdr->flag = SOS_MSG_RELEASE;
										state[RX].msg_state = SOS_MSG_RX_DATA;
										state[RX].idx = 0;
									} else {
										uart_reset_recv();
									}
								} else { // 0 length packet go straight to crc
									state[RX].msgHdr->flag &= ~SOS_MSG_RELEASE;
									state[RX].msgHdr->data = NULL;
									state[RX].msg_state = SOS_MSG_RX_CRC_LOW;
								}
							} else { // invalid msg length
								uart_reset_recv();
							}
						}
						break;

					case SOS_MSG_RX_RAW:
					case SOS_MSG_RX_DATA:
						if (err != 0) {
							uart_reset_recv();
							return;
						}
						state[RX].buff[state[RX].idx++] = byte_in;
						if (state[RX].flags & UART_CRC_FLAG) {
							state[RX].crc = crcByte(state[RX].crc, byte_in);
						}
						if (state[RX].idx == state[RX].msgLen) {
							if (state[RX].flags & UART_SOS_MSG_FLAG) {
								state[RX].hdlc_state = HDLC_CRC;
								state[RX].msg_state = SOS_MSG_RX_CRC_LOW;
							} else {
								// rx buffer overflow
								uart_reset_recv();
							}
						}
						break;

					case SOS_MSG_RX_CRC_LOW:
						crc_in = byte_in;
						state[RX].msg_state = SOS_MSG_RX_CRC_HIGH;
						break;

					case SOS_MSG_RX_CRC_HIGH:
						crc_in |= ((uint16_t)(byte_in) << 8);
						state[RX].hdlc_state = HDLC_PADDING;
						state[RX].msg_state = SOS_MSG_RX_END;
						state[RX].state = UART_HDLC_STOP;
						break;

					case SOS_MSG_RX_END:  // should never get here
					default:
						uart_reset_recv();
						break;
				}
				break;

		case UART_HDLC_STOP:
				if (byte_in != HDLC_FLAG) {
					// silently drop until hdlc stop symbol
					break;
				} else { // sos msg rx done
					state[RX].hdlc_state = HDLC_IDLE;
					if(crc_in == state[RX].crc) {
#ifndef NO_SOS_UART_MGR
						set_uart_address(entohs(state[RX].msgHdr->saddr));
#endif
						handle_incoming_msg(state[RX].msgHdr, SOS_MSG_UART_IO);
						state[RX].msgHdr = NULL;
					} else {
						msg_dispose(state[RX].msgHdr);
						state[RX].msgHdr = NULL;
					}
					state[RX].state = UART_IDLE;
					state[RX].msg_state = SOS_MSG_NO_STATE;
					state[RX].hdlc_state = HDLC_IDLE;
					//uart_reset_recv();

					//state[RX].msg_state = SOS_MSG_NO_STATE;
					//state[RX].state = UART_HDLC_START;
				}
				break;
				// XXX fall through

		default:
				uart_reset_recv();
				break;
	} // state[RX].state
}
#else
uart_send_interrupt() {
	uart_disable();
}
uart_recv_interrupt() {
	uart_disable();
}
#endif // DISABLE_UART



