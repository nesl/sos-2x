/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                        CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                  *
 *      ***   + +   ***                                                                                *
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
 * This file implements the MAC functions (request and response) to be called by the higher layer      *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_H
#define MAC_H

#include <sos_inttypes.h>

//----------------------------------------------------------------------------------------------------------
// MAC framework initialization
#define MAC_INIT() \
    do { \
        DISABLE_GLOBAL_INT(); \
        MAC_PORT_INIT(); \
        SPI_INIT(); \
		SFD_INT_INIT(); \
        FIFOP_INT_INIT(); \
        mschInit(); \
        mtimInit(); \
        macInfo.state = MAC_STATE_DEFAULT; \
        mpmInfo.currentState = MPM_CC2420_XOSC_AND_VREG_OFF; \
        mpmInfo.selectedMode = MPM_CC2420_XOSC_AND_VREG_OFF; \
        mrxInfo.keepFifopIntOff = TRUE; \
        DISABLE_FIFOP_INT(); \
        ENABLE_T1_COMPA_INT(); \
        ENABLE_GLOBAL_INT(); \
    } while (0)
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// PHY / MAC constants
#define aMaxPHYPacketSize       127
#define aTurnaroundTime         12 // symbol periods
#define aUnitBackoffPeriod      20


// Extended address, must be set by higher layer
extern QWORD aExtendedAddress;

// MAC constants
#define aBaseSlotDuration       60
#define aNumSuperframeSlots     16
#define aBaseSuperframeDuration (aBaseSlotDuration * aNumSuperframeSlots)
#define aMaxBE                  5
#define aMaxBeaconOverhead      75
#define aMaxBeaconPayloadLength (aMaxPHYPacketSize - aMaxBeaconOverhead)
#define aGTSDescPersistenceTime 4
#define aMaxFrameOverhead       25
#define aMaxFrameResponseTime   1220
#define aMaxFrameRetries        3
#define aMaxLostBeacons         4
#define aMaxMACFrameSize        (aMaxPHYPacketSize - aMaxFrameOverhead)
#define aMaxSIFSFrameSize       18
#define aMinCAPLength           440
#define aMinLIFSPeriod          40
#define aMinSIFSPeriod          12
#define aResponseWaitTime       (32 * aBaseSuperframeDuration)
#define MAC_OPT_MAX_PAN_DESCRIPTORS 4
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// MAC ACL and security information bases
#define MAC_SECURITY_NONE               0x00
#define MAC_SECURITY_AES_CTR            0x01
#define MAC_SECURITY_AES_CCM128         0x02
#define MAC_SECURITY_AES_CCM64          0x03
#define MAC_SECURITY_AES_CCM32          0x04
#define MAC_SECURITY_AES_CBC_MAC128     0x05
#define MAC_SECURITY_AES_CBC_MAC64      0x06
#define MAC_SECURITY_AES_CBC_MAC32      0x07
#define MAC_HIGHEST_SECURITY_MODE       MAC_SECURITY_AES_CBC_MAC32

#define MAC_UNSECURED_MODE              0x00
#define MAC_ACL_MODE                    0x01
#define MAC_SECURED_MODE                0x02
//----------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------
// MAC PAN information base

