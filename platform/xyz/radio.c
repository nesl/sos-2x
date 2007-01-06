/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
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
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
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

#include <message_queue.h>
#include <sos_types.h>
#include <sos.h>
#include <radio.h>
#include <net_stack.h>
#include "include/mac_headers.h"
#include "hardware.h"
#include <sos_uart.h>

//---------------------------------------------------------------
// Radio and protocol parameters
#define PANID 0x2420
#define COORD_SHORT 0x7A2B/*0x1975*/
#define CHANNEL 26

//---------------------------------------------------------------
// Addresses
volatile UHWORD myShortAddr;	// The short address given to
volatile UHWORD myBcastAddr = BCAST_ADDRESS;
QWORD coordExtAddr;				// The extended address of the coordinator
UHWORD panId;					// The PAN ID
UINT8 SOSPanChannel;			// Channel
UHWORD associatedAddress;		// The address given by the coordinator to the other device

//----------------------------------------------------------------
MAC_ENUM status;
UINT8 n;
ADDRESS coordAddr;
PAN_DESCRIPTOR *pPanDescriptor;
UBYTE setAttributeValue;

int enable_timestamp;

static volatile bool tx_busy;	//! Indicate radio is in transmission
static Message *tx_msg;			//! message in transmission
static mq_t pq;					//! message queue
static void radio_tx_msg_to_mac(Message *m);

/*
static int8_t mac802_15_4_handler(void *state, Message *msg){
	return SOS_OK;
}
*/

void disable_radio(void)
{
	DISABLE_SFD_CAPTURE_INT();
	DISABLE_FIFOP_INT();
	TIMER1_STOP();
	DISABLE_T1_COMPA_INT();
	DISABLE_T1_COMPC_INT();
	DISABLE_T1_CAPTURE_INT();
}

void enable_radio(void)
{
	ENABLE_SFD_CAPTURE_INT();
	ENABLE_FIFOP_INT();
	TIMER1_START();
	ENABLE_T1_COMPA_INT();
	ENABLE_T1_COMPC_INT();
	ENABLE_T1_CAPTURE_INT();
}

void xyz_zigbee_radio_init(void){
	// Take control of the ENABLE pin of the voltage regulator
	//! Set the direction for PIOE[9] - Output
	//put_hvalue(GPPME, get_hvalue(GPPME) | 0x0200);
	//put_hvalue(GPPOE, get_hvalue(GPPOE) & 0xFDFF);  	//!< VREG_EN = LOW!

	enable_timestamp = 0;

	put_hvalue(GPPMD, 0x00F9);  						//!< GPIO pins directions
	put_hvalue(GPCTL, get_hvalue(GPCTL) | 0x1800);		//! Fifop and Sfd interrupts enabled!
/*
	put_wvalue(CGBCNT0, 0x0000003C);					// unlock code
	put_wvalue(CGBCNT0, 0x00000000); 					// 58 MHz
*/
	MAC_INIT();
	mpmSetRequest(MPM_CC2420_ON);						// Power up
	while (mpmGetState() != MPM_CC2420_ON);

	// The address given to the associated device
	// XXX not used...
	associatedAddress = 0x1814;

	// Initialize variables
	//aExtendedAddress = (0x0000004722950000 + (unsigned long)(ker_id()));
	aExtendedAddress = ker_id();
	myShortAddr = ker_id();
	panId = ker_id();

	// Reset the MAC layer
	mlmeResetRequest(TRUE);

	// Modify necessary PIB attributes
	mlmeSetRequest(MAC_SHORT_ADDRESS, (UBYTE*) &myShortAddr);
	mlmeSetRequest(MAC_PAN_ID, &panId);
	coordAddr.Short=0xDEAD;

/* #ifdef RADIO_XMIT_POWER */
/* 	halTxSetPower(RADIO_XMIT_POWER); */
/* #else */
/* 	halTxSetPower(3); */
/* #endif */

#ifdef RADIO_CHANNEL
	SOSPanChannel = RADIO_CHANNEL;
#else
	SOSPanChannel = 25;
#endif

#ifdef SOS_NIC
	{
		BYTE temp;
		temp = TRUE;
		mlmeSetRequest(MAC_PROMISCUOUS_MODE, &temp);
	}
#endif

	mlmeStartRequest(panId, SOSPanChannel, 15, 15, TRUE, FALSE, FALSE, FALSE);
	// we are ready to send/receive after this point
	mrxIncrOnCounter();  // enable the receiver
	//! set transmission to be free
	tx_busy = false;
	//! initialize message queue
	mq_init(&pq);
	//! register radio module
	//ker_register_task(RADIO_PID, 0, mac802_15_4_handler);

	FASTSPI_SETREG(CC2420_TXCTRL,0xA0FF);	//! Set maximum transmit power-level
}

