/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                                   TX Engine                                    *
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
 * This module contains the MAC TX engine, which is used to transmit all RF packets.                   *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_TX_ENGINE_H
#define MAC_TX_ENGINE_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Bits to be used with MAC_TX_PACKET.txMode
#define MTX_MODE_USE_CSMACA_BM          0x01 /* Transmitted using CSMA-CA */
#define MTX_MODE_FORCE_UNSLOTTED_BM     0x02 /* Force unslotted transmission in a beacon network */
#define MTX_MODE_MAC_INTERNAL_BM        0x04 /* The packet is transmitted by the MAC layer */
#define MTX_MODE_SCAN_RELATED_BM        0x08 /* This packet is related to channel scanning */
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                  STATE MACHINES                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Return values for csmaSetStartupTime(...)
#define MTX_SST_SUCCESS                     0
#define MTX_SST_RESOURCE_SHORTAGE           1
#define MTX_SST_INDIRECT_PACKET_TOO_LATE    2

// mtxScheduleTransmission
#define MTX_STATE_INIT_TRANSMISSION         0
#define MTX_STATE_INIT_CSMACA               1
#define MTX_STATE_SET_STARTUP_TIME          2
#define MTX_STATE_WAIT                      3

// mtxStartTransmission
#define MTX_STATE_TURN_ON_RX                0
#define MTX_STATE_PREPARE                   1
#define MTX_STATE_CCA                       2
#define MTX_STATE_START_TRANSMISSION        3
                                            
// TX status                                
#define MTX_STATUS_WAITING                  0
#define MTX_STATUS_TRANSMISSION_STARTED     1
#define MTX_STATUS_TX_FINISHED              2
#define MTX_STATUS_CHANNEL_ACCESS_FAILURE   3
#define MTX_STATUS_ACK_TIMEOUT              4
#define MTX_STATUS_ACK_RECEIVED             5
#define MTX_STATUS_ACK_HANDLER_CREATED      6
#define MTX_STATUS_FINISHED                 7
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Internal module data
typedef struct {
    // A pointer to the active packet (this pointer can be used by mcpsPurgeRequest() to check
    // whether or not the packet is in the CSMA-CA mechanism
    MAC_TX_PACKET *pPacket;

    // CSMA-CA "state machine"
    UINT8 be;            // Backoff exponent
    UINT8 nb;            // Number of backoffs
    
    // The status which is reported after the packet has been scheduled
    UBYTE status;
    
    // Variables used when the CSMA-CA mechanism is resumed after the beacon
    BOOL waitForBeacon;
    BOOL searchForBeacon; // The TX engine has initiated a search for the beacon
    INT8 randomBackoff;
    
    // A permanently reserved task used to start the transmission (mtxStartTransmission(...))
    UINT8 startTxTask;
    
} MAC_TX_INFO;
extern MAC_TX_INFO mtxInfo;
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// msecFindSecurityMaterial and msecProcessSecurityCounters function prototypes. Moved from 
// mac_security.h because of type definition conflicts between these files and function headers
void msecFindTxSecurityMaterial(MAC_TX_PACKET *pPacket, BOOL securityEnable, UBYTE addrMode, UHWORD panId, ADDRESS *pAddr);
UINT8 msecProcessSecurityCounters(MAC_TX_PACKET *pPacket, UBYTE *pPayload);
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Entry function
void mtxScheduleTransmission(MAC_TASK_INFO *pTask);

// Transmission functions (CCA + TX on + FIFO writing)
void mtxStartTransmission(MAC_TASK_INFO *pTask);

// Beacon location (when not tracking in a beacon network)
void mtxLocateBeaconTimeout(void);

// Startup time
void mtxCreateStartTask(void);
BOOL mtxStartAfterDelay(UINT8 delay);
BOOL mtxSetStartupTime(void);

// Timeout
void mtxPacketTimeout(void);

// Resume transmission after beacon reception/transmission (out of CAP in the last superframe)
void mtxResumeAfterBeacon(void);
void mtxResumeAfterBeaconCallback(void);
void mtxResumeAfterBeaconTask(MAC_TASK_INFO *pTask);

// Generates a properly formatted comm-status indication (for TX) to the higher layer
void mtxCommStatusIndication(MAC_TX_PACKET *pPacket, UBYTE status);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_tx_engine.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:51  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:29  simonhan
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
 * Revision 1.5  2004/08/13 13:04:44  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