// These constants shall be used with mlmeSetRequest and mlmeGetRequest.
typedef enum {
    MAC_ACK_WAIT_DURATION = 0x40,
    MAC_ASSOCIATION_PERMIT,
    MAC_AUTO_REQUEST,
    MAC_BATT_LIFE_EXT,
    MAC_BATT_LIFE_EXT_PERIODS,
    MAC_BEACON_PAYLOAD,
    MAC_BEACON_PAYLOAD_LENGTH,
    MAC_BEACON_ORDER,
    MAC_BEACON_TX_TIME,
    MAC_BSN,
    MAC_COORD_EXTENDED_ADDRESS,
    MAC_COORD_SHORT_ADDRESS,
    MAC_DSN,
    MAC_GTS_PERMIT,
    MAC_MAX_CSMA_BACKOFFS,
    MAC_MIN_BE,
    MAC_PAN_ID,
    MAC_PROMISCUOUS_MODE,
    MAC_RX_ON_WHEN_IDLE,
    MAC_SHORT_ADDRESS,
    MAC_SUPERFRAME_ORDER,
    MAC_TRANSACTION_PERSISTENCE_TIME,
    MAC_ACL_ENTRY_DESCRIPTOR_SET = 0x70,
    MAC_ACL_ENTRY_DESCRIPTOR_SETSIZE,
    MAC_DEFAULT_SECURITY,
    MAC_DEFAULT_SECURITY_MATERIAL_LENGTH,
    MAC_DEFAULT_SECURITY_MATERIAL,
    MAC_DEFAULT_SECURITY_SUITE,
    MAC_SECURITY_MODE
} MAC_PIB_ATTR;
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// MAC status values
typedef UBYTE MAC_ENUM;

#define SUCCESS                 0
#define BEACON_LOSS             0xE0
#define CHANNEL_ACCESS_FAILURE  0xE1
#define DENIED                  0xE2
#define DISABLE_TRX_FAILURE     0xE3
#define FAILED_SECURITY_CHECK   0xE4
#define FRAME_TOO_LONG          0xE5
#define INVALID_GTS             0xE6
#define INVALID_HANDLE          0xE7
#define INVALID_PARAMETER       0xE8
#define NO_ACK                  0xE9
#define NO_BEACON               0xEA
#define NO_DATA                 0xEB
#define NO_SHORT_ADDRESS        0xEC
#define OUT_OF_CAP              0xED
#define PAN_ID_CONFLICT         0xEE
#define REALIGNMENT             0xEF
#define TRANSACTION_EXPIRED     0xF0
#define TRANSACTION_OVERFLOW    0xF1
#define TX_ACTIVE               0xF2
#define UNAVAILABLE_KEY         0xF3
#define UNSUPPORTED_ATTRIBUTE   0xF4
#define RX_DEFERRED             0xF5

// Association status
#define PAN_AT_CAPACITY 1
#define PAN_ACCESS_DENIED 2
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// Data structures used with the MAC primitives
typedef union {
    UHWORD  Short;
    QWORD Extended;
} ADDRESS;

// DO NOT CHANGE THE ORDER INSIDE THIS STRUCTURE!!!
typedef struct {
    UBYTE    srcAddrMode;
    UHWORD    srcPanId;
    ADDRESS srcAddr;
    UBYTE    dstAddrMode;
    UHWORD    dstPanId;
    ADDRESS dstAddr;
    UINT8   mpduLinkQuality;
    BOOL    securityUse;
    UINT8   aclEntry;
    INT8    msduLength;
    UBYTE    pMsdu[aMaxMACFrameSize];

	// DIMITRIOS ADDITION FOR RSSI SUPPORT
	UBYTE RSSI_VALUE;
	uint32_t systime;
} MCPS_DATA_INDICATION;

// DO NOT CHANGE THE ORDER INSIDE THIS STRUCTURE!!!
typedef struct {                        // Overlap when shoehorned into MAC_RX_PACKET
    BOOL securityFailure;               // MAC_RX_PACKET.securityStatus
    DWORD timeStamp;                    // MAC_RX_PACKET.timeStamp -> OK
    UINT8 coordAddrMode;                // MCPS_DATA_INDICATION.srcAddrMode -> OK
    UHWORD coordPanId;                    // MCPS_DATA_INDICATION.srcPanId -> OK
    ADDRESS coordAddress;               // MCPS_DATA_INDICATION.srcAddr -> OK
    BOOL securityUse;                   // MCPS_DATA_INDICATION.dstAddrMode
    UHWORD superframeSpec;                // MCPS_DATA_INDICATION.dstPanId
    BOOL gtsPermit;                     // MCPS_DATA_INDICATION.dstAddr
    UINT8 linkQuality;                  // MCPS_DATA_INDICATION.dstAddr
    UINT8 aclEntry;                     // MCPS_DATA_INDICATION.dstAddr
    UINT8 logicalChannel;               // MCPS_DATA_INDICATION.dstAddr
} PAN_DESCRIPTOR;

