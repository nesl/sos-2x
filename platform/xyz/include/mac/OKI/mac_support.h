/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                       Various Support/Utility Functions                        *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * CONFIDENTIAL                                                                                        *
 * The use of this file is restricted by the signed MAC software license agreement.                    *
 *                                                                                                     *
 * Copyright Chipcon AS, 2004                                                                          *
 *******************************************************************************************************
 * This module contains support/utility functions for the MAC sublayer.                                *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_SUPPORT_H
#define MAC_SUPPORT_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Creates a task that modifies a flag when run, and then just exits
void msupWaitTask(MAC_TASK_INFO *pTask);

// Compares two QWORDs
BOOL msupCompareQword(QWORD *pA, QWORD *pB);

// Copies an array of data in the reverse order
void msupReverseCopy(UBYTE *pDestination, UBYTE *pSource, UINT8 length);

// Pseudo-random generator
void msupInitRandom(void);
UBYTE msupGetRandomByte(void);

// RF channel
BOOL msupChannelValid(UINT8 logicalChannel);
void msupSetChannel(UINT8 logicalChannel, BOOL changePib);

// Generates packet headers
void msupPrepareHeader(MAC_TX_PACKET *pTxPacket, UBYTE type, UBYTE addrModes, UHWORD srcPanId, ADDRESS *pSrcAddr, UHWORD destPanId, ADDRESS *pDestAddr, UBYTE txOptions);

// CC2420 FIFO/RAM access
void msupReadFifo(void *pData, UINT8 count);
void msupWriteFifo(void *pData, UINT8 count);
void msupWriteRam(void *pData, UINT16 address, UINT8 count, BOOL disableInterrupts);
void msupReadRam(void *pData, UINT16 address, UINT8 count);

// Calculates the duration of a packet, including acknowledgment and interframe-spacing
UINT8 msupCalcPacketDuration(UINT8 length, BOOL ackRequest);

// Simple calculations (using the current PIB attribute values)
UINT32 msupCalcCapDuration(void);
UINT32 msupCalcSuperframeDuration(void);
UINT32 msupCalcBeaconInterval(void);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_support.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:50  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:29  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:01  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.3  2004/10/27 03:49:26  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.4  2004/08/13 13:04:44  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
