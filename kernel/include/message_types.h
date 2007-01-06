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
 */
/**
 * @brief Message related types and defines
 * @author Simon Han (simonhan@ee.ucla.edu)
 */
#ifndef _MESSAGE_TYPES_H
#define _MESSAGE_TYPES_H

#include <pid.h>
#include <stddef.h>
// for struct packing
#include <sos_info.h>

#define SOS_MSG_PAYLOAD_LENGTH 4
/**
 * @brief message 
 *
 * NOTE: *data will have to be before flag
 * the main reason is flag is not transmitted over network
 * if any additional header field is needed,
 * if this new field need to be over network, add before *data
 * otherwise, add after *data.
 * the fields below larg will not be transmitted over the network.
 */
typedef struct Message{
	sos_pid_t  did;                          //!< module destination id
	sos_pid_t  sid;                          //!< module source id
	uint16_t daddr;                          //!< node destination address
	uint16_t saddr;                          //!< node source address
	uint8_t  type;                           //!< module specific message type
	uint8_t  len;                            //!< payload length 
	uint8_t  *data;                          //!< actual payload
	uint16_t flag;                           //!< flag to indicate the status of message, see below
	uint8_t payload[SOS_MSG_PAYLOAD_LENGTH]; //!< statically allocated payload
	struct Message *next;                    //!< link list for the Message
} PACK_STRUCT  
Message;


typedef int8_t (*msg_handler_t)(void *state, Message *m);


#define SOS_MSG_HEADER_SIZE (offsetof(struct Message, data))
#define SOS_MSG_DID_OFFSET  (offsetof(struct Message, did))
#define SOS_MSG_TYPE_OFFSET (offsetof(struct Message, type))
#define SOS_MSG_LEN_OFFSET (offsetof(struct Message, len))
#define SOS_MSG_PRE_HEADER_SIZE 1 //! This pre-header currently sends the group ID (Transparent to apps)
#define SOS_MSG_CRC_SIZE (sizeof(uint16_t))

/**
 * states for a tx/rx protocol to step through
 * while working with a sos msg
 *
 * the uninitalized state of any messing system must be no_state!
 * 
 * raw states are for framed raw byte streams.  the behavior of the 
 * system should be equivalant to that of a rx/tx data state.
 * 
 * crc_only is for byte streams that are doing crc verification
 * in the case of a sos_msg a crc is required
 *
 * the start/end states are msg wait states to allow for 
 * lower level framing (i.e. HDLC start stop symbols)
 *
 * the sequence of states corospond to the parts of a sos_msg
 * [START]  lower layer framing
 * HDR      header
 * DATA     payload
 * CRC_LOW  low byte of crc
 * CRC_HIGH low byte of crc
 * [END]    lower layer framing
 * 
 */
enum {
	SOS_MSG_NO_STATE,  // no defined state
	SOS_MSG_WAIT,      // expecting sos msg but no action

	SOS_MSG_TX_RAW,    // tx raw data no msg header/crc
	SOS_MSG_RX_RAW,    // rx raw data no msg header/crc

	SOS_MSG_TX_CRC_ONLY,  // raw data no msg header but crc enabled
	SOS_MSG_RX_CRC_ONLY,  // raw data no msg header but crc enabled

	SOS_MSG_TX_START,  // tx modes for sos msg
	SOS_MSG_TX_HDR,
	SOS_MSG_TX_DATA,
	SOS_MSG_TX_CRC_LOW,
	SOS_MSG_TX_CRC_HIGH,
	SOS_MSG_TX_END,

	SOS_MSG_RX_START,  // rx modes for sos msg
	SOS_MSG_RX_HDR,
	SOS_MSG_RX_DATA,
	SOS_MSG_RX_CRC_LOW,
	SOS_MSG_RX_CRC_HIGH,
	SOS_MSG_RX_END,
};

/**
 * @brief data structure used for statically allocated payload
 * We provide a common case allocation
 */
typedef struct {
	uint8_t byte;         //!< one byte parameter
	uint16_t word;        //!< two bytes parameter
} PACK_STRUCT
MsgParam;

/**
 * @brief message flag field
 * 
 * The flags are used for memory mgmt. and time sync.
 */

enum {
  // Network IO Flags
  // These flags have to be sequential
  // Please update NUM_IO_LINKS
  SOS_MSG_FROM_NETWORK    = 0x0100,    //!< Message is coming in from the network
  SOS_MSG_RADIO_IO        = 0x0200,    //!< Message is Rx/Tx over radio
  SOS_MSG_I2C_IO          = 0x0400,    //!< Message ix Rx/Tx over I2C
  SOS_MSG_UART_IO         = 0x0800,    //!< Message is Rx/Tx over UART
  SOS_MSG_SPI_IO          = 0x1000,    //!< Message is Rx/Tx over SPI
  SOS_MSG_ALL_LINK_IO     = 0x1E00,    //!< Message is Rx/Tx over all IO links
  SOS_MSG_LINK_AUTO       = 0x2000,    //!< automatically select right link
  // Scheduler Priority Flags
  SOS_MSG_SYSTEM_PRIORITY = 0x0080,    //!< Highest priority message
  SOS_MSG_HIGH_PRIORITY   = 0x0040,    //!< High priority message
  // Memory Management Flags
  SOS_MSG_RELIABLE        = 0x0008,    //!< Indicate senddone should be sent, memory will be included as payload
  SOS_MSG_RELEASE         = 0x0004,    //!< Indicate larg is dynamically allocated 
  SOS_MSG_SEND_FAIL       = 0x0002,    //!< Message failed to send
  // MAC flags
  SOS_MSG_USE_UBMAC       = 0x0020,    //!< Send packet using UBMAC
};