/**
 * @brief link layer ack
 */
void ker_radio_ack_enable()
{
	return;
}

void ker_radio_ack_disable()
{
	return;
}

int8_t radio_set_timestamp(bool on){
  	//bTsEnable = on;
  	enable_timestamp = on;
	return SOS_OK;
//	return -EPERM;
}

// Convert Buffer to Message
static Message* buff_to_msg(uint8_t* buff)
{
  	Message* m;
  	Message* tmpMsg;

  	if (buff[0] != node_group_id)	// Filtering based on group ID
    	return NULL;

  	m = msg_create();				// Create and copy the message header
  	if (m == NULL)
    	return NULL;
  	memcpy( m, buff + SOS_MSG_PRE_HEADER_SIZE, SOS_MSG_HEADER_SIZE);
  	tmpMsg = (Message*) (buff + SOS_MSG_PRE_HEADER_SIZE);

  	if (tmpMsg->len == 0)			// Check if the message has empty payload
    	return m;

  	m->data = ker_malloc(tmpMsg->len, RADIO_PID);	// Copy the message payload
  	if (m->data == NULL){
    	msg_dispose(m);
    	return NULL;
  	}
  	memcpy(m->data, buff + SOS_MSG_PRE_HEADER_SIZE + SOS_MSG_HEADER_SIZE, tmpMsg->len);
  	m->flag = SOS_MSG_RELEASE;
  	return m;
}

/**
 * Post the send packet
 */
void radio_msg_alloc(Message *m)
{
	HAS_CRITICAL_SECTION;
	if(flag_msg_release(m->flag)){
		ker_change_own(m->data, RADIO_PID);
	}
	ENTER_CRITICAL_SECTION();
	if(tx_busy) {
		mq_enqueue(&pq, m);
		LEAVE_CRITICAL_SECTION();
		return;
	}
	LEAVE_CRITICAL_SECTION();
	radio_tx_msg_to_mac(m);
}

static void radio_tx_msg_to_mac(Message *m)
{
	HAS_CRITICAL_SECTION;
	uint8_t buff[MAX_ZIGBEE_PAYLOAD];

	ENTER_CRITICAL_SECTION();
	tx_busy = true;
	tx_msg = m;
	LEAVE_CRITICAL_SECTION();

	buff[0] = node_group_id;
	memcpy(buff + SOS_MSG_PRE_HEADER_SIZE, m, SOS_MSG_HEADER_SIZE);
	if (m->len) {
		memcpy(buff + SOS_MSG_PRE_HEADER_SIZE + SOS_MSG_HEADER_SIZE, m->data, m->len);
	}

	//XXX I don't know why this has to be done...
	mrxIncrOnCounter();  // enable the receiver

	mcpsDataRequest(SRC_ADDR_SHORT | DEST_ADDR_SHORT,      //! Addressing Modes
		    panId,                              //! Source PAN ID
		    (ADDRESS*) &myShortAddr,               //! Source Address
			BCAST_ADDRESS,
			(ADDRESS*) &(myBcastAddr),
		    //m->daddr,                       //! Destination PAN ID
		    //(ADDRESS*) &(m->daddr),         //! Destination Address
		    SOS_MSG_PRE_HEADER_SIZE + SOS_MSG_HEADER_SIZE + m->len,   //! Length of MSDU
		    (BYTE*) buff,                   //! Pointer to MSDU
		    7,                              //! Handle for the Packet
		    TX_OPT_ACK_REQ);                //! Transmit Options

	//XXX I don't know why this has to be done either...
	mrxIncrOnCounter();  // enable the receiver
}


