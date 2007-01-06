/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                        Beacon TX and Tracking + Related                        *
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
 * This module contains functions the functions responsible for receiving and transmitting beacons,    *
 * and related functions, such as synchronization and coordinator realignment.                         *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACBEACONHANDLER_H
#define MACBEACONHANDLER_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Time constants (number of backoff periods)

// The overhead related to beacon reception (the number of backoff periods used to turn on RX)
#define MBCN_RX_STARTUP_OVERHEAD 2

// The number of backoff periods used by the TX engine to initiate the transmission (do not change!!!)
#define MBCN_TX_STARTUP_OVERHEAD 3

// The number of backoff periods required to generate the beacon packet, and perform various housekeeping
// procedures (e.g. expiration of old indirect packets). May change depending on the size of the indirect
// packet queue.
#define MBCN_TX_PREPARE_TIME 7
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                  STATE MACHINES                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// RX state machine
#define MBCN_SET_NEXT_CALLBACK_RXON     0
#define MBCN_HANDLE_TRACKING            1

// TX state machine
#define MBCN_SET_NEXT_CALLBACK_PREPTX   0
#define MBCN_START_TRANSMISSION         1
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Internal module data

// Some parameters from the superframe specification in the last beacon
typedef struct {
    UINT8 finalCap;
    BOOL battLifeExt;
} MAC_SUPERFRAME_SPEC;

// Module data
typedef struct {

    // The tasks used to transmit/receive periodical beacons
    UINT8 txTaskNumber;
    UINT8 rxTaskNumber;
    
    // This packet must only be used with periodically transmitted beacons, because it will affect how the beacon is transmitted.
    // The pointer must be NULL when regular beacon transmission is off.
    MAC_TX_PACKET *pTxPacket;
    
    // The duration (number of backoff periods) of the most recently received beacon. This number includes
    // the interframe spacing following the beacon, and is rounded up to the closest backoff slot.
    UINT8 beaconDuration;
    
    // Beacon tracking (controlled by mlmeSyncRequest)
    BOOL trackBeacon; // The beacon is currently being tracked
    BOOL findBeacon;  // Searching for a single beacon (RX constantly on)
    
    // The number of beacons that still can be lost before tracking fails
    UINT8 noBcnCountdown;
    
    MAC_SUPERFRAME_SPEC lastSfSpec;
    
} MAC_BEACON_INFO;
extern MAC_BEACON_INFO mbcnInfo;
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Beacon reception (tracking)
void mbcnRxPeriodicalBeacon(void);
void mbcnRxPeriodicalBeaconTask(MAC_TASK_INFO *pTask);
void mbcnRxBeaconTimeout(void);

// Beacon transmission
void mbcnTxPeriodicalBeacon(void);
void mbcnTxPeriodicalBeaconTask(MAC_TASK_INFO *pTask);

// Frame preparation function
void mbcnPrepareBeacon(MAC_TX_PACKET *pPacket);
void mbcnPrepareCoordinatorRealignment(MAC_TX_PACKET *pPacket, QWORD *pDestAddr, UHWORD shortAddr, BOOL securityEnable, UHWORD newPanId, UINT8 logicalChannel);

// Beacon reception margin (required because of clock inaccuracy)
UINT8 mbcnGetBeaconMargin(void);

// PIB attribute related
void mbcnHandleBeaconModeChange(BOOL wasNonBeacon);
void mbcnUpdateBufferedPibAttributes(void);

// Synchronization to beacons (run by mlmeSyncRequest())
void mbcnSyncToBeacons(MAC_TASK_INFO *pTask);

// Expiration of indirect packets at regular intervals in a non-beacon network
void mbcnExpirePacketsNonBeacon(void);
void mbcnExpirePacketsNonBeaconTask(MAC_TASK_INFO *pTask);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_beacon_handler.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:48  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:27  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:11:59  simonhan
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
 * Revision 1.7  2004/08/13 13:04:42  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
