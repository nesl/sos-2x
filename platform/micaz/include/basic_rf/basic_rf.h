/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                               CHIPCON CC2420 BASIC RF LIBRARY                          *
 *      ***   + +   ***                             Library header file                                *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * The "Basic RF" library contains simple functions for packet transmission and reception with the     *
 * Chipcon CC2420 radio chip. The intention of this library is mainly to demonstrate how the CC2420 is *
 * operated, and not to provide a complete and fully-functional packet protocol. The protocol uses     *
 * 802.15.4 MAC compliant data and acknowledgment packets, however it contains only a small subset of  *
 * the 802.15.4 standard:                                                                              *
 *     - Association, scanning, beacons is not implemented                                             *
 *     - No defined coordinator/device roles (peer-to-peer, all nodes are equal)                       *
 *     - Waits for the channel to become ready, but does not check CCA twice (802.15.4 CSMA-CA)        *
 *     - Does not retransmit packets                                                                   *
 *     - Can not communicate with other networks (using a different PAN identifier)                    *
 *                                                                                                     *
 * INSTRUCTIONS:                                                                                       *
 * Startup:                                                                                            *
 *     1. Create a BASIC_RF_RX_INFO structure, and initialize the following members:                   *
 *         - rfRxInfo.pPayload (must point to an array of at least BASIC_RF_MAX_PAYLOAD_SIZE bytes)    *
 *     2. Call basicRfInit() to initialize the packet protocol.                                        *
 *                                                                                                     *
 * Transmission:                                                                                       *
 *     1. Create a BASIC_RF_TX_INFO structure, and initialize the following members:                   *
 *         - rfTxInfo.destAddr (the destination address, on the same PAN as you)                       *
 *         - rfTxInfo.pPayload (the payload data to be transmitted to the other node)                  *
 *         - rfTxInfo.length (the size od rfTxInfo.pPayload)                                           *
 *         - rfTxInfo.ackRequest (acknowledgment requested)                                            *
 *     2. Call basicRfSendPacket()                                                                     *
 *                                                                                                     *
 * Reception:                                                                                          *
 *     1. Call basicRfReceiveOn() to enable packet reception                                           *
 *     2. When a packet arrives, the FIFOP interrupt will run, and will in turn call                   *
 *        basicRfReceivePacket(), which must be defined by the application                             *
 *     3. Call basicRfReceiveOff() to disable packet reception                                         *
 *                                                                                                     *
 * FRAME FORMATS:                                                                                      *
 * Data packets:                                                                                       *
 *     [Preambles (4)][SFD (1)][Length (1)][Frame control field (2)][Sequence number (1)][PAN ID (2)]  *
 *     [Dest. address (2)][Source address (2)][Payload (Length - 2+1+2+2+2)][Frame check sequence (2)] *
 *                                                                                                     *
 * Acknowledgment packets:                                                                             *
 *     [Preambles (4)][SFD (1)][Length = 5 (1)][Frame control field (2)][Sequence number (1)]          *
 *     [Frame check sequence (2)]                                                                      *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any MCU with very few modifications required                    *
 *******************************************************************************************************
 * Revision history:                                                                                   *
 *  $Log: basic_rf.h,v $
 *  Revision 1.1.1.1  2005/06/23 05:11:39  simonhan
 *  initial import
 *
 *  Revision 1.1.1.1  2005/06/23 04:44:18  simonhan
 *  initial import
 *
 *  Revision 1.1.1.1  2005/06/23 04:11:49  simonhan
 *  initial import
 *
 *  Revision 1.1  2005/04/25 08:02:27  simonhan
 *  check in micaz directory
 *
 *  Revision 1.2  2005/03/29 00:54:58  simonhan
 *  remove SOS_DEV_SECTION
 *
 *  Revision 1.1.1.1  2005/01/14 22:30:25  simonhan
 *  sos micaz
 *
 *  Revision 1.2  2004/11/15 04:06:58  ram
 *  Moved the Zigbee MAC to the
 *
 *  Revision 1.1  2004/08/26 20:06:39  ram
 *  MicaZ IEEE 802.15.4 MAC
 *
 *  Revision 1.3  2004/03/30 14:58:45  mbr
 *  Release for web
 *                                                                                                     *
 *
 *
 *******************************************************************************************************/
#ifndef BASIC_RF_H
#define BASIC_RF_H
#include <sos.h>




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                 General constants                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Constants concerned with the Basic RF packet format

// Packet overhead ((frame control field, sequence number, PAN ID, destination and source) + (footer))
// Note that the length byte itself is not included included in the packet length
#define BASIC_RF_PACKET_OVERHEAD_SIZE   ((2 + 1 + 2 + 2 + 2) + (2))
#define BASIC_RF_MAX_PAYLOAD_SIZE		(127 - BASIC_RF_PACKET_OVERHEAD_SIZE)
#define BASIC_RF_ACK_PACKET_SIZE		5

// The time it takes for the acknowledgment packet to be received after the data packet has been
// transmitted
#define BASIC_RF_ACK_DURATION			(32 * ((4 + 1) + (1) + (2 + 1) + (2)))
#define BASIC_RF_SYMBOL_DURATION	    (32 * ((4 + 1) + (1) + (2 + 1) + (2)))

// The length byte
#define BASIC_RF_LENGTH_MASK            0x7F

// Frame control field
#define BASIC_RF_FCF_NOACK              0x8841
#define BASIC_RF_FCF_ACK                0x8861
#define BASIC_RF_FCF_ACK_BM             0x0020
#define BASIC_RF_FCF_BM                 (~BASIC_RF_FCF_ACK_BM)
#define BASIC_RF_ACK_FCF		        0x0002

