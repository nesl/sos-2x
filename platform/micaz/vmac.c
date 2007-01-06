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
 * @brief    virtual hal for radio 
 * @author   Hubert Wu {huberwu@cs.ucla.edu}
 */

#include <random.h>
#include <hardware.h>
#include <sos_types.h>
#include <sos_module_types.h>
#include <sos_timer.h>
#include <malloc.h>
#include <net_stack.h>
#include <timestamp.h>
#include <systime.h>
#include <sos_info.h>
#include <message.h>
#include <string.h> // for memcpy
#include <spi_hal.h>
#include <vmac.h>
//#define LED_DEBUG
#include <led_dbg.h>
#include <sys_module.h>
#include <module.h>

/*************************************************************************
 * define the maximum number retry times for sending a packet            *
 *************************************************************************/
#define MAX_RETRIES		7

/*************************************************************************
 * define the maximum number of messages in the MAC queue                *
 *************************************************************************/
#define MAX_MSGS_IN_QUEUE	30

/*************************************************************************
 * define the timer ID                                                   *
 *************************************************************************/
#define WAKEUP_TIMER_TID    0 //!< Wakeup Timer TID

/*************************************************************************
 * declare the timer                                                     *
 *************************************************************************/
static sos_timer_t wakeup_timer;// = TIMER_INITIALIZER(RADIO_PID, WAKEUP_TIMER_TID, TIMER_ONE_SHOT);

/*************************************************************************
 * declare the message handler                                           *
 *************************************************************************/
static int8_t vmac_handler(void *state, Message *e);

/*************************************************************************
 * declare backoff mechanism functions                                   *
 *************************************************************************/
static void backoff_timeout(); 
static int16_t MacBackoff_congestionBackoff(int8_t retries);

/*************************************************************************
 * define the MAC module header                                          *
 *************************************************************************/
static mod_header_t mod_header SOS_MODULE_HEADER ={    
mod_id : RADIO_PID,    
state_size : 0,    
num_timers : 0,
num_sub_func : 0,   
num_prov_func : 0,   
module_handler: vmac_handler,    
};

/*************************************************************************
 * declare the MAC module                                                *
 *************************************************************************/
static sos_module_t vmac_module;

/*************************************************************************
 * declare the queue used for MAC                                        *
 *************************************************************************/
static mq_t vmac_pq;

/*************************************************************************
 * declare the sequence number variable                                  *
 *************************************************************************/
static uint16_t seq_count;

/*************************************************************************
 * define the retry times variable                                       *
 *************************************************************************/
static uint8_t retry_count;


/*************************************************************************
 * get sequence number                                                   *
 *************************************************************************/