// DO NOT CHANGE THE ORDER INSIDE THIS STRUCTURE!!!
typedef struct {                        // Overlap when shoehorned into MAC_RX_PACKET
    UINT8 bsn;                          // MAC_RX_PACKET.sequenceNumber -> OK
    PAN_DESCRIPTOR panDescriptor;       // See above...
    UBYTE pendAddrSpec;                  // MCPS_DATA_INDICATION.dstAddr
    UINT8 sduLength;                    // MCPS_DATA_INDICATION.dstAddr
    ADDRESS pAddrList[7];               // MCPS_DATA_INDICATION.dstAddr to pMsdu
    UBYTE pSdu[aMaxBeaconPayloadLength]; // MCPS_DATA_INDICATION.pMsdu
} MLME_BEACON_NOTIFY_INDICATION;

// The results returned by mlmeScanRequest (slightly different from MLME_SCAN_CONFIRM used by version 0.6.2
// and earlier)
typedef struct {
    UBYTE scanType;
    DWORD unscannedChannels;
    UINT8 resultListSize;
    union{
        UINT8 pEnergyDetectList[16];
        PAN_DESCRIPTOR pPANDescriptorList[MAC_OPT_MAX_PAN_DESCRIPTORS];
    };
} MAC_SCAN_RESULT;
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// Address modes - for use with mcpsDataRequest only (all other address mode parameters shall use
// AM_SHORT_16 or AM_EXTENDED_64)
#define SRC_ADDR_BM             0xC0
#define SRC_ADDR_NONE           0x00
#define SRC_ADDR_SHORT          0x80
#define SRC_ADDR_EXT            0xC0

#define DEST_ADDR_BM            0x0C
#define DEST_ADDR_NONE          0x00
#define DEST_ADDR_EXT           0x0C
#define DEST_ADDR_SHORT         0x08

// Address modes:
#define AM_NONE                 0
#define AM_SHORT_16             2
#define AM_EXTENDED_64          3

// Internal
#define BOTH_ADDR_USED          0x88
//----------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------
// TX options - for use with mcpsDataRequest only
#define TX_OPT_SECURITY_ENABLE 0x08
#define TX_OPT_INDIRECT        0x04
#define TX_OPT_GTS             0x02
#define TX_OPT_ACK_REQ         0x01
#define TX_OPT_NONE            0x00
//----------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               MCPS Function Prototypes            **************************
 *******************************************************************************************************
 *******************************************************************************************************/




//-------------------------------------------------------------------------------------------------------
//  void mcpsDataRequest(BYTE addrModes, WORD srcPanId, ADDRESS *pSrcAddr, WORD destPanId, ADDRESS ...)
//
//  DESCRIPTION:
//      Transmit a data packet.
//
//  PARAMETERS:
//      BYTE addrModes
//          Address mode for source and destination
//          (SRC_ADDR_SHORT, SRC_ADDR_EXT or 0) | (DEST_ADDR_SHORT, DEST_ADDR_EXT or 0)
//      WORD srcPanId
//          Source PAN identifier
//      ADDRESS *pSrcAddr
//          Pointer to the source address (short or extended)
//      WORD destPanId,
//          Destination PAN identifier
//      ADDRESS *pDestAddr,
//          Pointer to the destination address (short or extended)
//      UINT8 msduLength,
//          The length of pMsdu[]
//      BYTE *pMsdu,
//          A pointer to the packet payload
//      BYTE msduHandle,
//          A handle to this packet which is used later on with mcpsPurgeRequest() and mcpsDataConfirm()
//      BYTE txOptions
//          (TX_OPT_SECURITY_ENABLE | TX_OPT_INDIRECT | TX_OPT_GTS | TX_OPT_ACK_REQ) or TX_OPT_NONE
//          Note: Indirect transmission only available for MAC_OPT_FFD=1
//-------------------------------------------------------------------------------------------------------
void mcpsDataRequest(UBYTE addrModes, UHWORD srcPanId, ADDRESS *pSrcAddr, UHWORD destPanId, ADDRESS *pDestAddr, UINT8 msduLength, UBYTE *pMsdu, UBYTE msduHandle, UBYTE txOptions);


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
void mcpsDataConfirm(MAC_ENUM status, UBYTE msduHandle);




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
void mcpsDataIndication(MCPS_DATA_INDICATION *pMDI);




