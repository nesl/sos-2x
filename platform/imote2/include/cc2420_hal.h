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
 * @brief    cc2420 header file for imote2
 * @author   Hubert Wu {hubertwu@cs.ucla.edu}
 * @author   Andrew Barton-Sweeney {abs@cs.yale.edu}
 * @author   Dimitrios Lymberopoulos {dimitrios.lymberopoulos@yale.edu}
 */


#ifndef _CC2420_HAL_H
#define _CC2420_HAL_H
#include <CC2420Const.h>
#include <sos_inttypes.h>
#include <bitsop.h>

// CC2420 RADIO #defines
#define CC_VREN_PIN (115)
#define CC_RSTN_PIN (22)
#define CC_FIFO_PIN (114)
#define RADIO_CCA_PIN (116)
#define CC_FIFOP_PIN (0)
#define CC_SFD_PIN (16)
#define CC_CSN_PIN (39)

#define SSP3_RXD (41)
#define SSP3_RXD_ALTFN (3)
#define SSP3_TXD (35)
#define SSP3_TXD_ALTFN (3)
#define SSP3_SFRM (39)
#define SSP3_SFRM_ALTFN (3)
#define SSP3_SCLK (34)
#define SSP3_SCLK_ALTFN (3)

#define SSP1_RXD (26)
#define SSP1_RXD_ALTFN (1 )
#define SSP1_TXD (25)
#define SSP1_TXD_ALTFN (2 )
#define SSP1_SCLK (23)
#define SSP1_SCLK_ALTFN (2 )
#define SSP1_SFRM (24)
#define SSP1_SFRM_ALTFN (2 )

SOS_ASSIGN_PIN(CC_VREN,A,CC_VREN_PIN);
SOS_ASSIGN_PIN(CC_RSTN,A,CC_RSTN_PIN);
SOS_ASSIGN_PIN(CC_FIFO,A,CC_FIFO_PIN);
SOS_ASSIGN_PIN(RADIO_CCA,A,RADIO_CCA_PIN);
SOS_ASSIGN_PIN(CC_FIFOP,A,CC_FIFOP_PIN);
SOS_ASSIGN_PIN(CC_SFD,A,CC_SFD_PIN);
SOS_ASSIGN_PIN(CC_CSN,A,CC_CSN_PIN);
SOS_ASSIGN_PIN(CC_SO,A,SSP3_RXD);
SOS_ASSIGN_PIN(CC_SI,A,SSP3_TXD);
SOS_ASSIGN_PIN(CC_SCLK,A,SSP3_SCLK);

/**********************************************************************
 * define the getTime function for timestamps                         *
 **********************************************************************/
#define getTime()	ker_systime32()

/**********************************************************************
 * define the maximum size for a packet that is sent by cc2420        *
 **********************************************************************/
#define MAXBUFFSIZE	128

/**********************************************************************
 * define the maximum times that SFD will checked for detecting SFD   *
 * interrupt                                                          *
 **********************************************************************/
#define MAXCOUNTSFD	1000

/**********************************************************************
 * define the maximum times that CCA will be checked for detecting    *
 * CCA value. used for implement CSMA/CA                              *
 **********************************************************************/
#define MAXCOUNTCCA	1000

/**********************************************************************
 * define pin operations for cc2420 that connecting to imote2         *
 *--------------------------------------------------------------------*
 * pin map: PXA271 <---------> cc2420                                 *
 *--------------------------------------------------------------------*
 *                A115 <---------> VREG_EN                            *
 *                A22  <---------> RESET                              *
 *--------------------------------------------------------------------*
 *                A35  <---------> SI                                 *
 *                A41  <---------> SO                                 *
 *                A34  <---------> SCLK                               *
 *                A39  <---------> CSn                                *
 *--------------------------------------------------------------------*
 *                A0   <---------> FIFOP                              *
 *                A114 <---------> FIFO                               *
 *                A116 <---------> CCA                                *
 *                A16  <---------> SFD                                *
 **********************************************************************/
extern void TC_INIT_PINS();

#define TC_SET_VREG_EN		SOS_SET_CC_VREN_PIN()
#define TC_CLR_VREG_EN		SOS_CLR_CC_VREN_PIN()

#define TC_SET_RESET		SOS_SET_CC_RSTN_PIN()
#define TC_CLR_RESET		SOS_CLR_CC_RSTN_PIN()