/*******************************************************************************************************
 *******************************************************************************************************
 **************************               MCPS Function Prototypes            **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
//  void mcpsDataConfirm(MAC_ENUM status, BYTE msduHandle)
//
//  DESCRIPTION:
//      MAC callback to the higher layer upon complete processing of a mcpsDataRequest
//      Function must be implemented by the higher layer
//
//  PARAMETERS:
//      MAC_ENUM status
//          (SUCCESS | TRANSACTION_OVERFLOW | TRANSACTION_EXPIRED | CHANNEL_ACCESS_FAILURE
//           INVALID_GTS | NO_ACK | UNAVAILABLE_KEY | FRAME_TOO_LONG | FAILED_SECURITY_CHECK)
//      BYTE msduHandle,
//          A handle to this packet from the mcpsDataRequest() function
//-------------------------------------------------------------------------------------------------------
void mcpsDataConfirm(MAC_ENUM status, UBYTE msduHandle){
	Message *m;
	bool succ = false;
	HAS_CRITICAL_SECTION;
	if(status == SUCCESS) {
		succ = true;
	} else if(status == NO_ACK && tx_msg->daddr == BCAST_ADDRESS) {
		succ = true;
	}
	/*
	else {
		succ = false;
	}
	*/

	msg_send_senddone(tx_msg, succ, RADIO_PID);
	m = mq_dequeue(&pq);
	if(m) {
		radio_tx_msg_to_mac(m);
	} else {
		ENTER_CRITICAL_SECTION();
		tx_busy = false;
		LEAVE_CRITICAL_SECTION();
	}
}

//-------------------------------------------------------------------------------------------------------
//  void mcpsDataIndication(MCPS_DATA_INDICATION *pMDI)
//
//  DESCRIPTION:
//      MAC callback to the higher layer upon reception of a MAC data frame
//      Multi-buffering is handled internally in the MAC sublayer
//      Function must be implemented by the higher layer
//
//  PARAMETERS:
//      MCPS_DATA_INDICATION *pMDI
//          Pointer to the MCPS_DATA_INDICATION data indication struct
//-------------------------------------------------------------------------------------------------------
void mcpsDataIndication(MCPS_DATA_INDICATION *pMDI){
	Message *RxMsg;

	RxMsg = buff_to_msg(pMDI->pMsdu);
	if (RxMsg != NULL) {
#ifdef SOS_UART_NIC
		uart_msg_alloc(RxMsg);
		led_green_toggle();
#else
		if (enable_timestamp)
			timestamp_incoming(RxMsg,pMDI->systime);
		handle_incoming_msg(RxMsg, SOS_MSG_RADIO_IO);
#endif
	}
}


//----------------------------------------------------------------------------------------------------------
// MLME prototypes

//-------------------------------------------------------------------------------------------------------
//  void mlmeAssociateIndication(ADDRESS deviceAddress, BYTE capabilityInformation, BOOL securityUse,...)
//
//  DESCRIPTION:
//      mlmeAssociateIndication is generated by the MAC layer upon reception of an associate request
//      command frame. For this demo application, all devices are allowed to associate, however only one
//      device at the time. The coordinator must be reset before a new device can associate.
//
//      The short address is assigned from the associatedAddress variable, which should have been
//      incremented if more devices could have joined.
//
//      Function must be implemented by the higher layer
//
//  PARAMETERS:
//      ADDRESS deviceAddress
//          The extended address of the device requesting association
//      BYTE capabilityInformation
//          The operational capabilities of the device requesting association
//              CI_ALTERNATE_PAN_COORD_BM   0x01
//              CI_DEVICE_TYPE_IS_FFD_BM    0x02
//              CI_POWER_SOURCE_BM          0x04
//              CI_RX_ON_WHEN_IDLE_BM       0x08
//              CI_SECURITY_CAPABILITY_BM   0x40
//              CI_ALLOCATE_ADDRESS_BM      0x80
//      BOOL securityUse
//          An indication of whether the received MAC command frame is using security. This value set to
//          TRUE if the security enable subfield was set to 1 or FALSE if the security enabled subfield
//          was set to 0
//      UINT8 aclEntry
//          The macSecurityMode parameter value from the ACL entry associated with the sender of the
//          data frame. This value is set to 0x08 if the sender of the data frame was not found in the
//          ACL.
//-------------------------------------------------------------------------------------------------------
void mlmeAssociateIndication(ADDRESS deviceAddress, UBYTE capabilityInformation, BOOL securityUse, UINT8 aclEntry){}


