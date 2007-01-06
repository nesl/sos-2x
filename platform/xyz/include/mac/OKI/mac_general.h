/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                       General MAC Constants, Types, etc.                       *
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
 * This module contains general types, constants, macros and functions used by many other MAC modules. *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACGENERAL_H
#define MACGENERAL_H


/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Inherent MAC constants

// The maximum number of addresses shown in a pending list (in a beacon)
#define MAX_PENDING_LIST_SIZE   7
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// MAC command packet types
typedef enum {
    CMD_ASSOCIATION_REQUEST = 1,
    CMD_ASSOCIATION_RESPONSE,
    CMD_DISASSOCIATION_NOTIFICATION,
    CMD_DATA_REQUEST,
    CMD_PAN_ID_CONFLICT_NOTIFICATION,
    CMD_ORPHAN_NOTIFICATION,
    CMD_BEACON_REQUEST,
    CMD_COORDINATOR_REALIGNMENT,
    CMD_GTS_REQUEST
} MAC_COMMAND_TYPE;
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// MAC command payload lengths, including the command frame identifier

#define CMD_ASSOCIATION_REQUEST_PAYLOAD_LENGTH          2
#define CMD_ASSOCIATION_RESPONSE_PAYLOAD_LENGTH         4
#define CMD_DISASSOCIATION_NOTIFICATION_PAYLOAD_LENGTH  2
#define CMD_DATA_REQUEST_PAYLOAD_LENGTH                 1
#define CMD_PAN_ID_CONFLICT_NOTIFICATION_PAYLOAD_LENGTH 1
#define CMD_ORPHAN_NOTIFICATION_PAYLOAD_LENGTH          1
#define CMD_BEACON_REQUEST_PAYLOAD_LENGTH               1
#define CMD_COORDINATOR_REALIGNMENT_PAYLOAD_LENGTH      8
#define CMD_GTS_REQUEST_PAYLOAD_LENGTH                  2


//-------------------------------------------------------------------------------------------------------



//-------------------------------------------------------------------------------------------------------
// Frame control field formatting

// Indexes
#define FRAME_TYPE_IDX              0
#define SECURITY_ENABLED_IDX        3
#define FRAME_PENDING_IDX           4
#define ACK_REQ_IDX                 5
#define INTRA_PAN_IDX               6
#define DEST_ADDR_MODE_IDX          10
#define SRC_ADDR_MODE_IDX           14

// Bit masks
#define FRAME_TYPE_BM               0x0007
#define SECURITY_ENABLED_BM         0x0008
#define FRAME_PENDING_BM            0x0010
#define ACK_REQ_BM                  0x0020
#define INTRA_PAN_BM                0x0040
#define DEST_ADDR_MODE_BM           0x0C00
#define SRC_ADDR_MODE_BM            0xC000

// FRAME_TYPE
#define FT_BEACON                   0
#define FT_DATA                     1
#define FT_ACKNOWLEDGMENT           2
#define FT_MAC_COMMAND              3
#define FT_100                      4
#define FT_101                      5
#define FT_110                      6
#define FT_111                      7

// ADDR_MODE
// See mac.h
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Scan types
#define ENERGY_SCAN                 0
#define ACTIVE_SCAN                 1
#define PASSIVE_SCAN                2
#define ORPHAN_SCAN                 3
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Disassociate reasons
#define COORD_WISHES_DEVICE_TO_LEAVE 1
#define DEVICE_WISHES_TO_LEAVE       2
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Superframe specification field formatting (beacon)

// Indexes
#define SS_BEACON_ORDER_IDX         0
#define SS_SUPERFRAME_ORDER_IDX     4
#define SS_FINAL_CAP_SLOT_IDX       8
#define SS_BATT_LIFE_EXT_IDX        12
#define SS_PAN_COORDINATOR_IDX      14
#define SS_ASSOCIATION_PERMIT_IDX   15