// Footer
#define BASIC_RF_CRC_OK_BM              0x80
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                Packet transmission                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The data structure which is used to transmit packets
typedef struct {
    WORD destPanId;
	WORD destAddr;
	INT8 length;
    BYTE *pPayload;
	BOOL ackRequest;
} BASIC_RF_TX_INFO;
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  BYTE basicRfSendPacket(BASIC_RF_TX_INFO *pRTI)
//
//  DESCRIPTION:
//		Transmits a packet using the IEEE 802.15.4 MAC data packet format with short addresses. CCA is
//		measured only once before backet transmission (not compliant with 802.15.4 CSMA-CA).
//		The function returns:
//			- When pRTI->ackRequest is FALSE: After the transmission has begun (SFD gone high)
//			- When pRTI->ackRequest is TRUE: After the acknowledgment has been received/declared missing.
//		The acknowledgment is received through the FIFOP interrupt.
//
//  ARGUMENTS:
//      BASIC_RF_TX_INFO *pRTI
//          The transmission structure, which contains all relevant info about the packet.
//
//  RETURN VALUE:
//		BOOL
//			Successful transmission (acknowledgment received)
//-------------------------------------------------------------------------------------------------------
BOOL basicRfSendPacket(BASIC_RF_TX_INFO *pRTI);




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                 Packet reception                  **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The receive struct:
typedef struct {
    BYTE seqNumber;
	WORD srcAddr;
	WORD srcPanId;
	INT8 length;
    BYTE *pPayload;
	BOOL ackRequest;
	INT8 rssi;
} BASIC_RF_RX_INFO;
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  void halRfReceiveOn(void)
//
//  DESCRIPTION:
//      Enables the CC2420 receiver and the FIFOP interrupt. When a packet is received through this
//      interrupt, it will call halRfReceivePacket(...), which must be defined by the application
//-------------------------------------------------------------------------------------------------------
void basicRfReceiveOn(void);


//-------------------------------------------------------------------------------------------------------
//  void halRfReceiveOff(void)
//
//  DESCRIPTION:
//      Disables the CC2420 receiver and the FIFOP interrupt.
//-------------------------------------------------------------------------------------------------------
void basicRfReceiveOff(void);


//-------------------------------------------------------------------------------------------------------
//  SIGNAL(SIG_INTERRUPT0) - CC2420 FIFOP interrupt service routine
//
//  DESCRIPTION:
//		When a packet has been completely received, this ISR will extract the data from the RX FIFO, put
//		it into the active BASIC_RF_RX_INFO structure, and call basicRfReceivePacket() (defined by the
//		application). FIFO overflow and illegally formatted packets is handled by this routine.
//
//      Note: Packets are acknowledged automatically by CC2420 through the auto-acknowledgment feature.
//-------------------------------------------------------------------------------------------------------
// SIGNAL(SIG_INTERRUPT0)


//-------------------------------------------------------------------------------------------------------
//  BASIC_RF_RX_INFO* basicRfReceivePacket(BASIC_RF_RX_INFO *pRRI)
//
//  DESCRIPTION:
//      This function is a part of the basic RF library, but must be declared by the application. Once
//		the application has turned on the receiver, using basicRfReceiveOn(), all incoming packets will
//		be received by the FIFOP interrupt service routine. When finished, the ISR will call the
//		basicRfReceivePacket() function. Please note that this function must return quickly, since the
//		next received packet will overwrite the active BASIC_RF_RX_INFO structure (pointed to by pRRI).
//
//  ARGUMENTS:
//		BASIC_RF_RX_INFO *pRRI
//	      	The reception structure, which contains all relevant info about the received packet.
//
//  RETURN VALUE:
//     BASIC_RF_RX_INFO*
//			The pointer to the next BASIC_RF_RX_INFO structure to be used by the FIFOP ISR. If there is
//			only one buffer, then return pRRI.
//-------------------------------------------------------------------------------------------------------
BASIC_RF_RX_INFO* basicRfReceivePacket(BASIC_RF_RX_INFO *pRRI);




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                  Initialization                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The RF settings structure:
typedef struct {
    BASIC_RF_RX_INFO *pRxInfo;
    UINT8 txSeqNumber;
    volatile BOOL ackReceived;
    WORD panId;
    WORD myAddr;
    BOOL receiveOn;
} BASIC_RF_SETTINGS;
extern volatile BASIC_RF_SETTINGS rfSettings;
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//  void basicRfInit(BASIC_RF_RX_INFO *pRRI, UINT8 channel, WORD panId, WORD myAddr)
//
//  DESCRIPTION:
//      Initializes CC2420 for radio communication via the basic RF library functions. Turns on the
//		voltage regulator, resets the CC2420, turns on the crystal oscillator, writes all necessary
//		registers and protocol addresses (for automatic address recognition). Note that the crystal
//		oscillator will remain on (forever).
//
//  ARGUMENTS:
//      BASIC_RF_RX_INFO *pRRI
//          A pointer the BASIC_RF_RX_INFO data structure to be used during the first packet reception.
//			The structure can be switched upon packet reception.
//      UINT8 channel
//          The RF channel to be used (11 = 2405 MHz to 26 = 2480 MHz)
//      WORD panId
//          The personal area network identification number
//      WORD myAddr
//          The 16-bit short address which is used by this node. Must together with the PAN ID form a
//			unique 32-bit identifier to avoid addressing conflicts. Normally, in a 802.15.4 network, the
//			short address will be given to associated nodes by the PAN coordinator.
//-------------------------------------------------------------------------------------------------------
void basicRfInit(BASIC_RF_RX_INFO *pRRI, UINT8 channel, WORD panId, WORD myAddr);


#endif