//-------------------------------------------------------------------------------------------------------
//  void mlmeAssociateConfirm(WORD assocShortAddress, MAC_ENUM status)
//
//  DESCRIPTION:
//      mlmeAssociateConfirm is generated by the MAC layer when an association attempt has succeeded or
//      failed (initiated by mlmeAssociateRequest(...)).
//      Function must be implemented by the higher layer
//
//  PARAMETERS:
//      WORD assocShortAddress
//          The short device address allocated by the coordinator on successful association. This
//          parameter will be equal to 0xFFFF if the association attempt was unsuccessful.
//      MAC_ENUM status
//          The status of the association attempt (SUCCESS, CHANNEL_ACCESS_FAILURE, NO_DATA, etc.)
//-------------------------------------------------------------------------------------------------------
void mlmeAssociateConfirm(UHWORD AssocShortAddress, MAC_ENUM status){}




//-------------------------------------------------------------------------------------------------------
//  void mlmeBeaconNotifyIndication(MLME_BEACON_NOTIFY_INDICATION *pMBNI)
//
//  DESCRIPTION:
//      MAC callback to the higher layer upon reception of a beacon frame with beacon payload
//      or when MAC_AUTO_REQUEST is set to FALSE.
//      Function must be implemented by the higher layer
//
//  PARAMETERS:
//      MLME_BEACON_NOTIFY_INDICATION *pMBNI
//          Pointer to the MLME_BEACON_NOTIFY_INDICATION beacon notification struct
//-------------------------------------------------------------------------------------------------------
void mlmeBeaconNotifyIndication(MLME_BEACON_NOTIFY_INDICATION *pMBNI){}




//-------------------------------------------------------------------------------------------------------
//  void mlmeCommStatusIndication(WORD panId, BYTE srcAddrMode, ADDRESS *pSrcAddr, BYTE dstAddrMode, ...
//
//  DESCRIPTION:
//      The mlmeCommStatusIndication callback is called by the MAC sublayer
//      either following a transmission instigated through a .response primitive or on receipt of a
//      frame that generates an error in its secure processing.
//
//      Function must be implemented by the higher layer
//
//  PARAMETERS:
//      WORD panId
//          The 16 bit PAN identifier of the device from which the frame was received or to
//          which the frame was being sent.
//      BYTE srcAddrMode
//          Source address mode
//      ADDRESS *pSrcAddr
//          Source address pointer
//      BYTE dstAddrMode
//          Destination address mode
//      ADDRESS *pDstAddr
//          Destination address pointer
//      MAC_ENUM status
//          Status enumeration
//          (SUCCESS | TRANSACTION_OVERFLOW | TRANSACTION_EXPIRED | CHANNEL_ACCESS_FAILURE | NO_ACK |
//          UNAVAILABLE_KEY | FRAME_TOO_LONG | FAILED_SECURITY_CHECK | INVALID_PARAMETER)
//-------------------------------------------------------------------------------------------------------
void mlmeCommStatusIndication(UHWORD panId, UBYTE srcAddrMode, ADDRESS *pSrcAddr, UBYTE dstAddrMode, ADDRESS *pDstAddr, MAC_ENUM status){}

//-------------------------------------------------------------------------------------------------------
//  void mlmeDisassociateIndication(QWORD deviceAddress, BYTE disassociateReason, BOOL securityUse, ...
//
//  DESCRIPTION:
//      Callback generated by the MAC sublayer to the higher layer upon reception of a
//      disassociation notification command frame
//      Function must be implemented by the higher layer of a FFD device
//
//  PARAMETERS:
//      QWORD deviceAddress
//          Extended address of the device requesting disassociation
//      BYTE disassociateReason
//          The disassociate reason (COORD_WISHES_DEVICE_TO_LEAVE | DEVICE_WISHES_TO_LEAVE)
//      BOOL securityUse
//          Security enabled for the incoming frame?
//      UINT8 aclEntry
//          The macSecurityMode parameter value from the ACL entry associated with the sender of
//          the data frame. This value is set to 0x08 if the sender of the data frame was not
//          found in the ACL.
//-------------------------------------------------------------------------------------------------------
void mlmeDisassociateIndication(QWORD deviceAddress, UBYTE disassociateReason, BOOL securityUse, UINT8 aclEntry){}