//-------------------------------------------------------------------------------------------------------
//  MAC_ENUM mcpsPurgeRequest(BYTE msduHandle)
//
//  DESCRIPTION:
//      Purge data frames from the indirect data transmission queue
//
//  PARAMETERS:
//      BYTE msduHandle
//          The packet handle (from mcpsDataRequest(...))
//
//  RETURN VALUE:
//      MAC_ENUM
//          SUCCESS: OK
//          INVALID_HANDLE: The packet could not be found (already transmitted?)
//-------------------------------------------------------------------------------------------------------
MAC_ENUM mcpsPurgeRequest(UBYTE msduHandle);




//----------------------------------------------------------------------------------------------------------
// MLME prototypes


//-------------------------------------------------------------------------------------------------------
//  void mlmeAssociateRequest(UINT8 logicalChannel, BYTE coordAddrMode, WORD coordPANId, ...)
//
//  DESCRIPTION:
//      Generates an association request command frame, transmitted to the coordinator using direct
//      transmission. The response is polled automatically from the coordinator.
//      NOTE: Please note that the PAN ID and the coordinator address (short or extended) are set by the
//      the MAC layer, according to the IEEE 802.15.4 spec.
//
//  PARAMETERS:
//      UINT8 logicalChannel
//          Channel number (0x0B - 0x1A)
//      BYTE coordAddrMode
//          AM_SHORT_16 or AM_EXTENDED_64
//      WORD coordPANId
//          The coordinator PAN identifier
//      ADDRESS *pCoordAddress
//          Pointer to the short or extended address of the coordinator
//      BYTE capabilityInformation
//          (CI_ALTERNATE_PAN_COORD_BM | CI_DEVICE_TYPE_IS_FFD_BM | CI_POWER_SOURCE_BM |
//           CI_RX_ON_WHEN_IDLE_BM | CI_SECURITY_CAPABILITY_BM | CI_ALLOCATE_ADDRESS_BM)
//      BOOL securityEnable
//          Security is enabled?
//-------------------------------------------------------------------------------------------------------
void mlmeAssociateRequest(UINT8 logicalChannel, UBYTE coordAddrMode, UHWORD coordPANId, ADDRESS *pCoordAddress, UBYTE capabilityInformation, BOOL securityEnable);




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
void mlmeAssociateIndication(ADDRESS deviceAddress, UBYTE capabilityInformation, BOOL securityUse, UINT8 aclEntry);




//-------------------------------------------------------------------------------------------------------
//  void mlmeAssociateResponse(ADDRESS *pDeviceAddress, WORD assocShortAddress, MAC_ENUM status, ...
//
//  DESCRIPTION:
//      Used by a ccordinator to respond to an association indication. The response is placed in the
//      indirect transmission queue.
//      Generates a mlmeCommStatusIndication callback upon completion
//
//  PARAMETERS:
//      ADDRESS *pDeviceAddress
//          Pointer to the extended address of the associated device
//      WORD assocShortAddress
//          The assigned short address
//      MAC_ENUM status
//          The association status
//      BOOL securityEnable
//          Security is enabled?
//-------------------------------------------------------------------------------------------------------
void mlmeAssociateResponse(ADDRESS *deviceAddress, UHWORD assocShortAddress, MAC_ENUM status, BOOL securityEnable);




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
void mlmeAssociateConfirm(UHWORD AssocShortAddress, MAC_ENUM status);




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
void mlmeBeaconNotifyIndication(MLME_BEACON_NOTIFY_INDICATION *pMBNI);




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
void mlmeCommStatusIndication(UHWORD panId, UBYTE srcAddrMode, ADDRESS *pSrcAddr, UBYTE dstAddrMode, ADDRESS *pDstAddr, MAC_ENUM status);