// This has to be the first network interface
#define SOS_MSG_START_IO SOS_MSG_RADIO_IO
// This is the number of IO links supported by the kernel
#define NUM_IO_LINKS     4

// SOS Link Identifier
enum{
  SOS_RADIO_LINK_ID = 0,
  SOS_I2C_LINK_ID,
  SOS_UART_LINK_ID,
  SOS_SPI_LINK_ID,
};


/**
 * @brief flag helpers
 */
// Network IO Flag Helpers
#define flag_msg_from_network(fflag)    ((fflag) & SOS_MSG_FROM_NETWORK)
#define flag_msg_from_radio(fflag)      ((fflag) & SOS_MSG_RADIO_IO)
#define flag_msg_from_i2c(fflag)        ((fflag) & SOS_MSG_I2C_IO)
#define flag_msg_from_uart(fflag)       ((fflag) & SOS_MSG_UART_IO)
#define flag_msg_from_spi(fflag)        ((fflag) & SOS_MSG_SPI_IO)
#define flag_msg_link_auto(fflag)       ((fflag) & SOS_MSG_LINK_AUTO)
// Scheduler Priority Flag Helpers
#define flag_system(fflag)              ((fflag) & SOS_MSG_SYSTEM_PRIORITY)
#define flag_high_priority(fflag)       ((fflag) & SOS_MSG_HIGH_PRIORITY)
// Memory Management Flag Helpers
#define flag_msg_release(fflag)         ((fflag) & SOS_MSG_RELEASE)
#define flag_msg_reliable(fflag)        ((fflag) & SOS_MSG_RELIABLE)
#define flag_send_fail(fflag)           ((fflag) & SOS_MSG_SEND_FAIL)
#define flag_use_ubmac(fflag)           ((fflag) & SOS_MSG_USE_UBMAC)

/**
 * @brief message filter flags
 *
 * These are used in ker_msg_change_rules()
 *
 * NOTE that the bottom four flags are allocated for kernel itself
 */
typedef uint8_t sos_ker_flag_t;
enum {
	//! user request receiving promiscuous messages
	SOS_MSG_RULES_PROMISCUOUS     = 0x40,
	//! module state is statically allocated
	SOS_KER_STATIC_MODULE         = 0x02,
	//! kernel flag indicating memory failed
	SOS_KER_MEM_FAILED            = 0x01,
};

/**
 * @brief starting number for each kind of message in a module
 *
 * This is module specific message type
 * kernel should not use these numbers
 */
enum {
	KER_MSG_START         = 0,
};

/**
 * By default, all messages are assume to be successful.
 * Module only gets message (INT_ERROR) when error happens
 */
// ---------------------------------------------------------------------------------------------
//                                                msg discription          
enum {
	MSG_INIT              = (KER_MSG_START + 0),  //!< initialization       
	MSG_DEBUG             = (KER_MSG_START + 1),  //!< debug info request    
	MSG_TIMER_TIMEOUT     = (KER_MSG_START + 2),  //!< timeout timer id    
	MSG_PKT_SENDDONE      = (KER_MSG_START + 3),  //!< send done            
	MSG_DATA_READY        = (KER_MSG_START + 4),  //!< sensor data ready  
	MSG_TIMER3_TIMEOUT    = (KER_MSG_START + 5),  //!< Timer 3 timeout   
	MSG_FINAL             = (KER_MSG_START + 6),  //!< process kill      
	MSG_FROM_USER         = (KER_MSG_START + 7),  //!< user input (gw only)
	MSG_GET_DATA          = (KER_MSG_START + 8),  //!< sensor get data    
	MSG_SEND_PACKET       = (KER_MSG_START + 9),  //!< send packet message (Implemented by routing protocols)
	MSG_DFUNC_REMOVED     = (KER_MSG_START + 10), //!< message to tell module that dynamic functions are removed, function entry index is included
	MSG_FUNC_USER_REMOVED = (KER_MSG_START + 11), //!< message to tell module that function user is removed, module id is included
	MSG_FETCHER_DONE      = (KER_MSG_START + 12), //!< module fetch is completed
	MSG_MODULE_OP         = (KER_MSG_START + 13), //!< module operation, see sos_module_types.h for the message format
	MSG_CAL_DATA_READY    = (KER_MSG_START + 14), //!< Calibrated Data Ready
	MSG_ERROR             = (KER_MSG_START + 15), //!< Error message contains <Mod Id, SOS Error No.>
	MSG_TIMESTAMP         = (KER_MSG_START + 16), //!< timestamped packet (used only by post_net)
	MSG_DISCOVERY         = (KER_MSG_START + 17), //!< discovery anouncement for new device detection on a link
	MSG_COMM_TEST         = (KER_MSG_START + 21), //!< test packet for developing comm layers 0x15 = 00010101 aiding scope debugging
	MSG_KER_UNKNOWN       = (KER_MSG_START + 31), //!< undefined or unknown message type
	//! MAXIMUM is 31 for now
	MOD_MSG_START		  = (KER_MSG_START + 32), //!< Type for Reply message and p2p message
};
//! PLEASE add name string to kernel/message.c

/**
 * @brief application message type definition
 */
enum {
	PROC_MSG_START		= 0x40,
	PLAT_MSG_START		= 0x80,
	MOD_CMD_START       = 0xc0,    //!< Type for Command message
};

#ifdef PC_PLATFORM
extern char ker_msg_name[][256];
#endif


#endif
