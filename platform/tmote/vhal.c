/* "Copyright (c) 2000-2003 The Regents of the University  of California.  
* All rights reserved.
*
* Permission to use, copy, modify, and distribute this software and its
* documentation for any purpose, without fee, and without written agreement is
* hereby granted, provided that the above copyright notice, the following
* two paragraphs and the author appear in all copies of this software.
* 
* IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
* DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
* OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
* CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
* ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
* PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
*
* Copyright (c) 2002-2003 Intel Corporation
* All rights reserved.
*
* This file is distributed under the terms in the attached INTEL-LICENSE     
* file. If you do not find these files, copies can be found by writing to
* Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA, 
* 94704.  Attention:  Intel License Inquiry.
*
*/

/**
 * @file vhal.c
 * @brief virtual radio Hardware Abstraction Layer
 * @author hubert wu {hubertwu@cs.ucla.edu}
 * @author Dimitrios Lymberopoulos {dimitrios.lymberopoulos@yale.edu}
 */
//--------------------------------------------------------
// INCLUDES
//--------------------------------------------------------
#include <hardware.h>
#include <spi_hal.h>
#include <vhal.h>
//#define LED_DEBUG
#include <led_dbg.h>

// Implemented in vmac.c
void _MacRecvAck(uint8_t ack_seq);
//keep the sequence number of the last received ACK
//static uint8_t received_ack_seq = 0;

void Radio_Init()
{	
	HAS_CRITICAL_SECTION;
	
	uint16_t SRC_ADDRESS;
	uint16_t PANID;
	ENTER_CRITICAL_SECTION();

	TC_INIT_PINS();
	Radio_On();
	TC_UWAIT(50);
	Radio_Reset();
	Radio_Wakeup();

	TC_SET_CCA_MODE(1);
//	TC_SET_CCA_HYST(0);
//	TC_SET_LENGTH_THRESHOLD(64);
//	Radio_Disable_ADDR_CHK();
//	Radio_Disable_Auto_CRC();
	Radio_Enable_Address_Check();

        // Maximum Power Level
        // TC_SET_REG(CC2420_TXCTRL,15,8,0xA0); //MSB
        // TC_SET_REG(CC2420_TXCTRL,7,0,0xFF); //LSB

	// Enable Automatic Packet Acknowledgment
	TC_SET_REG(CC2420_MDMCTRL0,4,4,1);

	//Set parameters
	SRC_ADDRESS = NODE_ADDR;
	TC_SET_SHORTADR(&SRC_ADDRESS);

	PANID = VMAC_PANID;
	TC_SET_PANID(&PANID);

	LEAVE_CRITICAL_SECTION();
}

void Radio_Wakeup()
{	
	TC_STROBE(CC2420_SXOSCON);
	while(!TC_IS_XOSC16M_STABLE)
		TC_STROBE(CC2420_SNOP);

	TC_FLUSH_RX;
	TC_FLUSH_TX;
	TC_STROBE(CC2420_SRXON);
}

int8_t Radio_Check_CCA()	//normal it will return 1. because once TC_CCA_IS_SET can let it return 1
{
	int count;
	count=0;

	TC_STROBE(CC2420_SNOP);
	if(TC_IS_TX_ACTIVE) {	//it is busy on sending
		return 0;
	}
	
	TC_STROBE(CC2420_SRXON);
	while(!TC_IS_RSSI_VALID)
		TC_STROBE(CC2420_SNOP);
	
	while(count<MAXCOUNTCCA) {
		TC_STROBE(CC2420_SNOP);
		if(TC_CCA_IS_SET) {
			return 1;
		}
		count++;
	}
	return 0;
}

int8_t Radio_Check_SFD() {
	int count;
	int16_t timestamp;
	count=0;
	TC_STROBE(CC2420_SNOP);
	while( !TC_SFD_IS_SET && ++count<MAXCOUNTSFD )
		TC_STROBE(CC2420_SNOP);
	if(count<MAXCOUNTSFD) { 
		if(_SFD_IS_SET_CALL) {
			timestamp = getTime();
			_SFD_IS_SET_CALL(timestamp);
		}
		return 1;
	}
	return 0;
}

int8_t Radio_Send_CCA(uint8_t *bytes, uint8_t num)
{	
	int i;
	TC_SELECT_RADIO;
	TC_WRITE_BYTE(CC2420_TXFIFO);
	TC_WRITE_BYTE(num);
	for(i=0;i<num;i++)
		TC_WRITE_BYTE(bytes[i]);
	TC_UNSELECT_RADIO;
	TC_STROBE(CC2420_STXONCCA);
	if( TC_IS_TX_UNDERFLOW || !TC_IS_TX_ACTIVE  ) {
		TC_FLUSH_TX;
		return 0;
	}
	return 1;
}

int8_t Radio_Check_Preamble()
{	
	int count;
	count=0;
	
	//Check that channel has signal
	if( !TC_IS_RSSI_VALID ) {
		TC_STROBE(CC2420_SRXON);
		while(!TC_IS_RSSI_VALID)
			TC_STROBE(CC2420_SNOP);
	}	
	while(count<MAXCOUNTCCA) {
		if(!TC_CCA_IS_SET) {
			count = -1;
			break;
		}
		count++;
	}

	if( count == -1 ) {	//if yes, check SFD
//		if( Radio_Check_SFD() )	//? we don't need to check SFD, if !CCA, means has data
			return 1;
	}
	return 0;
}

