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


#ifndef _VHAL_H
#define _VHAL_H
#include <cc2420_hal.h>

//define the PID for VHAL, used by mem
#define VHALPID		99

//define the operatios on radio

//define start up functions
#define Radio_On()	TC_SET_VREG_EN
#define Radio_Off()	TC_CLR_VREG_EN
#define Radio_Reset()	( TC_CLR_RESET && TC_SET_RESET )

//define address functions
#define Radio_Set_Address(C)	TC_SET_IEEEADR(C)
#define Radio_Get_Address(C)	TC_GET_IEEEADR(C)

//define channle/frequency functions
#define Radio_Set_Channel(N)	TC_SET_CHANNEL(N)
#define Radio_Get_Channel(N)	TC_GET_CHANNEL(N)
#define Radio_Set_Frequency(N)	TC_SET_FREQUENCY(N)
#define Radio_Get_Frequency(N)	TC_GET_FREQUENCY(N)

//define power saving functions
#define Radio_Set_RF_Power(N)	TC_SET_RF_POWER(N)
#define Radio_Get_RF_Power(N)	TC_GET_RF_POWER(N)
#define Radio_Sleep()		TC_STROBE(CC2420_SXOSCOFF)
#define Radio_Idle()		TC_STROBE(CC2420_SRFOFF)
extern void Radio_Wakeup();

//define Preamble functions
#define Radio_Set_Preamble_Length(N)	TC_SET_PREAMBLE_LENGTH(N)
#define Radio_Get_Preamble_Length(N)	TC_GET_PREAMBLE_LENGTH(N)
#define Radio_Set_SYNC_Word(N)		TC_SET_SYNC_WORD(N)
#define Radio_Get_SYNC_Word(N)		TC_GET_SYNC_WORD(N)
extern int8_t Radio_Check_Preamble();

//define the general format for data
typedef struct _vhal_data {
	uint8_t pre_payload_len;
	uint8_t payload_len;
	uint8_t post_payload_len;
	uint8_t *pre_payload;
	uint8_t *payload;
	uint8_t *post_payload;
}vhal_data;
 
//define data transmission functions				
extern int8_t Radio_Check_CCA();
extern int8_t Radio_Check_SFD();
extern int8_t Radio_Send(uint8_t *, uint8_t);
extern int8_t Radio_Send_CCA(uint8_t *, uint8_t);
extern int8_t Radio_Recv(uint8_t *, uint8_t *);

extern int8_t Radio_Send_Pack(vhal_data, int16_t *);
extern int8_t Radio_Send_Pack_CCA(vhal_data, int16_t *);
extern int8_t Radio_Recv_Pack(vhal_data*);

//define Security functions
#define Radio_Set_ENC_Mode(N)
#define Radio_Get_ENC_Mode(N)
#define Radio_Set_ENC_KEY0(K)	
#define Radio_Get_ENC_KEY0(K)
#define Radio_Set_ENC_KEY1(K)
#define Radio_Get_ENC_KEY1(K)
#define Radio_Enable_ENC
#define Radio_Disable_ENC

//define initialize function
extern void Radio_Init();

//define the interrupt functions
#define Radio_Enable_Interrupt()	TC_ENABLE_INTERRUPT
#define Radio_Disable_Interrupt()	TC_DISABLE_INTERRUPT
#define Radio_SetPackRecvedCallBack	TC_SetFIFOPCallBack

//define some auto control functions
#define Radio_Enable_Address_Check() 	TC_ENABLE_ADDR_CHK
#define Radio_Disable_ADDR_CHK()	TC_DISABLE_ADDR_CHK
#define Radio_Enable_Auto_CRC()		TC_ENABLE_AUTO_CRC
#define Radio_Disable_Auto_CRC()	TC_DISABLE_AUTO_CRC

//define the showbyte function for debug 
extern void showbyte(int8_t bb);

#endif //_VHAL_H

