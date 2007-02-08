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
 * @author   Hubert Wu {hubertwu@cs.ucla.edu}
 */

#ifndef _VMAC_H
#define _VMAC_H

#include <vhal.h>

/**********************************************************************
 * define the preamble length                                         *
 **********************************************************************/
#define PREAMBLELENGTH	4

/**********************************************************************
 * define the address length                                          *
 **********************************************************************/
#define ADDRESSLENGTH	2

/**********************************************************************
 * define SHR in zigbee format                                        *
 **********************************************************************/
typedef struct{
	uint8_t preamble[PREAMBLELENGTH];
	uint8_t sfd;
}VMAC_SHR;

/**********************************************************************
 * define MPDU in zigbee format                                        *
 **********************************************************************/
typedef struct _VMAC_MPDU {
	uint16_t fcf;
	uint16_t seq;
	uint16_t daddr;                          //!< node destination address
	uint16_t saddr;                          //!< node source address
	sos_pid_t  did;                          //!< module destination id
	sos_pid_t  sid;                          //!< module source id
	uint8_t  type;                           //!< module specific message type
	uint8_t  group;                          //!< SOS group info
	uint8_t *data;
	uint16_t fcs;
}VMAC_MPDU;

/**********************************************************************
 * define the pre-payload and post-payload length                     *
 **********************************************************************/
#define PRE_PAYLOAD_LEN 	12
#define POST_PAYLOAD_LEN	2

/**********************************************************************
 * define PPDU in zigbee format                                       *
 **********************************************************************/
typedef struct _VMAC_PPDU {
	VMAC_SHR shr;
	uint8_t len;
	VMAC_MPDU mpdu;
}VMAC_PPDU;

/**********************************************************************
 * Initiate the radio and mac                                         *
 **********************************************************************/
extern void mac_init();

/**********************************************************************
 * change packet's format among SOS message, MAC and VHAL             *
 **********************************************************************/
extern void sosmsg_to_mac(Message *msg, VMAC_PPDU *ppdu);
extern void mac_to_sosmsg(VMAC_PPDU *ppdu, Message *msg);
extern void mac_to_vhal(VMAC_PPDU *ppdu, vhal_data *vd);
extern void vhal_to_mac(vhal_data *vd, VMAC_PPDU *ppdu);

/*************************************************************************
 * will be called by post_net, etc functions to send message             *
 *************************************************************************/
extern void radio_msg_alloc(Message *msg);

/*************************************************************************
 * define the callback function for receiving data                       *
 *************************************************************************/
extern void (*_mac_recv_callback)(Message *m);
extern void mac_set_recv_callback(void (*func)(Message *m));

/*************************************************************************
 * set radio timestamp                                                   *
 *************************************************************************/
extern int8_t radio_set_timestamp(bool on);

#endif //_VMAC_H