//-------------------------------------------------------------------------------------------------------
//  void mlmeDisassociateConfirm(MAC_ENUM status)
//
//  DESCRIPTION:
//      Callback generated by the MAC sublayer to the higher layer upon completion of a
//      mlmeDisassociateRequest(...) call from the higher layer.
//      Function must be implemented by the higher layer.
//
//  PARAMETERS:
//      MAC_ENUM status
//          Status returned by the callback
//          (SUCCESS | TRANSACTION_OVERFLOW | TRANSACTION_EXPIRED | NO_ACK |
//           CHANNEL_ACCESS_FAILURE | UNAVAILABLE_KEY | FAILED_SECURITY_CHECK |
//           INVALID_PARAMETER)
//-------------------------------------------------------------------------------------------------------
void mlmeDisassociateConfirm(MAC_ENUM status){}



//-------------------------------------------------------------------------------------------------------
//  void mlmeOrphanIndication(QWORD orphanAddress, BOOL securityUse, UINT8 aclEntry)
//
//  DESCRIPTION:
//      Callback generated by the MAC sublayer to the higher layer upon reception of a
//      orphan notification command frame
//      Function must be implemented by the higher layer of a FFD device
//
//  PARAMETERS:
//      QWORD orphanAddress
//          Extended address of the device notifying its orphan state
//      BOOL securityUse
//          Security enabled for the incoming frame?
//      UINT8 aclEntry
//          The macSecurityMode parameter value from the ACL entry associated with the sender of
//          the data frame. This value is set to 0x08 if the sender of the data frame was not
//          found in the ACL.
//-------------------------------------------------------------------------------------------------------
void mlmeOrphanIndication(QWORD orphanAddress, BOOL securityUse, UINT8 aclEntry){}

//-------------------------------------------------------------------------------------------------------
//  void mlmeDisassociateConfirm(MAC_ENUM status)
//
//  DESCRIPTION:
//      Callback generated by the MAC sublayer to the higher layer upon completion of a
//      mlmePollRequest(...) call from the higher layer.
//      Function must be implemented by the higher layer.
//
//  PARAMETERS:
//      MAC_ENUM status
//          Status returned by the callback
//          (SUCCESS | CHANNEL_ACCESS_FAILURE | NO_ACK | NO_DATA | UNAVAILABLE_KEY |
//           FAILED_SECURITY_CHECK | INVALID_PARAMETER)
//-------------------------------------------------------------------------------------------------------
void mlmePollConfirm(MAC_ENUM status){}



//-------------------------------------------------------------------------------------------------------
//  void mlmeRxEnableConfirm(MAC_ENUM status)
//
//  DESCRIPTION:
//      Callback generated by the MAC sublayer to the higher layer upon completion of a
//      mlmeRxEnableRequest(...) call from the higher layer.
//      Function must be implemented by the higher layer.
//
//  PARAMETERS:
//      MAC_ENUM status
//          Status returned by the callback
//          (SUCCESS | TX_ACTIVE | OUT_OF_CAP | INVALID_PARAMETER)
//-------------------------------------------------------------------------------------------------------
void mlmeRxEnableConfirm(MAC_ENUM status){}



//-------------------------------------------------------------------------------------------------------
//  void mlmeSyncLossIndication(MAC_ENUM lossReason)
//
//  DESCRIPTION:
//      Callback generated by the MAC sublayer to the higher layer indicating the loss of
//      synchronization with a coordinator, PAN Id conflicts or realignment.
//
//      Function must be implemented by the higher layer.
//
//  PARAMETERS:
//      MAC_ENUM status
//          Status generated by the callback
//          (PAN_ID_CONFLICT | REALIGNMENT | BEACON_LOST)
//-------------------------------------------------------------------------------------------------------
void mlmeSyncLossIndication(MAC_ENUM lossReason){}



//-------------------------------------------------------------------------------------------------------
//  void mpmSetRequest(BYTE mode)
//
//  DESCRIPTION:
//      Confirms that the CC2420 power mode change initiated by mpmSetRequest has become effective
//
//  ARGUMENTS:
//      BYTE status
//          OK_POWER_MODE_CHANGED:   The power mode was changed
//          OK_POWER_MODE_UNCHANGED: No change was required
//          ERR_RX_ON_WHEN_IDLE:     Could not proceed because "RX on when idle" was enabled
//-------------------------------------------------------------------------------------------------------
void mpmSetConfirm(UBYTE status){}

