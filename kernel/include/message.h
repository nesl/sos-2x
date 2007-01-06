/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: message.h,v 1.8 2006/08/04 02:05:14 ram Exp $
 */
/**
 * @brief    SOS message structure
 * @author   Simon Han (simonhan@cs.ucla.edu)
 *
 * Defines the message structure and its related functions
 * 
 * @note message parameters explanations.                               \n
 * e     :   message pointer.                                           \n
 * daddr :   node destination address.                                  \n
 * did   :   module destination id.				                        \n
 * type  :   module specific message type.                              \n
 * saddr :   node source address.                                       \n
 * sid   :   module source id.                                          \n
 * len   :   payload length                                             \n
 * data  :   message payload.                                           \n
 * flag  :   flag to indicate the status of message.                    \n
 * In all case, function returns 0 for successful, errno for error.     \n
 */

#ifndef _MESSAGE_H
#define _MESSAGE_H
#include <sos_types.h>
#include <message_types.h>


/**
 * @brief Post a message
 * @param e Message pointer
 * @return errno
 */
extern int8_t post(Message *e);

/**
 * @brief Post message over an IO Link
 * @param did    Destination Module ID
 * @param sid    Source Module ID
 * @param type   Message Type
 * @param len    Message Payload Length
 * @param data   Pointer to message payload
 * @param flag   Message Options
 * @param daddr  Destination Node Address
 * @return errno
 */
extern int8_t post_link(sos_pid_t did, 
						sos_pid_t sid, 
						uint8_t type, 
						uint8_t len,
						void* data, 
						uint16_t flag, 
						uint16_t daddr);

/**
 * @brief Post message over the right link select by the system
 * @param did    Destination Module ID
 * @param sid    Source Module ID
 * @param type   Message Type
 * @param len    Message Payload Length
 * @param data   Pointer to message payload
 * @param flag   Message Options
 * @param daddr  Destination Node Address
 * @return errno
 */
static inline int8_t post_auto(sos_pid_t did,
                               sos_pid_t sid,
							   uint8_t type,
							   uint8_t len,
							   void* data,
							   uint16_t flag,
							   uint16_t daddr) {
  return post_link(did, sid, type, len, data, flag | SOS_MSG_ALL_LINK_IO | SOS_MSG_LINK_AUTO, daddr);
}


/**
 * @brief Post message over Radio Link
 * @param did    Destination Module ID
 * @param sid    Source Module ID
 * @param type   Message Type
 * @param len    Message Payload Length
 * @param data   Pointer to message payload
 * @param flag   Message Options
 * @param daddr  Destination Node Address
 * @return errno
 */
static inline int8_t post_net(sos_pid_t did, 
		sos_pid_t sid, 
					   uint8_t type, 
					   uint8_t len, 
					   void *data, 
					   uint16_t flag, 
					   uint16_t daddr){
  return post_link(did, sid, type, len, data, flag|SOS_MSG_RADIO_IO, daddr);
}

/**
 * @brief Post message over UART Link
 * @param did    Destination Module ID
 * @param sid    Source Module ID
 * @param type   Message Type
 * @param len    Message Payload Length
 * @param data   Pointer to message payload
 * @param flag   Message Options
 * @param daddr  Destination Node Address
 * @return errno
 */
static inline int8_t post_uart(sos_pid_t did, 
					   sos_pid_t sid, 
					   uint8_t type, 
					   uint8_t length, 
					   void *data, 
					   uint16_t flag, 
					   uint16_t daddr){
  return post_link(did, sid, type, length, data, flag|SOS_MSG_UART_IO, daddr);
}

/**
 * @brief Post message over I2C Link
 * @param did    Destination Module ID
 * @param sid    Source Module ID
 * @param type   Message Type
 * @param len    Message Payload Length
 * @param data   Pointer to message payload
 * @param flag   Message Options
 * @param daddr  Destination Node Address
 * @return errno
 */
static inline int8_t post_i2c(sos_pid_t did, 
					   sos_pid_t sid, 
					   uint8_t type, 
					   uint8_t length, 
					   void *data, 
					   uint16_t flag, 
					   uint16_t daddr){
  return post_link(did, sid, type, length, data, flag|SOS_MSG_I2C_IO, daddr);
}

/**
 * @brief Post message over SPI Link
 * @param did    Destination Module ID
 * @param sid    Source Module ID
 * @param type   Message Type
 * @param len    Message Payload Length
 * @param data   Pointer to message payload
 * @param flag   Message Options
 * @param daddr  Destination Node Address
 * @return errno
 */
static inline int8_t post_spi(sos_pid_t did, 
					   sos_pid_t sid, 
					   uint8_t type, 
					   uint8_t length, 
					   void *data, 
					   uint16_t flag, 
					   uint16_t daddr){
  return post_link(did, sid, type, length, data, flag|SOS_MSG_SPI_IO, daddr);
}




/**
 * @brief post buffered short message
 * @param did    destination module id
 * @param sid    source module id
 * @param type   message type
 * @param byte   one byte data
 * @param word   two byte data
 * @param flag   message flag
 * @return errno
 *
 * this is useful for posting message upto two parameters
 */
extern int8_t post_short(
		sos_pid_t did, 
        sos_pid_t sid, 
        uint8_t type, 
        uint8_t byte, 
        uint16_t word, 
        uint16_t flag);

/**
 * @brief post buffered long message
 * @param did    destination module id
 * @param sid    source module id
 * @param type   message type
 * @param len    size of data
 * @param data   data in the message
 * @param flag   message options
 * @return errno
 *
 */
extern int8_t post_long(sos_pid_t did, 
						sos_pid_t sid, 
						uint8_t type, 
						uint8_t len, 
						void *data, 
						uint16_t flag);

/**
 * @brief post buffered longer message
 * @param did    destination module id
 * @param sid    source module id
 * @param type   message type
 * @param len    size of data
 * @param data   data in the message
 * @param flag   message options
 * @param saddr  source address (comes in handy for routing messages)
 * @return errno
 *
 */
extern int8_t post_longer(sos_pid_t did, 
						  sos_pid_t sid, 
						  uint8_t type, 
						  uint8_t len, 
						  void *data, 
						  uint16_t flag,
						  uint16_t saddr);

/**
 * @brief get the data from message
 * @param pid module pid
 * @param msg Message
 * @return the pointer to data, or NULL for failure.
 *
 * ker_msg_take_data tries to get the data from message
 * When the message is dynamically allocated, it will detach data from 
 * message.  Otherwise, it will attempt to ker_malloc a buffer and copy 
 * the data.  Therefore, when there is no memory, it is possible that this 
 * call may fail.
 *
 * ker_msg_take_data will check whether message type is MSG_PKT_SENDDONE.
 * If so, data will be taken from inner message.  When trying to take data 
 * from MSG_PKT_SENDDONE, use the pointer of outter message.
 * 
 */
extern uint8_t *ker_msg_take_data(sos_pid_t pid, Message *msg);

#endif