#define TC_SET_SI		SOS_SET_CC_SI_PIN()
#define TC_CLR_SI		SOS_CLR_CC_SI_PIN()
#define TC_SO_IS_SET		SOS_READ_CC_SO_PIN()
#define TC_SET_SCLK		SOS_SET_CC_SCLK_PIN()
#define TC_CLR_SCLK		SOS_CLR_CC_SCLK_PIN()

#define TC_SET_CSN		SOS_SET_CC_CSN_PIN()
#define TC_CLR_CSN		SOS_CLR_CC_CSN_PIN()

#define TC_FIFOP_IS_SET		SOS_READ_CC_FIFOP_PIN()
#define TC_FIFO_IS_SET		SOS_READ_CC_FIFO_PIN()
#define TC_CCA_IS_SET		SOS_READ_RADIO_CCA_PIN()
#define TC_SFD_IS_SET		SOS_READ_CC_SFD_PIN()


#define TC_SET_SPI_WRITE	//TC_INIT_PINS()
#define TC_SET_SPI_READ		//TC_INIT_PINS()

/**********************************************************************
 * define select and unselect operatios for radio                     *
 *   when select radio, external flash must be unselect because they  *
 *   use the same spi interface                                       *
 **********************************************************************/
#define TC_SELECT_RADIO		TC_CLR_CSN
#define TC_UNSELECT_RADIO	TC_SET_CSN

/**********************************************************************
 * define BYTE/WORD read/write operations  (continue)                 *
 *  therefore, I define functions for them to reduce the code image   *
 *  size.                                                             *
 **********************************************************************/
extern void TC_WRITE_BYTE(uint8_t);
extern void TC_WRITE_WORD(uint16_t);
extern void TC_READ_BYTE(uint8_t*);
extern void TC_READ_WORD(uint16_t*);

extern uint8_t _current_stat;

extern void TC_STROBE(uint8_t);

/**********************************************************************
 * define the status retrival operations                              *
 *  best run TC_STROBE(CC2420_SNOP) before get the status             *
 **********************************************************************/
#define TC_GET_STAT(I)	GETBIT(_current_stat,I)

#define TC_IS_XOSC16M_STABLE	TC_GET_STAT(CC2420_XOSC16M_STABLE)
#define TC_IS_TX_UNDERFLOW	TC_GET_STAT(CC2420_TX_UNDERFLOW)
#define TC_IS_ENC_BUSY		TC_GET_STAT(CC2420_ENC_BUSY)
#define TC_IS_TX_ACTIVE		TC_GET_STAT(CC2420_TX_ACTIVE)
#define TC_IS_LOCK   		TC_GET_STAT(CC2420_LOCK)
#define TC_IS_RSSI_VALID	TC_GET_STAT(CC2420_RSSI_VALID)

/**********************************************************************
 * define flushing RX/TX fifo operations                              *
 *  best run TC_STROBE(CC2420_SNOP) before get the status             *
 **********************************************************************/
#define TC_FLUSH_RX	\
	do {	TC_STROBE(CC2420_SFLUSHRX);	\
		TC_STROBE(CC2420_SFLUSHRX);	\
	} while(0);
#define TC_FLUSH_TX	TC_STROBE(CC2420_SFLUSHTX)

/**********************************************************************
 * define get/set register bits funtions                              *
 *  R: the register, I: start position, J, end position, C: value of  *
 *  the bits                                                          *
 **********************************************************************/
extern void TC_GET_REG(uint8_t R, int8_t I, int8_t J, uint16_t* C);
extern void TC_SET_REG(uint8_t R, int8_t I, int8_t J, uint16_t C);

/**********************************************************************
 * define get and set parameter register funtions                     *
 **********************************************************************/
#define TC_ENABLE_ADDR_CHK		TC_SET_REG(CC2420_MDMCTRL0,11,11,1)
#define TC_DISABLE_ADDR_CHK		TC_SET_REG(CC2420_MDMCTRL0,11,11,0)
#define TC_ENABLE_AUTO_CRC		TC_SET_REG(CC2420_MDMCTRL0,5,5,1)
#define TC_DISABLE_AUTO_CRC		TC_SET_REG(CC2420_MDMCTRL0,5,5,0)
#define TC_SET_CCA_HYST(N)		TC_SET_REG(CC2420_MDMCTRL0,10,8,N)
#define TC_GET_CCA_HYST(N)		TC_GET_REG(CC2420_MDMCTRL0,10,8,&N)
#define TC_SET_CCA_MODE(N)		TC_SET_REG(CC2420_MDMCTRL0,7,6,N)
#define TC_GET_CCA_MODE(N)		TC_GET_REG(CC2420_MDMCTRL0,7,6,&N)

