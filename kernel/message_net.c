/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
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
 * $Id: message_net.c,v 1.36 2006/12/17 00:51:01 simonhan Exp $ 
 */
/**
 * @brief Message handling routines for network
 * @author Simon Han (simonhan@cs.ucla.edu)
 * @brief General Message Dispatcher (Handles diverse links)
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#include <message_queue.h>
#include <monitor.h>
#include <sos_info.h>
#include <message_types.h>
#include <malloc.h>
#include <message.h>
#include <sos_sched.h>
#include <hardware.h>
#include <sos_logging.h>

#if defined (SOS_UART_CHANNEL)
#include <sos_uart.h>
#include <sos_uart_mgr.h>
#endif

#if defined (SOS_I2C_CHANNEL)
#include <sos_i2c.h>
#include <sos_i2c_mgr.h>
#endif

//-------------------------------------------------------------------------------
// STATIC FUNCTION DECLARATIONS
//-------------------------------------------------------------------------------
static int8_t sos_msg_dispatch(Message* m);
static inline void msg_change_endian(Message* e);
static void sos_msg_find_right_link(Message *m);

//-------------------------------------------------------------------------------
// FUNCTION IMPLEMENTATIONS
//-------------------------------------------------------------------------------
// Fix the endian-ness of the message header
static inline void msg_change_endian(Message* e){
  e->daddr = ehtons(e->daddr);
  e->saddr = ehtons(e->saddr);
}

// Create a deep copy of the message

#if defined(SOS_UART_CHANNEL) || defined(SOS_I2C_CHANNEL) || defined(SOS_SPI_CHANNEL)
static Message* msg_duplicate(Message* m){
  Message* mcopy;
	uint8_t* d;
  mcopy = msg_create();
  if (NULL == mcopy) return NULL;
  d = (uint8_t*)ker_malloc(m->len, KER_SCHED_PID);
  if ((NULL == d) && (0 != m->len)){
    msg_dispose(mcopy);
    return NULL;
  }
	memcpy(mcopy, m, sizeof(Message));
	mcopy->data = d;
	mcopy->flag |= SOS_MSG_RELEASE;
  memcpy(mcopy->data, m->data, m->len);
  return mcopy;
}
#endif

// NULL Link - Simply free the message
static void null_link_msg_alloc(Message* m){
  msg_dispose(m);
}



//-------------------------------------------------------------------------------
// POST
//-------------------------------------------------------------------------------
// Copies message header and sends out the message
int8_t post(Message *e){
  Message *m = msg_create();
  if(m == NULL){ 
	if(flag_msg_release(e->flag)) {
	  ker_free(e->data);
	}
	return -ENOMEM;
  }
  // deep copy the header
  *m = *e;
  
  // Dispatch Message
  return sos_msg_dispatch(m);
}

//-------------------------------------------------------------------------------
// POST LINK
//-------------------------------------------------------------------------------
// Post a message over any link as specified by the flag
int8_t post_link(sos_pid_t did, sos_pid_t sid, 
		uint8_t type, uint8_t len, 
		void *data, uint16_t flag, 
		uint16_t daddr)
{
  // Create a message
  Message *m = msg_create();
  if (NULL == m){
    if (flag_msg_release(flag)){
      ker_free(data);
    }
    return -ENOMEM;
  }

  // Fill out message header
  m->daddr = daddr;
  m->did = did;
  m->type = type;
  m->saddr = node_address;
  m->sid = sid;
  m->len = len;
  m->data = (uint8_t*)data;
  m->flag = flag;

  // Dispatch Message
  return sos_msg_dispatch(m);
}


//-----------------------------------------------------------------------------
// SOS Find Right link
//-----------------------------------------------------------------------------
static void sos_msg_find_right_link(Message *m)
{
		bool link_found = false;
		
		// try to figure out the right link
#ifdef SOS_UART_CHANNEL
		if (check_uart_address(m->daddr) == SOS_OK) {
			m->flag |= SOS_MSG_UART_IO;	
			link_found = true;
		} else {
			m->flag &= ~SOS_MSG_UART_IO;	
		}
#else
		m->flag &= ~SOS_MSG_UART_IO;	
#endif
		
#ifdef SOS_I2C_CHANNEL
		if (check_i2c_address(m->daddr) == SOS_OK) {
			m->flag |= SOS_MSG_I2C_IO;	
			link_found = true;
		} else {
			m->flag &= ~SOS_MSG_I2C_IO;	
		}
#else
		m->flag &= ~SOS_MSG_I2C_IO;	
#endif

		if(link_found == false) {
			m->flag |= SOS_MSG_RADIO_IO;	
		} else {
			m->flag &= ~SOS_MSG_RADIO_IO;	
		}
}

//--------------------------------------------------------------------------------
// SOS Message Dispatcher
//--------------------------------------------------------------------------------
static int8_t sos_msg_dispatch(Message* m)
{
#if defined(SOS_RADIO_CHANNEL) || defined(SOS_UART_CHANNEL) || defined(SOS_I2C_CHANNEL) || defined(SOS_SPI_CHANNEL)
	Message* mcopy[NUM_IO_LINKS] = {NULL};
	uint8_t msg_count = 0;
#endif

  // Local Dispatch
  if (node_address == m->daddr){
    sched_msg_alloc(m);
		ker_log( SOS_LOG_POST_NET, m->sid, m->daddr );
    return SOS_OK;
  }

#if !defined(SOS_UART_CHANNEL) && !defined(SOS_I2C_CHANNEL) && !defined(SOS_RADIO_CHANNEL) && !defined(SOS_SPI_CHANNEL)
	null_link_msg_alloc(m);
	return -EINVAL;
#endif

	if (flag_msg_link_auto(m->flag) && m->daddr != BCAST_ADDRESS) {
		sos_msg_find_right_link(m);
	}

	if ((m->flag & SOS_MSG_ALL_LINK_IO) == 0) {
		null_link_msg_alloc(m);
		return SOS_OK;
	}

	// Pre-allocate the message copies to allow for
	// an atomic NOMEM failure
#ifdef SOS_RADIO_CHANNEL
	if (flag_msg_from_radio(m->flag)){
		mcopy[SOS_RADIO_LINK_ID] = m;
		msg_count ++;
	}
#endif
	
#ifdef SOS_UART_CHANNEL
	if( flag_msg_from_uart(m->flag) ) {
		if (msg_count == 0){
			mcopy[SOS_UART_LINK_ID] = m;
		} else {
			mcopy[SOS_UART_LINK_ID] = msg_duplicate(m);
			if (NULL == mcopy[SOS_UART_LINK_ID]) goto dispatch_cleanup;
			mcopy[SOS_UART_LINK_ID]->flag &= ~SOS_MSG_RELIABLE;
		}
		msg_count++;
	}
#endif

#ifdef SOS_I2C_CHANNEL
	if (flag_msg_from_i2c(m->flag)) {
		if (msg_count == 0){
			mcopy[SOS_I2C_LINK_ID] = m;
		} else {
			mcopy[SOS_I2C_LINK_ID] = msg_duplicate(m);
			if (NULL == mcopy[SOS_I2C_LINK_ID]) goto dispatch_cleanup;
			mcopy[SOS_I2C_LINK_ID]->flag &= ~SOS_MSG_RELIABLE;
		}
		msg_count++;
	}
#endif

#ifdef SOS_SPI_CHANNEL                                        
	if (flag_msg_from_spi(m->flag)) {      
		if (msg_count == 0){
			mcopy[SOS_SPI_LINK_ID] = m;
		} else {
			mcopy[SOS_SPI_LINK_ID] = msg_duplicate(m);
			if (NULL == mcopy[SOS_SPI_LINK_ID]) goto dispatch_cleanup;
			mcopy[SOS_SPI_LINK_ID]->flag &= ~SOS_MSG_RELIABLE;
		}
		msg_count++;
	}
#endif

	// Deliver to monitor only once
	monitor_deliver_outgoing_msg_to_monitor(m);

  // Radio Dispatch
#ifdef SOS_RADIO_CHANNEL
	if (NULL != mcopy[SOS_RADIO_LINK_ID]){
		msg_change_endian(mcopy[SOS_RADIO_LINK_ID]);
		SOS_RADIO_LINK_DISPATCH(mcopy[SOS_RADIO_LINK_ID]);
		ker_log( SOS_LOG_POST_NET, mcopy[SOS_RADIO_LINK_ID]->sid, mcopy[SOS_RADIO_LINK_ID]->daddr );
	}
#endif

  // UART Dispatch
#ifdef SOS_UART_CHANNEL
	if (NULL != mcopy[SOS_UART_LINK_ID]){
		msg_change_endian(mcopy[SOS_UART_LINK_ID]);
		SOS_UART_LINK_DISPATCH(mcopy[SOS_UART_LINK_ID]);
	}
#endif

  // I2C Dispatch 
#ifdef SOS_I2C_CHANNEL
	if (NULL != mcopy[SOS_I2C_LINK_ID]){
		msg_change_endian(mcopy[SOS_I2C_LINK_ID]);
		SOS_I2C_LINK_DISPATCH(mcopy[SOS_I2C_LINK_ID]);
	}
#endif

	// SPI Dispatch
#ifdef SOS_SPI_CHANNEL
	if (NULL != mcopy[SOS_SPI_LINK_ID]){
		msg_change_endian(mcopy[SOS_SPI_LINK_ID]);
		SOS_SPI_LINK_DISPATCH(mcopy[SOS_SPI_LINK_ID]);
	}
#endif

	return SOS_OK;

#if defined(SOS_UART_CHANNEL) || defined(SOS_I2C_CHANNEL) || defined(SOS_SPI_CHANNEL)
dispatch_cleanup:
	{
		uint8_t i;
		for (i = 0 ; i < NUM_IO_LINKS; i++){
			if (NULL != mcopy[i]){
				msg_dispose(mcopy[i]);
			}
		}
		DEBUG("----------------Cleaning up-----------\n");
	}
	return -ENOMEM;
#endif
}

//============================================================================
// sys APIs
//============================================================================

int8_t ker_sys_post_link(sos_pid_t dst_mod_id, uint8_t type,
		uint8_t size, void *data, uint16_t flag, uint16_t dst_node_addr)
{
	sos_pid_t my_id = ker_get_current_pid();
	
	if( post_link(dst_mod_id, my_id, type, size, data, flag, dst_node_addr) != SOS_OK) {
		return ker_mod_panic(my_id);
	}
	return SOS_OK;
}