// Bit masks
#define SS_BEACON_ORDER_BM          0x000F
#define SS_SUPERFRAME_ORDER_BM      0x00F0
#define SS_FINAL_CAP_SLOT_BM        0x0F00
#define SS_BATT_LIFE_EXT_BM         0x1000
#define SS_PAN_COORDINATOR_BM       0x4000
#define SS_ASSOCIATION_PERMIT_BM    0x8000
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Capability information field formatting (association request)

// Indexes
#define CI_ALTERNATE_PAN_COORD_IDX  0
#define CI_DEVICE_TYPE_IS_FFD_IDX   1
#define CI_POWER_SOURCE_IDX         2
#define CI_RX_ON_WHEN_IDLE_IDX      3
#define CI_SECURITY_CAPABILITY_IDX  6
#define CI_ALLOCATE_ADDRESS_IDX     7

// Bit masks
#define CI_ALTERNATE_PAN_COORD_BM   0x01
#define CI_DEVICE_TYPE_IS_FFD_BM    0x02
#define CI_POWER_SOURCE_BM          0x04
#define CI_RX_ON_WHEN_IDLE_BM       0x08
#define CI_SECURITY_CAPABILITY_BM   0x40
#define CI_ALLOCATE_ADDRESS_BM      0x80
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//----------------------------------------------------------------------------------------------------------


typedef UBYTE KEY[16];

typedef struct {
    KEY pSymmetricKey;
    DWORD frameCounter;
    UBYTE keySequenceCounter;
#if MAC_OPT_SEQUENTIAL_FRESHNESS
    DWORD externalFrameCounter;
    UBYTE externalKeySequenceCounter;
#endif
} SECURITY_MATERIAL;

// Access Control List
typedef struct {
    QWORD extendedAddress;
    UHWORD shortAddress;
    UHWORD panId;
    UBYTE securityMaterialLength;
    SECURITY_MATERIAL securityMaterial;
    UBYTE securitySuite;
} ACL_ENTRY;

typedef ACL_ENTRY ACL_ENTRY_SET[MAC_OPT_ACL_SIZE];

//----------------------------------------------------------------------------------------------------------
// MAC and PHY PAN information bases

// MAC PIB
typedef struct {
    UBYTE macAckWaitDuration;
    BOOL macAssociationPermit;
    BOOL macAutoRequest;
    BOOL macBattLifeExt;
    UBYTE macBattLifeExtPeriods;
    UBYTE* pMacBeaconPayload;
    UBYTE macBeaconPayloadLength;
    UBYTE macBeaconOrder;
    DWORD macBeaconTxTime;
    UBYTE macBSN;
    QWORD macCoordExtendedAddress;
    UHWORD macCoordShortAddress;
    UBYTE macDSN;
    BOOL macGTSPermit;
    UBYTE macMaxCsmaBackoffs;
    UBYTE macMinBE;
    UHWORD macPANId;
    BOOL macPromiscuousMode;
    BOOL macRxOnWhenIdle;
    UHWORD macShortAddress;
    UBYTE macSuperframeOrder;
    UHWORD macTransactionPersistenceTime;

    // ACL attributes
#if MAC_OPT_ACL_SIZE > 0
    ACL_ENTRY_SET* ppMacACLEntryDescriptorSet;
    UBYTE macACLEntryDescriptorSetSize;
#endif

    // Security attributes
#if MAC_OPT_SECURITY
    BOOL macDefaultSecurity;
    UBYTE macDefaultSecurityMaterialLength;
    SECURITY_MATERIAL* pMacDefaultSecurityMaterial;
    UBYTE macDefaultSecuritySuite;
#endif

#if ((MAC_OPT_SECURITY) || (MAC_OPT_ACL_SIZE>0))
    UBYTE macSecurityMode;
#endif

} MAC_PIB;
extern MAC_PIB mpib;

// PHY PIB
typedef struct {
    UINT8 phyCurrentChannel;
    UINT8 phyTransmitPower;
    UINT8 phyCcaMode;
} PHY_PIB;
extern PHY_PIB ppib;