#define TC_SET_PREAMBLE_LENGTH(N)	TC_SET_REG(CC2420_MDMCTRL0,3,0,N)
#define TC_GET_PREAMBLE_LENGTH(N)	TC_GET_REG(CC2420_MDMCTRL0,3,0,&N)
#define TC_SET_SYNC_WORD(N)		TC_SET_REG(CC2420_SYNCWORD,15,0,N)
#define TC_GET_SYNC_WORD(N)		TC_GET_REG(CC2420_SYNCWORD,15,0,&N)

#define TC_SET_LENGTH_THRESHOLD(C)	TC_SET_REG(CC2420_IOCFG0,6,0,C)

#define TC_SET_CHANNEL(N)	\
	do {	int16_t fs;	\
		fs = 357+5*(N-11); 	\
		TC_SET_REG(CC2420_FSCTRL,9,0,fs);	\
		if(TC_IS_XOSC16M_STABLE)	\
			TC_STROBE(CC2420_SRXON);	\
	} while(0);
#define TC_GET_CHANNEL(N)	\
	do {	int16_t fs;	\
		TC_GET_REG(CC2420_FSCTRL,9,0,fs);	\
		N = (fs-357)/5+11;	\
	} while(0);
#define TC_SET_FREQUENCY(N)	\
	do {	int16_t fs=N-2048;	\
		TC_SET_REG(CC2420_FSCTRL,9,0,fs);	\
		if(TC_IS_XOSC16M_STABLE)	\
			TC_STROBE(CC2420_SRXON);	\
	} while(0);
#define TC_GET_FREQUENCY(N)	\
	do {	int16_t fs;	\
		TC_GET_REG(CC2420_FSCTRL,9,0,fs);	\
		N = fs + 2048;	\
	} while(0);

#define TC_SET_RF_POWER(N)		TC_SET_REG(CC2420_FSCTRL,4,0,N)
#define TC_GET_RF_POWER(N)		TC_GET_REG(CC2420_FSCTRL,4,0,&N)

#define TC_SET_CCA_THRESHOLD(N)		TC_SET_REG(CC2420_RSSI,15,8,N)
#define TC_GET_CCA_THRESHOLD(N)		TC_GET_REG(CC2420_RSSI,15,8,&N)

#define TC_SET_RSSI_VAL(N)		TC_SET_REG(CC2420_RSSI,7,0,N)
#define TC_GET_RSSI_VAL(N)		TC_GET_REG(CC2420_RSSI,7,0,&N)

//define SEC related operations
#define TC_SELECT_SEC_SAKEY(N)	TC_SET_REG(CC2420_SECCTRL0,7,7,N)
#define TC_SELECT_SEC_TXKEY(N)	TC_SET_REG(CC2420_SECCTRL0,6,6,N)
#define TC_SELECT_SEC_RXKEY(N)	TC_GET_REG(CC2420_SECCTRL0,5,5,&N)
#define TC_SET_SEC_MODE(N)	TC_SET_REG(CC2420_SECCTRL0,1,0,N)
#define TC_GET_SEC_MODE(N)	TC_GET_REG(CC2420_SECCTRL0,1,0,&N)

#define TC_RX_DEC		TC_STROBE(CC2420_SRXDEC)
#define TC_TX_ENC		TC_STROBE(CC2420_STXENC)
#define TC_SAES			TC_STROBE(CC2420_SAES)

/**********************************************************************
 * define ram read/write funtions                                     *
 **********************************************************************/
// Dimitrios
// RAM ACCESS
// d: pointer to the data to be written
// a: address
// n: number of bytes to be written
#define CC2420_WRITE_RAM(d,a,n) \
	 do { \
	 	  unsigned int i=0;\
		  TC_SELECT_RADIO; \
		  TC_WRITE_BYTE(0x80 | (a & 0x7F)); \
		  TC_WRITE_BYTE((a >> 1) & 0xC0); \
		  for (i = 0; i < (n); i++) { \
				TC_WRITE_BYTE(((unsigned char*)(d))[i]); \
		  } \
		  TC_UNSELECT_RADIO; \
	 } while (0)