static uint16_t getSeq() 
{
	uint16_t ret;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = seq_count;
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * increase sequence number                                              *
 *************************************************************************/
static uint16_t incSeq() 
{
	uint16_t ret;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = seq_count++;
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * set sequence number to 0                                              *
 *************************************************************************/
static void resetSeq() 
{
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	seq_count = 0;
	LEAVE_CRITICAL_SECTION();
}

/*************************************************************************
 * get retry times                                                       *
 *************************************************************************/
static uint8_t getRetries() 
{
	uint8_t ret;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = retry_count;
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * increase retry times                                                  *
 *************************************************************************/
static uint8_t incRetries() 
{
	uint8_t ret;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = retry_count++;
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * set retry times to 0                                                  *
 *************************************************************************/
static void resetRetries() 
{
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	retry_count = 0;
	LEAVE_CRITICAL_SECTION();
}

/*************************************************************************
 * message dispatch function for backoff mechanism for Collision         *
 *   Avoidance implementation                                            *
 *************************************************************************/
static int8_t vmac_handler(void *state, Message *e)
{
//   MsgParam *p = (MsgParam*)(e->data);
   switch(e->type){
       case MSG_TIMER_TIMEOUT:
	 backoff_timeout();
	 break;
       default:
	break;
   }
   return SOS_OK;
}

/*************************************************************************
 * define the callback function for receiving data                       *
 *************************************************************************/
void (*_mac_recv_callback)(Message *m) = 0;
void mac_set_recv_callback(void (*func)(Message *m))
{
	_mac_recv_callback = func;
}

/*************************************************************************
 * change packet's format from SOS message to MAC                        *
 *************************************************************************/
void sosmsg_to_mac(Message *msg, VMAC_PPDU *ppdu)
{
	ppdu->len = msg->len + PRE_PAYLOAD_LEN + POST_PAYLOAD_LEN;

	ppdu->mpdu.daddr = host_to_net(msg->daddr);
	ppdu->mpdu.saddr = host_to_net(msg->saddr);
	ppdu->mpdu.did = msg->did;
	ppdu->mpdu.sid = msg->sid;
	ppdu->mpdu.type = msg->type;
	ppdu->mpdu.group = node_group_id;
	
	ppdu->mpdu.data = msg->data;
}

/*************************************************************************
 * change packet's format from MAC to SOS message                        *
 *************************************************************************/
void mac_to_sosmsg(VMAC_PPDU *ppdu, Message *msg)
{
	msg->len = ppdu->len - (PRE_PAYLOAD_LEN + POST_PAYLOAD_LEN);

	msg->daddr = net_to_host(ppdu->mpdu.daddr);
	msg->saddr = net_to_host(ppdu->mpdu.saddr);
	msg->did = ppdu->mpdu.did;
	msg->sid = ppdu->mpdu.sid;
	msg->type = ppdu->mpdu.type;
	
	msg->data = ppdu->mpdu.data;
	msg->flag |= SOS_MSG_RELEASE;
}

/*************************************************************************
 * change packet's format from MAC to vhal                               *
 *************************************************************************/
void mac_to_vhal(VMAC_PPDU *ppdu, vhal_data *vd)
{
	vd->pre_payload_len = PRE_PAYLOAD_LEN;
	vd->pre_payload = (uint8_t*)&ppdu->mpdu.fcf;

	vd->post_payload_len = POST_PAYLOAD_LEN;
	vd->post_payload = (uint8_t*)&ppdu->mpdu.fcs;

	vd->payload_len = ppdu->len - vd->pre_payload_len - vd->post_payload_len;
	vd->payload = ppdu->mpdu.data;
}

/*************************************************************************
 * change packet's format from vhal to MAC                               *
 *************************************************************************/
void vhal_to_mac(vhal_data *vd, VMAC_PPDU *ppdu)
{
	ppdu->len = vd->payload_len + vd->pre_payload_len + vd->post_payload_len;
	memcpy((uint8_t*)&ppdu->mpdu.fcf, vd->pre_payload, vd->pre_payload_len);
	ppdu->mpdu.data = vd->payload;
	memcpy((uint8_t*)&ppdu->mpdu.fcs, vd->post_payload, vd->post_payload_len);
}

/*************************************************************************
 * send the message by radio                                             *
 *************************************************************************/
static int8_t radio_msg_send(Message *msg)
{
	int16_t timestamp;
	//construct the packet
	VMAC_PPDU ppdu;
	vhal_data vd;
	
	sosmsg_to_mac(msg, &ppdu);

	ppdu.mpdu.fcf = 1;		//doesn't matter, hardware supports it
	ppdu.mpdu.seq = getSeq();	//count by software
	ppdu.mpdu.fcs = 1;		//doesn't matter, hardware supports it

	mac_to_vhal(&ppdu, &vd);
	timestamp_outgoing(msg, ker_systime32());
	return Radio_Send_Pack(vd, &timestamp);
}

/*************************************************************************
 * get message number in the queue                                       *
 *************************************************************************/
static uint8_t getMsgNumOfQueue() 
{
	uint8_t ret = 0;
	HAS_CRITICAL_SECTION;
	ENTER_CRITICAL_SECTION();
	ret = vmac_pq.msg_cnt;
	LEAVE_CRITICAL_SECTION();
	return ret;
}

/*************************************************************************
 * check whether the queue is empty                                      *
 *************************************************************************/
static int8_t isQueueEmpty() 
{
	return ( getMsgNumOfQueue()==0 );
}

/*************************************************************************
 * will be called by post_net, etc functions to send message             *
 *************************************************************************/
void radio_msg_alloc(Message *msg)
{
	uint16_t sleeptime = 0;
	incSeq();

	if( Radio_Check_CCA() ) {
		radio_msg_send(msg);
		msg_send_senddone(msg, 1, RADIO_PID);
	}
	else {
		if( getMsgNumOfQueue() < MAX_MSGS_IN_QUEUE )		//queue is full?
			mq_enqueue(&vmac_pq, msg);
		else
			msg_send_senddone(msg, 0, RADIO_PID);		//release the memory for the msg
		sleeptime = MacBackoff_congestionBackoff(retry_count);
		ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, sleeptime);	// setup backoff timer		
	}
}

/*************************************************************************
 * implement backoff mechanism for Colliosn Avoidance                    *
 *************************************************************************/
void backoff_timeout() 
{
	if( isQueueEmpty() )
		return;

	Message *msg = NULL;
	if( Radio_Check_CCA() ) {
		msg = mq_dequeue(&vmac_pq);	// dequeue packet from mq
		if(msg) {
			radio_msg_send(msg);
			msg_send_senddone(msg, 1, RADIO_PID);
			resetRetries();				//set retry_count 0
		}	
	} else {
		if( getRetries()  < (uint8_t)MAX_RETRIES ) {
			incRetries();				//increase retry_count
		} else {
			msg = mq_dequeue(&vmac_pq);			// dequeue packet from mq
			if(msg) {
				msg_send_senddone(msg, 0, RADIO_PID);	//to release the memory for this msg
				resetRetries();				//set retry_count 0
			}
		}		
	}
	uint16_t sleeptime = MacBackoff_congestionBackoff(retry_count);
	ker_timer_restart(RADIO_PID, WAKEUP_TIMER_TID, sleeptime);	// setup new backoff timer
}

/*************************************************************************
 * the callback fnction for reading data from cc2420                     *
 *************************************************************************/
void _MacRecvCallBack(int16_t timestamp)
{
	VMAC_PPDU ppdu;
	vhal_data vd;

	mac_to_vhal(&ppdu, &vd);
	Radio_Disable_Interrupt();		//disable interrupt while reading data
	if( !Radio_Recv_Pack(&vd) ) {
		Radio_Enable_Interrupt();	//enable interrupt
		return;
	}
	vhal_to_mac(&vd, &ppdu);

	if( ppdu.mpdu.group     != node_group_id ) {	 
		ker_free(vd.payload);	 
		Radio_Enable_Interrupt();		//enable interrupt
		return;	 
	}	 
	{
		Message *msg = msg_create();
		if( msg == NULL ) {
			ker_free(vd.payload);
			Radio_Enable_Interrupt();		//enable interrupt
			return;
		}
		mac_to_sosmsg(&ppdu, msg);
		timestamp_incoming(msg, ker_systime32());
		handle_incoming_msg(msg, SOS_MSG_RADIO_IO);
	}
	Radio_Enable_Interrupt();		//enable interrupt
}

/*************************************************************************
 * set radio timestamp                                                   *
 *************************************************************************/
int8_t radio_set_timestamp(bool on)
{
	return SOS_OK;
}

/*************************************************************************
 * Implement exponential backoff mechanism                               *
 *************************************************************************/
static int16_t MacBackoff_congestionBackoff(int8_t retries)
{
	int8_t i;
	int16_t masktime = 1;
	for(i=0; i<retries; i++)
		masktime *= 2;			//masktime = 2^retries
	masktime --;				//get the mask
	if( masktime > 1023 )
		masktime = 1023;		//max backoff 1023 
//	return ((ker_rand() & 0xF) + TIMER_MIN_INTERVAL);
	return ((ker_rand() & masktime) + TIMER_MIN_INTERVAL);
}

/*************************************************************************
 * Initiate the radio and mac                                            *
 *************************************************************************/
void mac_init()
{
	Radio_Init();		
	Radio_Set_Channel(13);

	sched_register_kernel_module(&vmac_module, sos_get_header_address(mod_header), NULL);
	
	// Timer needs to be done after reigsteration
	ker_permanent_timer_init(&wakeup_timer, RADIO_PID, WAKEUP_TIMER_TID, TIMER_ONE_SHOT);
	
	mq_init(&vmac_pq);	//! Initialize sending queue
	resetSeq();		//set seq_count 0
	resetRetries(); 	//set retries 0

	//enable interrupt for receiving data
	Radio_Enable_Interrupt();
	Radio_SetPackRecvedCallBack(_MacRecvCallBack);
}