//-------------------------------------------------------------------------------------------------------
//  void mlmeDisassociateRequest(QWORD *pDeviceAddress, BYTE disassociateReason, BOOL securityEnable)
//
//  DESCRIPTION:
//      Used by an associated device to notify the coordinator of its intent to leave the PAN or
//      used by the coordinator to instruct an associated device to leave the PAN. pDeviceAddress is a
//      pointer to the extended address of the device to which to send the disassociation notification
//      command.
//
//  PARAMETERS:
//      QWORD *pDeviceAddress
//          For coordinators: A pointer to the extended address of the device to disassociate
//          For devices: A pointer to the extended address of coordinator
//      BYTE disassociateReason
//          The disassociate reason (COORD_WISHES_DEVICE_TO_LEAVE | DEVICE_WISHES_TO_LEAVE)
//      BOOL securityEnable
//          Security is enabled?
//-------------------------------------------------------------------------------------------------------
void mlmeDisassociateRequest(QWORD *pDeviceAddress, UBYTE disassociateReason, BOOL securityEnable);




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
void mlmeDisassociateIndication(QWORD deviceAddress, UBYTE disassociateReason, BOOL securityUse, UINT8 aclEntry);




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
void mlmeDisassociateConfirm(MAC_ENUM status);




//-------------------------------------------------------------------------------------------------------
//  MAC_ENUM mlmeGetRequest(MAC_PIB_ATTR pibAttribute, void *pPibAttributeValue)
//
//  DESCRIPTION:
//      Get MAC PIB attributes. The value is copied to the location pointed to by the void*. Note that
//      some values are returned as pointers:
//          - pMacBeaconPayload
//          - pMacACLEntryDescriptorSet
//          - pMacDefaultSecurityMaterial
//
//  PARAMETERS:
//      MAC_PIB_ATTR pibAttribute
//          The attribute to be changed
//      void *pPibAttributeValue
//          A pointer to the PIB attribute. Note that this data is _copied_ into the PIB.
//
//  RETURN VALUE:
//      MAC_ENUM
//          SUCCESS or UNSUPPORTED_ATTRIBUTE
//-------------------------------------------------------------------------------------------------------
MAC_ENUM mlmeGetRequest(MAC_PIB_ATTR pibAttribute, void *pPibAttributeValue);




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
void mlmeOrphanIndication(QWORD orphanAddress, BOOL securityUse, UINT8 aclEntry);




//-------------------------------------------------------------------------------------------------------
//  void mlmeOrphanResponse(QWORD orphanAddress, WORD shortAddress, BOOL associatedMember, BOOL ...)
//
//  DESCRIPTION:
//      Respond to an orphan notification by transmitting a coordinator realignment frame.
//
//  PARAMETERS:
//      QWORD orphanAddress
//          Extended address of the orphaned device
//      WORD shortAddress
//          The short address of the coordinator
//      BOOL associatedMember
//          This node is associated on this PAN
//          Note: mlmeOrphanResponse is ignored if set to FALSE
//      BOOL securityEnable
//          Security is enabled for the coordinator realignment command frame?
//-------------------------------------------------------------------------------------------------------
void mlmeOrphanResponse(QWORD orphanAddress, UHWORD shortAddress, BOOL associatedMember, BOOL securityEnable);




