/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                                   RX Engine                                    *
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
 * This module contains the MAC RX engine (FIFOP interrupt), including packet processing functions,    *
 * and functions to control the RX state (on/off...)                                                   *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_RX_ENGINE_H
#define MAC_RX_ENGINE_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The time constant used when delaying the re-activation of the FIFOP interrupt
#define MRX_FIFOP_INT_ON_DELAY  200 /* MCU cycles */

// Flags used as taskData in mrxForceRxOffTask(...)
#define MRX_PRESERVE_ON_COUNTER 0
#define MRX_RESET_ON_COUNTER    1
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                  STATE MACHINES                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
typedef enum {
    MRX_STATE_LEN_FCF_SEQ = 0,
    MRX_STATE_DEST_ADDR,
    MRX_STATE_SRC_ADDR,
    MRX_STATE_SECURITY_INIT,
    MRX_STATE_SECURITY_ACL_SEARCH,
    MRX_STATE_FIND_COUNTERS,
    MRX_STATE_CMD_IDENTIFIER,
    MRX_STATE_READ_FRAME_COUNTER,
    MRX_STATE_READ_KEY_SEQUENCE_COUNTER,
    MRX_STATE_KEY_SETUP_AND_DECRYPT,
    MRX_STATE_PAYLOAD,
    MRX_STATE_FCS,
    MRX_STATE_DISCARD_ALL,
    MRX_STATE_DISCARD_REMAINING,
    MRX_STATE_DISCARD
} MAC_RX_STATE;

// mrxProcessBeacon
#define MRX_STATE_BCN_ALIGN_OR_SPEC_MODES       0
#define MRX_STATE_BCN_TX_AFTER_BEACON           1
#define MRX_STATE_BCN_EXAMINE_PENDING_FIELDS    2
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Internal module data
typedef struct {
    // The four initial bytes of the received packet
    INT8 length;
    UHWORD frameControlField;
    UINT8 sequenceNumber;

    // Data used in the RX engine state machine
    union {
        struct {
            UINT8 dstAddrLength;
            UINT8 srcAddrLength;
            UBYTE *pDstAddrStoragePtr;
            UBYTE *pSrcAddrStoragePtr;
        };
        struct {
            UBYTE *pPayloadStoragePtr;
            UBYTE pFooter[2];
        };
    };

    // A pointer to the structure that stores the received packet
    MAC_RX_PACKET *pMrxPacket;

    // The task that will be responsible for processing the received packet
    UINT8 taskNumber;

    // The current state of the MAC RX engine
    volatile MAC_RX_STATE state;

    // This counter decides whether or not the receiver is on (greater than 0) or off (0)
    //UINT8 onCounter;
    UWORD onCounter;		// A bug in the MAC increments this until overflow which causes the Receiver to disable. We avoid that for a while.

    // FIFO underflow prevention/detection
    BOOL handleFifoUnderflow;
    BOOL keepFifopIntOff;

    // mlmeRxEnableRequest parameters
    UINT32 rxEnableOnDuration;

#if (MAC_OPT_ACL_SIZE>0)
    UINT8 aclEntrySearch;           // Used for indexing during search for ACL
#endif

} MAC_RX_INFO;
extern MAC_RX_INFO mrxInfo;

#if MAC_OPT_SECURITY
typedef struct {
    DWORD frameCounter;             // Incoming Frame Counter
    UBYTE keySequenceCounter;        // Incoming Key Sequence Counter
    UBYTE securitySuite;
    UINT8 authenticateLength;
    MSEC_SETUP_INFO securitySetup;
    SECURITY_MATERIAL* pMrxSecurityMaterial;
    QWORD* pExtendedAddress;
} MAC_RX_SECURITY_INFO;

extern MAC_RX_SECURITY_INFO mrxSecurityInfo;
#endif // MAC_OPT_SECURITY

//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// RX state control

// The counter which is used to control RX on/off. RX is off when the counter is 0, or on otherwise. The
// counter is incremented each time there is a reason for being in RX, or when the chip will turn on RX
// automatically (after TX)
void mrxIncrOnCounter(void);
void mrxAutoIncrOnCounter(void);
void mrxDecrOnCounter(void);

// Functions related to forcing RX off, and cleaning up (to avoid FIFO underflow)
void mrxForceRxOff(void);
void mrxForceRxOffTask(MAC_TASK_INFO *pTask);
void mrxResetRxEngine(void);

// The task created by mlmeRxEnableRequest()
void mrxRxEnableRequestTask(MAC_TASK_INFO *pTask);
void mrxRxEnableRequestOff(void);

// Fifop Interrupt Handler
void FifopHandler(void);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_rx_engine.h,v $
 * Revision 1.2  2005/11/07 21:55:30  abs
 * add include string.h to the mac_headers to declare the memcpy routine.
 * clean up the definitions for the interrupt control macros.
 * increase the size of the onCounter to avoid the counter overflow that was effectively disabling the receiver.
 *
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
 * Revision 1.9  2004/08/13 13:04:43  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