void Radio_Send_Pack(vhal_data *vd, int16_t *timestamp)
{	
	int i;
	uint8_t num;

	num = vd->pre_payload_len + vd->payload_len + vd->post_payload_len;
	if( num > MAXBUFFSIZE ) {
		TC_FLUSH_TX;
		TC_STROBE(CC2420_SNOP);
		return;
	}

	// Flush the TXFIFO before you transmit.
	TC_FLUSH_TX;

	TC_SELECT_RADIO;
	TC_WRITE_BYTE(CC2420_TXFIFO);
	TC_WRITE_BYTE(num);

	// FCF, Sequence Number
	for(i=0;i<3;i++)
		TC_WRITE_BYTE(vd->pre_payload[i]);

	// Skip the forth byte to work around msp430 gcc

	// PANID, Destination Address, Source Address
	// Did, Sid, Message Type 
	for(i=4; i<13;i++)
		TC_WRITE_BYTE(vd->pre_payload[i]);

	// Actual message payload
	for(i=0;i<vd->payload_len;i++)
		TC_WRITE_BYTE(vd->payload[i]);

	TC_UNSELECT_RADIO;
	TC_STROBE(CC2420_STXONCCA);
	TC_STROBE(CC2420_SNOP);

		// ACK was received! Everything went fine...
	*timestamp = getTime();
	return;
}

int8_t Radio_Recv_Pack(vhal_data *vd)
{	
	uint8_t i;
	uint8_t num;
	uint8_t tmp_byte;
	uint16_t FCF;
	uint8_t CRC;
#ifndef MICAZ_PLATFORM
	uint8_t break_flag;
	
	break_flag=0;
#endif

	if( TC_IS_RX_OVERFLOW ) {
		TC_FLUSH_RX;
		TC_STROBE(CC2420_SNOP);
		return 0;
	}

	TC_SELECT_RADIO;
	TC_WRITE_BYTE( ( 0x40 | CC2420_RXFIFO ) );
	TC_READ_BYTE(&num);
	num = num & 0x7F; // The MSB of the length byte is not used!!!


	// Frame Control Field
	TC_READ_BYTE(&(vd->pre_payload[0]));
	TC_READ_BYTE(&(vd->pre_payload[1]));
	FCF = vd->pre_payload[0];
	FCF |= ((uint16_t) (vd->pre_payload[1] << 8) );

	// Sequence number
	TC_READ_BYTE(&(vd->pre_payload[2]));

	//error check
	if(num < BASIC_RF_ACK_PACKET_SIZE)
	{
		// ignore everything
		TC_UNSELECT_RADIO;
		TC_FLUSH_RX;
		TC_STROBE(CC2420_SNOP);
		return 0;
	} else if ((num == BASIC_RF_ACK_PACKET_SIZE) && (FCF == BASIC_RF_ACK_FCF))
	{
		// An ACK packet has been received!
		// Footer
		TC_READ_BYTE(&tmp_byte);
		TC_READ_BYTE(&CRC);
		if(((FCF & (BASIC_RF_FCF_BM)) && (CRC & BASIC_RF_CRC_OK_BM)))
		{
			_MacRecvAck(vd->pre_payload[2]);
			//received_ack_seq = vd->pre_payload[2]; // notify the Transmitter that the ACK was received!
		}
		// Even if the CRC is correct do not pass the ACKs directly to the application layer
		TC_UNSELECT_RADIO;
		TC_FLUSH_RX;
		return 0;
	}


	// A regular data packet has been received!
	vd->payload_len = num - vd->pre_payload_len - vd->post_payload_len;
	if( num>MAXBUFFSIZE || vd->pre_payload_len>=num || vd->payload_len>=num || vd->post_payload_len>=num) {
		TC_UNSELECT_RADIO;
		TC_FLUSH_RX;
		TC_STROBE(CC2420_SNOP);
		return 0;
	}

	if( vd->payload_len > 0 ) {
		vd->payload = (uint8_t*)ker_malloc(vd->payload_len, VHALPID);
		if( vd->payload == NULL) {
			TC_UNSELECT_RADIO;
			TC_FLUSH_RX;
			TC_STROBE(CC2420_SNOP);
			return 0;
		}
	}

	//for(i=3;i<vd->pre_payload_len;i++){
	// Simon: skip the forth byte when filling the buffer to avoid mspgcc problem
	for(i=4;i<(vd->pre_payload_len+1);i++){
#ifndef MICAZ_PLATFORM
		if(!TC_FIFO_IS_SET)
		{
			break_flag=1;
			break;
		}
#endif
		TC_READ_BYTE(&(vd->pre_payload[i]));
	}
	for(i=0;i<vd->payload_len;i++){
#ifndef MICAZ_PLATFORM
		if(!TC_FIFO_IS_SET)
		{
			break_flag=1;
			break;
		}
#endif
		TC_READ_BYTE(&(vd->payload[i]));
	}
	for(i=0;i<vd->post_payload_len;i++){
#ifndef MICAZ_PLATFORM
		if(!TC_FIFO_IS_SET)
		{
			break_flag=1;
			break;
		}
#endif
		TC_READ_BYTE(&(vd->post_payload[i]));
	}
	TC_UNSELECT_RADIO;
	TC_FLUSH_RX;
	TC_STROBE(CC2420_SNOP);

#ifndef MICAZ_PLATFORM
	if(break_flag==1) {
		ker_free(vd->payload);
		return 0;
	}
#endif

	// vd->postpayload[i-1] corresponds to the CRC
	CRC = (uint8_t) vd->post_payload[i-1];
	if(((FCF & (BASIC_RF_FCF_BM)) && (CRC & BASIC_RF_CRC_OK_BM)))
	{
		// CRC is correct!
		return 1;
	}
	else
	{
		// CRC is wrong...
		ker_free(vd->payload);
		return 0;
	}
}