//-------------------------------------------------------------------------------------------------------
//  void mlmePollRequest(BYTE coordAddrMode, WORD coordPANId, ADDRESS *pCoordAddress, BOOL ...)
//
//  DESCRIPTION:
//      Poll indirect data from the coordinator
//
//  PARAMETERS:
//      BYTE coordAddrMode
//          The coordinator address mode (AM_SHORT_16 or AM_EXTENDED_64)
//      WORD coordPANId
//          The PAN identifier of the coordinator
//      ADDRESS *pCoordAddress
//          A pointer to the coordinator address (short or extended)
//      BOOL securityEnable
//          Enable security for data-request command frame?
//-------------------------------------------------------------------------------------------------------
void mlmePollRequest(UBYTE coordAddrMode, UHWORD coordPANId, ADDRESS *coordAddress, BOOL securityEnable);




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
void mlmePollConfirm(MAC_ENUM status);




//-------------------------------------------------------------------------------------------------------
//  MAC_ENUM mlmeResetRequest(BOOL setDefaultPIB)
//
//  DESCRIPTION:
//      Reset the MAC and PHY layers, including CC2420, the microcontroller, and all state variables.
//      NOTE: The initialization and power-up sequence must be performed according to
//            the MAC documentation prior to calling mlmeResetRequest or other
//            MAC primitives
//
//  PARAMETERS:
//      BOOL setDefaultPIB
//          Reset the PHY and MAC PIBs
//
//  RETURN VALUE:
//      MAC_ENUM
//          Always SUCCESS
//-------------------------------------------------------------------------------------------------------
MAC_ENUM mlmeResetRequest(BOOL setDefaultPIB);




//-------------------------------------------------------------------------------------------------------
//  void mlmeRxEnableRequest(BOOL deferPermit, UINT32 rxOnTime, UINT32 rxOnDuration)
//
//  DESCRIPTION:
//      Enable the receiver for after a given timeout (in symbols), and turn it off after the given
//      duration (also in symbols). An rxOnDuration = 0 will immediately shut down the receiver.
//      Note: Do NOT use on beacon networks, set RX_ON_WHEN_IDLE to TRUE in stead
//
//  PARAMETERS:
//      BOOL deferPermit
//          Reception can be deferred until the next superframe
//      UINT32 rxOnTime
//          The number of symbols to elapse before the receiver should be turned on
//      UINT32 rxOnDuration
//          The number of symbols to listen before turning the receiver off
//-------------------------------------------------------------------------------------------------------
void mlmeRxEnableRequest(BOOL deferPermit, UINT32 rxOnTime, UINT32 rxOnDuration);




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
void mlmeRxEnableConfirm(MAC_ENUM status);




//-------------------------------------------------------------------------------------------------------
//  MAC_ENUM mlmeScanRequest(BYTE scanType, DWORD scanChannels, UINT8 scanDuration)
//
//  DESCRIPTION:
//      Scan through the selected channels (energy, active, passive and orphan scanning supported).
//      Important:
//          - The maximum number of results returned for active and passive scans is
//            defined by the MAC_OPT_MAX_PAN_DESCRIPTORS (>= 1) mac option
//          - This function will not exit before the scan is completed.
//
//  PARAMETERS:
//      BYTE scanType
//          ENERGY_SCAN, ACTIVE_SCAN, PASSIVE_SCAN or ORPHAN_SCAN
//      DWORD scanChannels
//          The channel index mask (0x07FFF800 are the legal values for 2.4 GHz channels)
//      UINT8 scanDuration
//          The scan duration defines the time spent scanning each channel, defined as:
//              (aBaseSuperframeDuration * (2 ^^ scanDuration + 1)) symbol periods
//              = (60 * 16 * (2^^scanDuration+1)) symbol periods
//          E.g., scanning all 16 channels with Scanduration 5 takes 8.11 seconds
//      MAC_SCAN_RESULT *pScanResult
//          The pointer to the MAC_SCAN_RESULT struct (defined by the higher layer) where
//          the MAC sublayer shall store the scan result.
//
//  RETURN VALUE:
//      MAC_ENUM
//          INVALID_PARAMETER, SUCCESS or NO_BEACON
//-------------------------------------------------------------------------------------------------------
MAC_ENUM mlmeScanRequest(UBYTE scanType, DWORD scanChannels, UINT8 scanDuration, MAC_SCAN_RESULT *pScanResult);




