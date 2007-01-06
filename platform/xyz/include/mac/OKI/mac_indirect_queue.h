/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                        Indirect Packet Queue + Related                         *
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
 * This module contains functions related to the indirect packet queue (on coordinators), including    *
 * queue management, transmission initiation, expiration, and other functions.                         *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACINDIRECTQUEUE_H
#define MACINDIRECTQUEUE_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// MAC_TX_PACKET.timeToLive is set to MIQ_PACKET_PURGED to indicate that a packet was purged (callbacks
#define MIQ_PACKET_PURGED -1

// Packet queue termination
#define NO_PACKET 0xFF
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                  STATE MACHINES                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// miqAddIndirectPacket
#define MIQ_STATE_FIND_FIRST_PACKET_FROM_NODE  0
#define MIQ_STATE_INSERT_INTO_QUEUE            1

// miqRemoveIndirectPacket
#define MIQ_STATE_REMOVE_PACKET                0
#define MIQ_STATE_MOVE_FIRST_PACKET_FLAG       1

// miqExpireIndirectPacketsTask
#define MIQ_STATE_FIND_EXPIRED_PACKET          0
#define MIQ_STATE_REMOVE_EXPIRED_PACKET        1
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/

#if MAC_OPT_FFD
//-------------------------------------------------------------------------------------------------------
// Internal module data
typedef struct {
    
    // Indexes of the first and the last packet in the indirect queue
    UINT8 firstIndirectPacket;
    UINT8 lastIndirectPacket;
    
    // The number of requested indirect packets (to tell whether or not to continue looping the 
    // miqTransmitRequestedPackets(...) task.
    UINT8 requestedCounter;
    
} MAC_INDIRECT_QUEUE_INFO;
extern MAC_INDIRECT_QUEUE_INFO miqInfo;
//-------------------------------------------------------------------------------------------------------
#endif // MAC_OPT_FFD




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Queue initialization
void miqInit(void);

// Queue management
void miqAddIndirectPacket(MAC_TASK_INFO *pTask);
void miqRemoveIndirectPacket(MAC_TASK_INFO *pTask);

// Transmission of requested packets
BOOL miqFindAndRequestIndirectPacket(ADDRESS *pDestAddr, BOOL isExtAddr);
void miqSetRequested(MAC_TX_PACKET *pPacket, BOOL requested);
void miqUnrequestAll(void);
void miqTransmitRequestedPackets(MAC_TASK_INFO *pTask);

// Expiration of old or purged packets
void miqExpireIndirectPacketsTask(MAC_TASK_INFO *pTask);
void miqDecrTimeToLive(void);

// Transmission of the empty data packet, which is sent when there is no data for a polling device
BOOL miqTransmitNoDataPacket(ADDRESS *pDestAddr, BOOL isExtAddr);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_indirect_queue.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:49  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:28  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:00  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.3  2004/10/27 03:49:25  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.5  2004/08/13 13:04:43  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
