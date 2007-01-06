/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                                RX Packet Pool                                  *
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
 * This module contains the TX packet pool, which manages a table of MAC_TX_PACKET structures to be    *
 * used with the TX engine.                                                                            *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_TX_POOL_H
#define MAC_TX_POOL_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The maximum length of the packet header
#define MTX_MAX_HEADER_LENGTH 23

// The maximum length of the security field (TBD)
#define MTX_MAX_SECURITY_LENGTH 5

// The length of the frame check sequence
#define MAC_FCS_LENGTH 2
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Packet TX structure
typedef struct {
    volatile BOOL occupied;
    
    // Transmission buffer
    UINT8 headerLength;
    UINT8 length;
    UBYTE pHeader[MTX_MAX_HEADER_LENGTH]; // Frame control field (2) | Sequence number (1) | Addressing fields (0 to 20)
    UBYTE pPayload[aMaxMACFrameSize];

    // Various packet parameters
    UBYTE type;                  // FT_BEACON, FT_DATA or FT_MAC_COMMAND
    UBYTE txOptions;
    union {
        UBYTE msduHandle;
        UBYTE commandType;
    };
    BOOL toCoord;
    UINT8 duration;             // Backoff periods including ACK and IFS
    UBYTE txMode;
    BOOL slotted;               // Set automatically by the TX engine

    // For indirect packets
    INT16 timeToLive;           // This variable is signed because "time to live" becomes -1 when purged
    BOOL requested;
    BOOL transmissionStarted;   // This flag indicates that the transmission has been started and that the packet cannot be removed
    UINT8 nextPacket;
    UINT8 prevPacket;
    BOOL isFirstPacket;         // Is this the first packet in the queue for this node?
    UINT8 poolIndex;
    
    // The number of retries left (must be placed here because of indirect packets)
    UINT8 retriesLeft;
#if MAC_OPT_SECURITY
    // Security material, if used
    UBYTE securitySuite;
    SECURITY_MATERIAL *pSecurityMaterial;    
    MSEC_SETUP_INFO securitySetup;
#endif

} MAC_TX_PACKET;
extern MAC_TX_PACKET pMtxPacketPool[MAC_OPT_TX_POOL_SIZE];
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Initializes the packet pool
void mtxpInit(void);

// Reserve/release
MAC_TX_PACKET* mtxpReservePacket(void);
void mtxpReleasePacket(MAC_TX_PACKET* pPacket);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_tx_pool.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:51  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:30  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:02  simonhan
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
 * Revision 1.6  2004/08/13 13:04:45  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