//-------------------------------------------------------------------------------------------------------
//  MAC_ENUM mlmeSetRequest(MAC_PIB_ATTR pibAttribute, void *pPibAttributeValue)
//
//  DESCRIPTION:
//      Set MAC PIB attribute.
//
//  PARAMETERS:
//      MAC_PIB_ATTR pibAttribute
//          The attribute to be changed
//      void *pPibAttributeValue
//          A pointer to the PIB attribute. Note that this data is _copied_ into the PIB.
//
//  RETURN VALUE:
//      MAC_ENUM
//          INVALID_PARAMETER, SUCCESS or UNSUPPORTED_ATTRIBUTE
//-------------------------------------------------------------------------------------------------------
MAC_ENUM mlmeSetRequest(MAC_PIB_ATTR pibAttribute, void *pPibAttributeValue);




//-------------------------------------------------------------------------------------------------------
//  MAC_ENUM mlmeStartRequest(WORD panId, UINT8 logicalChannel, UINT8 beaconOrder, UINT8 ...)
//
//  DESCRIPTION:
//      As a coordinator: Start or stop transmitting beacons.
//
//  PARAMETERS:
//      WORD panId
//          The new PAN identifier
//      UINT8 logicalChannel
//          The channel to operate on (11-26)
//      UINT8 beaconOrder
//          The beacon order, which defines the beacon interval (0-14 for beacon PAN, 15 for non-beacon PAN)
//      UINT8 superframeOrder
//          The superframe order, which defines the superframe duration (that is the active period of the
//          beacon interval). superframeOrder must be <= beaconOrder
//      BOOL panCoordinator
//          TRUE if this node should be the PAN coordinator
//      BOOL batteryLifeExtension
//          Enable battery life extension
//      BOOL coordRealignment
//          Transmit a coordinator realignment frame before making the changes
//      BOOL securityEnable
//          Security is enabled?
//
//  RETURN VALUE:
//      MAC_ENUM
//          SUCCESS, NO_SHORT_ADDRESS or INVALID_PARAMETER
//-------------------------------------------------------------------------------------------------------
MAC_ENUM mlmeStartRequest(UHWORD panId, UINT8 logicalChannel, UINT8 beaconOrder, UINT8 superframeOrder, BOOL panCoordinator, BOOL batteryLifeExtension, BOOL coordRealignment, BOOL securityEnable);




//-------------------------------------------------------------------------------------------------------
//  void mlmeSyncRequest(UINT8 logicalChannel, BOOL trackBeacon)
//
//  DESCRIPTION:
//      Switch to the selected channel, locate a single beacon, and start or stop tracking beacons
//      (optional).
//
//  PARAMETERS:
//      UINT8 logicalChannel
//          The channel to switch to.
//      BOOL trackBeacon
//          Track beacons if >0.
//-------------------------------------------------------------------------------------------------------
void mlmeSyncRequest(UINT8 logicalChannel, BOOL trackBeacon);




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
void mlmeSyncLossIndication(MAC_ENUM lossReason);




//----------------------------------------------------------------------------------------------------------

#endif


/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac.h,v $
 * Revision 1.2  2005/11/07 22:13:21  abs
 * add timestamping to the mac layer.
 * add systime.c and call systime_init in hardware.c/hardware_init.
 * add timestamps to the radio.c.
 *
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
 * Revision 1.5  2004/11/22 21:57:35  simonhan
 * update xyz
 *
 * Revision 1.4  2004/10/29 03:11:30  asavvide
 * *** empty log message ***
 *
 * Revision 1.3  2004/10/27 03:49:25  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.11  2004/08/13 13:04:42  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