//-------------------------------------------------------------------------------------------------------
// Internal MAC state

// Bit masks
#define MAC_STATE_TX_DATA_REQUEST_BM    0x10
#define MAC_STATE_DATA_REQUEST_SENT_BM  0x20
#define MAC_STATE_SCAN_BM               0x40
#define MAC_STATE_ASSOC_BM              0x80

// The internal MAC states
typedef enum {
    MAC_STATE_DEFAULT = 0x00,
    
    MAC_STATE_TX_AUTO_DATA_REQUEST = MAC_STATE_TX_DATA_REQUEST_BM,
    MAC_STATE_TX_MANUAL_DATA_REQUEST,
    MAC_STATE_TX_ASSOC_DATA_REQUEST,
    
    MAC_STATE_AUTO_DATA_REQUEST_SENT = MAC_STATE_DATA_REQUEST_SENT_BM,
    MAC_STATE_MANUAL_DATA_REQUEST_SENT,
    MAC_STATE_ASSOC_DATA_REQUEST_SENT,
    
    MAC_STATE_ENERGY_SCAN = MAC_STATE_SCAN_BM,
    MAC_STATE_ACTIVE_OR_PASSIVE_SCAN,
    MAC_STATE_SCAN_RESULT_BUFFER_FULL,
    MAC_STATE_ORPHAN_SCAN,
    MAC_STATE_ORPHAN_REALIGNED,
    
    MAC_STATE_TX_ASSOC_REQUEST = MAC_STATE_ASSOC_BM,
    MAC_STATE_ASSOC_REQUEST_SENT
} MAC_STATE_TYPE;


// MAC_ENUM definition, for MAC internal use only
#define RESOURCE_SHORTAGE            0x01


// Temporary buffer for some PIB attributes (the PIB is updated when the next beacon is received or 
// transmitted). These are the category 4 attributes from mlmeSetRequest().
#define MPIB_UPD_BATT_LIFE_EXT_BM    0x01 
#define MPIB_UPD_BEACON_ORDER_BM     0x02 
#define MPIB_UPD_RX_ON_WHEN_IDLE_BM  0x04 
#define MPIB_UPD_SUPERFRAME_ORDER_BM 0x08 

// Data holding structure + a mask to tell which ones to update
typedef struct {
    BOOL macBattLifeExt;
    UINT8 macBeaconOrder;
    BOOL macRxOnWhenIdle;
    UINT8 macSuperframeOrder;
    UBYTE updateMask;
} MAC_PIB_TEMP_BUFFER;

// Internal module data
typedef struct {
    volatile MAC_STATE_TYPE state;
    MAC_PIB_TEMP_BUFFER pibTempBuffer;
    UBYTE flags;
} MAC_INFO;
extern MAC_INFO macInfo;
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// Internal mac flags

// Access macros
#define SET_MF(n)           do { macInfo.flags |= n; } while (0)
#define CLEAR_MF(n)         do { macInfo.flags &= ~n; } while (0)
#define RESET_MF()          do { macInfo.flags = 0; } while (0)
#define GET_MF(n)           (!!(macInfo.flags & n))

// Bit definitions
#define MF_COORDINATOR      0x01 /* The device is a coordinator */
#define MF_PAN_COORDINATOR  0x02 /* The device is a PAN coordinator (set in addition to MF_COORDINATOR) */
#define MF_BEACON_SECURITY  0x04 /* Beacon security is enabled */
#define MF_TRANSMIT_BEACON  0x08 /* The device is currently transmitting beacons */
#define MF_SECURE_ASSOC     0x10 /* Set when performing a secure association */
//----------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//----------------------------------------------------------------------------------------------------------
// Changes the internal MAC state
BOOL macSetState(MAC_STATE_TYPE newState);
//----------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_general.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:49  simonhan
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
 * Revision 1.6  2004/08/13 13:04:42  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