#define TC_READ_RAM_BYTES(A,C,N)	\
	do {	int i;	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
		TC_WRITE_BYTE( (A&0x7F) | 0x80 );	\
		TC_WRITE_BYTE( ( (A>>1) & 0xC0) | 0x20 );	\
		for(i=N-1;i>=0;i--)	\
			TC_READ_BYTE(C[i]);	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
	} while(0);
#define TC_WRITE_RAM_BYTES(A,C,N)	\
	do {	int i;	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
		TC_WRITE_BYTE( (A&0x7F) | 0x80 );	\
		TC_WRITE_BYTE( (A>>1) & 0xC0 );	\
		for(i=N-1;i>=0;i--)	\
			TC_WRITE_BYTE(C[i]);	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
	} while(0);
#define TC_READ_RAM_WORD(A,C)	\
	do {	int8_t ADDR;	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
		ADDR = (A&0x7F) | 0x80;	\
		TC_WRITE_BYTE( ADDR );	\
		ADDR = ( (A>>1) & 0xC0 ) | 0x20 ;	\
		TC_WRITE_BYTE( ADDR );	\
		TC_READ_WORD(C);	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
	} while(0);
#define TC_WRITE_RAM_WORD(A,C)	\
	do {	int8_t ADDR;	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
		ADDR = (A&0x7F) | 0x80;	\
		TC_WRITE_BYTE( ADDR );	\
		ADDR = (A>>1) & 0xC0;	\
		TC_WRITE_BYTE( ADDR );	\
		TC_WRITE_WORD(C);	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
	} while(0);
#define TC_READ_RAM_BYTE(A,C)	\
	do {	TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
		TC_WRITE_BYTE( (A&0x7F) | 0x80 );	\
		TC_WRITE_BYTE( ( (A>>1) & 0xC0) | 0x20 );	\
		TC_READ_BYTE(C);	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
	} while(0);
#define TC_WRITE_RAM_BYTE(A,C)	\
	do {	TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
		TC_WRITE_BYTE( (A&0x7F) | 0x80 );	\
		TC_WRITE_BYTE( (A>>1) & 0xC0 );	\
		TC_WRITE_BYTE(C);	\
		TC_UNSELECT_RADIO;	\
		TC_SELECT_RADIO;	\
	} while(0);

#define SWITCH_BYTES_ML(C,N)	\
	do {	int8_t temp;	\
		int8_t i;	\
		for(i=0;i<N/2;i++) {	\
			temp = C[i];	\
			C[i] = C[N-i-1];	\
			C[N-i-1] = temp;	\
		}	\
	} while(0);
#define SWITCH_WORD_ML(C)	( C = ( (C>>8) & 0x00ff ) | (C<<8) )

/**********************************************************************
 * define functions for config parameters in ram                      *
 **********************************************************************/
#define TC_SET_SHORTADR(C)	\
	do {	SWITCH_WORD_ML(C);	\
		TC_WRITE_RAM_WORD(CC2420_RAM_SHORTADR,C);	\
	} while(0);
#define TC_GET_SHORTADR(C)	TC_READ_RAM_WORD(CC2420_RAM_SHORTADR,C)

#define TC_SET_PANID(C)			\
	do {	SWITCH_WORD_ML(C);	\
		TC_WRITE_RAM_WORD(CC2420_RAM_PANID,C);	\
	} while(0);
#define TC_GET_PANID(C)		TC_READ_RAM_WORD(CC2420_RAM_PANID,C)

#define TC_SET_IEEEADR(C)	TC_WRITE_RAM_BYTES(CC2420_RAM_IEEEADR,C,8)
#define TC_GET_IEEEADR(C)	TC_READ_RAM_BYTES(CC2420_RAM_IEEEADR,C,8)

#define TC_SET_CBCSTATE(C)	TC_WRITE_RAM_BYTES(CC2420_RAM_CBCSTATE,C,16)
#define TC_GET_CBCSTATE(C)	TC_READ_RAM_BYTES(CC2420_RAM_CBCSTATE,C,16)

#define TC_SET_TXNONCE(C)	TC_WRITE_RAM_BYTES(CC2420_RAM_TXNONCE,C, 16)
#define TC_GET_TXNONCE(C)	TC_READ_RAM_BYTES(CC2420_RAM_TXNONCE,C, 16)

