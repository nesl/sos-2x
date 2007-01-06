/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                                 Scan Functions                                 *
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
 * This module contains the scan state machine, and transmissin of scan-related packets.               *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACSCAN_H
#define MACSCAN_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// A bit-mask containing the valid channels for 2.4 GHz devices (channels 11 to 26)
#define MSC_VALID_CHANNELS                  0x07FFF800

// The scan status returned to mlmeScanRequest(...)
typedef enum {
    MSC_STATUS_ACTIVE,
    MSC_STATUS_FINISHED,
    MSC_STATUS_ORPHAN_REALIGNED    
} MSC_SCAN_STATUS;
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                  STATE MACHINES                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Scan state machine

// Initial preparations
#define MSC_STATE_INITIALIZE_SCAN           0
#define MSC_STATE_UNREQUEST_INDIRECT        1

// Preparations for each channel
#define MSC_STATE_SET_CHANNEL               2

// Scan states (E = Energy detection, A = Active, O = Orphan, P = Passive)
#define MSC_STATE_E_SAMPLE                  3
#define MSC_STATE_A_TX_BEACON_REQUEST       4
#define MSC_STATE_O_TX_ORPHAN_NOTIFICATION  5
#define MSC_STATE_APO_WAIT                  6

// Channel complete
#define MSC_STATE_NEXT_CHANNEL              7

// Scan complete
#define MSC_STATE_FINISH                    8
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Internal module data
typedef struct {
    
    // Scan parameters
    UBYTE scanType;
    DWORD scanChannels;
    UINT8 scanDuration;
    MAC_SCAN_RESULT *pScanResult;
    
    // Scan status
    UINT8 currentChannel;
    volatile BOOL channelComplete;
    volatile MSC_SCAN_STATUS scanStatus;
    
    // Saved variables
    UINT8 oldPhyCurrentChannel;
    
} MAC_SCAN_INFO;
extern MAC_SCAN_INFO mscInfo;
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Used by a coordinator to transmit a single beacon in a non-beacon network
BOOL mscTransmitBeacon(void);

// Used by a scanning device to transmit a beacon request frame
BOOL mscTransmitBeaconRequest(void);

// Scan state machine + channel timeout
void mscScanProcedure(MAC_TASK_INFO *pTask);
void mscChannelTimeout(void);

// Searches for a found PAN descriptor in the current result list (used to avoid duplicates)
BOOL mscPanDescriptorExists(PAN_DESCRIPTOR *pPanDescriptor);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_scan.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:50  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:28  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:01  simonhan
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
 * Revision 1.4  2004/08/13 13:04:43  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