#define TC_SET_KEY1(C)		TC_WRITE_RAM_BYTES(CC2420_RAM_KEY1,C,16)
#define TC_GET_KEY1(C)		TC_READ_RAM_BYTES(CC2420_RAM_KEY1,C,16)

#define TC_SET_SABUF(C)		TC_WRITE_RAM_BYTES(CC2420_RAM_SABUF,C,16)
#define TC_GET_SABUF(C)		TC_READ_RAM_BYTES(CC2420_RAM_SABUF,C,16)

#define TC_SET_RXNONCE(C)	TC_WRITE_RAM_BYTES(CC2420_RAM_RXNONCE,C, 16)
#define TC_GET_RXNONCE(C)	TC_READ_RAM_BYTES(CC2420_RAM_RXNONCE,C, 16)

#define TC_SET_KEY0(C)		TC_WRITE_RAM_BYTES(CC2420_RAM_KEY0,C,16)
#define TC_GET_KEY0(C)		TC_READ_RAM_BYTES(CC2420_RAM_KEY0,C,16)

/**********************************************************************
 * define checking fifo overflow function                             *
 **********************************************************************/
#define TC_IS_RX_OVERFLOW	( !TC_FIFO_IS_SET && TC_FIFOP_IS_SET )

/**********************************************************************
 * define the callback functions for events                           *
 **********************************************************************/
extern void (*_FIFOP_IS_SET_CALL)(int16_t timestamp);
extern void (*_FIFO_IS_SET_CALL)(int16_t timestamp);
extern void (*_CCA_IS_SET_CALL)(int16_t timestamp);
extern void (*_SFD_IS_SET_CALL)(int16_t timestamp);
extern void (*_SFD_IS_CLR_CALL)(int16_t timestamp);

extern void TC_SetFIFOPCallBack(void (*f)(int16_t));
extern void TC_SetFIFOCallBack(void (*f)(int16_t));
extern void TC_SetCCACallBack(void (*f)(int16_t));
extern void TC_SetSFDCallBack(void (*f1)(int16_t), void (*f2)(int16_t));

/**********************************************************************
 * define the interruption function                                   *
 **********************************************************************/
extern void TC_Setup_Interrupt();
extern void TC_Enable_Interrupt();
extern void TC_Disable_Interrupt();
#define TC_ENABLE_INTERRUPT	TC_Enable_Interrupt()
#define TC_DISABLE_INTERRUPT	TC_Disable_Interrupt()

#if 0
extern ISR(INT6_vect);		//FIFOP
extern ISR(INT7_vect);		//FIFO
#endif

/**********************************************************************
 * check sfd's edge from low to high, from high to low                *
 **********************************************************************/
#define TC_CHECK_SFD_LOW_TO_HIGH	\
	do {	int count;	\
		int16_t timestamp;	\
		count=0;	\
		while( !TC_SFD_IS_SET && ++count<MAXCOUNTSFD );	\
		timestamp = getTime();	\
		if(TC_SFD_IS_SET) _SFD_IS_SET_CALL(timestamp);	\
	} while(0);

#define TC_CHECK_SFD_HIGH_TO_LOW	\
	do {	int count;	\
		int16_t timestamp;	\
		count=0;	\
		while( TC_SFD_IS_SET && ++count<MAXCOUNTSFD );	\
		timestamp = getTime();	\
		if(!TC_SFD_IS_SET) _SFD_IS_CLR_CALL(timestamp);	\
	} while(0);

/**********************************************************************
 * define the cc2420_hardware_init function                           *
 **********************************************************************/
extern void cc2420_hardware_init();

/**********************************************************************
 * define the TC_UWAIT function                                       *
 *            TC_MWAIT function                                       *
 **********************************************************************/
//extern void TC_UWAIT(uint16_t u);
#define TC_UWAIT(u)		TOSH_uwait(u)
extern void TC_MWAIT(uint16_t u);

/*****************************************************************
 * define these two functions for hardware.c                     *
 *****************************************************************/
extern void ker_radio_ack_enable();
extern void ker_radio_ack_disable();

/*****************************************************************
 * define endian switch function for host between net            *
 *****************************************************************/
extern uint16_t host_to_net(uint16_t);
extern uint16_t net_to_host(uint16_t);

#endif //_CC2420_HAL_H

