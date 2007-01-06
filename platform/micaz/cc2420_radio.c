/**
 * @brief Port of the Chipcon's IEEE 802.15.4 MAC to SOS on MicaZ
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

/**
 * Description:
 * 1. SOS by default will operate in the NON-BEACONED mode.
 *    - TODO: Enable the MAC to be configurable to operate in the other modes as well
 * 2. Address Allocation
 *    - Every node is hard-coded with a unique 16-bit node ID
 *    - This unique ID is added to 0x0000004722950000 to set the variable aExtendedAddress variable
 *      The standard mandates the value of this variable to be unique throughout the network
 * 3. Network StartUp
 *    - Every node performs an active scan on startup to look out for any active PANs
 *    - If a PAN is found, then the node associates to the PAN co-ordinator and is assigned a network ID which is identical to the node ID.
 *    - If a PAN is not found, then the node becomes a PAN co-ordinator and starts a Non-Beaconed PAN
 * 4. Association
 *    - Currently, the first node to boot becomes the PAN coordinator and all the nodes can associate only to it 
 *    - We have to wait for the PAN coordinator to boot up before we start the rest of the nodes
 * 5. Misc. Issues (VERY IMPORTANT !!)
 *    - The MAC code is full of spin-locks while transmitting a packet. Therefore upon send done, a message to self is posted
 *      to service the next packet.
 *    - The MAC copies the Transmit Buffer entirely through memcpy
 *    - Some of the important properties of the MAC can be configured using the /dev/micaz/include/mac/mac_setup.h file
 */
/**
 * RAM - IMPORTANT NOTE - DATE - NOV 14th, 2004
 * I am trying out if every node can be a PAN Coordinator. Therefore,
 * 1. NodePanId variable is set to ker_id();
 * 2. There is no active scan being performed
 * 3. All the nodes start a non-beaconed PAN as PAN co-ordinators
 * 4. All the data transmissions now require to specify the destination PAN id which will be same as the destination node id
 */

#include <hardware.h>

#include "include.h" // insightfully named include for cc2420
#include "cc2420_radio.h"

#include <net_stack.h>
#include <timestamp.h>
#include <systime.h>

//#define LED_DEBUG
#include <led_dbg.h>

//-------------------------------------------------------------------------------------------------------
// Constant Definitions
//#define PANID 0xDEAD
//#define CHANNEL 18
enum
  {
    NON_BEACONED_PAN_COORDINATOR = 0,
    NON_BEACONED_REGULAR = 1
  };
//-------------------------------------------------------------------------------------------------------
// Application buffers
//BYTE pBuffer[256];
//MCPS_DATA_INDICATION dataIndication;
MLME_BEACON_NOTIFY_INDICATION beaconNotifyIndication;

//-------------------------------------------------------------------------------------------------------
// Global Variables
ADDRESS coordExtAddr;
MAC_ENUM status;
PAN_DESCRIPTOR* pPanDescriptor;
ADDRESS coordAddr;


//-------------------------------------------------------------------------------------------------------
// SOS Variables
volatile WORD myShortAddr;
volatile WORD myBcastAddr = BCAST_ADDRESS;
UINT8 SOSPanChannel;
WORD NodePanId;
static mq_t pq;
static BOOL bTxBusy;
static Message* TxMsgPtr;
static uint8_t* txbuff;

static bool bTsEnable = false;
extern UINT32 timestamp;

//MCPS_DATA_INDICATION *pRecvMDI; 
//-------------------------------------------------------------------------------------------------------
// Function Prototypes
//static int8_t radio_active_scan();
static int8_t radio_non_beacon_PAN_init(uint8_t mode);
//static int8_t radio_beacon_PAN_init();
static void WaitFor(uint8_t time);
int8_t cc2420_radio_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER ={
mod_id : RADIO_PID,
state_size : 0,
num_sub_func : 0,
num_prov_func : 0,
module_handler: cc2420_radio_handler,
};

static sos_module_t cc2420_module;
//-------------------------------------------------------------------------------------------------------
// Convert Message to Buffer
static inline uint8_t* msg_to_buff(Message* m)
{
  uint8_t* buff;
  buff = (uint8_t*)ker_malloc(m->len + SOS_MSG_HEADER_SIZE + SOS_MSG_PRE_HEADER_SIZE, RADIO_PID);
  if (buff == NULL)
    return NULL;
  buff[0] = node_group_id;
  memcpy(buff + SOS_MSG_PRE_HEADER_SIZE, m, SOS_MSG_HEADER_SIZE);
  memcpy(buff + SOS_MSG_PRE_HEADER_SIZE + SOS_MSG_HEADER_SIZE, m->data, m->len);
  return buff;
}


//-------------------------------------------------------------------------------------------------------
// Convert Buffer to Message
static inline Message* buff_to_msg(uint8_t* buff)
{
  Message* m;
  Message* tmpMsg;

  // Filtering based on group ID
  if (buff[0] != node_group_id)
    return NULL;

  // Create and copy the message header
  m = msg_create();
  if (m == NULL)
    return NULL;
  memcpy( m, buff + SOS_MSG_PRE_HEADER_SIZE, SOS_MSG_HEADER_SIZE);

  tmpMsg = (Message*) (buff + SOS_MSG_PRE_HEADER_SIZE);
  // Check if the message has empty payload
  if (tmpMsg->len == 0)
    return m;

  // Copy the message payload
  m->data = ker_malloc(tmpMsg->len, RADIO_PID);
  if (m->data == NULL){
    msg_dispose(m);
    return NULL;
  }
  memcpy(m->data, buff + SOS_MSG_PRE_HEADER_SIZE + SOS_MSG_HEADER_SIZE, tmpMsg->len);
  m->flag = SOS_MSG_RELEASE;
  return m;
}
//-------------------------------------------------------------------------------------------------------
// Transmit a Packet
static inline void cc2420_tx_pkt()
{
  // When we call this, we are sure that TxMsgPtr != NULL and bTxBusy = TRUE
  txbuff = msg_to_buff(TxMsgPtr);

  if (txbuff == NULL){
    // We have no memory to transfer this packet so we generate a failed send done
	 if(bTsEnable) {
		  timestamp_outgoing(TxMsgPtr, ker_systime32());
	  }

    msg_send_senddone(TxMsgPtr, false, RADIO_PID);
    // Check if we have more packets in the send queue
    TxMsgPtr = mq_dequeue(&pq);
    if (TxMsgPtr)
      post_short(RADIO_PID, RADIO_PID, MSG_RADIO_PKT_SEND, 0, 0, 0);    
    else
      bTxBusy = FALSE;
  }
  
  else{
    mcpsDataRequest(SRC_ADDR_SHORT | DEST_ADDR_SHORT,      //! Addressing Modes
		    NodePanId,                              //! Source PAN ID
		    (ADDRESS*) &myShortAddr,               //! Source Address
		    //		    NodePanId,                              //! Destination PAN ID
		    //! Ram:-Trying out if every node can have its own PAN
		    TxMsgPtr->daddr,                       //! Destination PAN ID
		    (ADDRESS*) &(TxMsgPtr->daddr),         //! Destination Address
		    //			BCAST_ADDRESS,
		    //			(ADDRESS*) &(myBcastAddr),
		    SOS_MSG_PRE_HEADER_SIZE + SOS_MSG_HEADER_SIZE + TxMsgPtr->len,   //! Length of MSDU
		    (BYTE*) txbuff,                        //! Pointer to MSDU
		    7,                                     //! Handle for the Packet
		    TX_OPT_NONE);                          //! Transmit Options
  }
}

//-------------------------------------------------------------------------------------------------------
// Receive a Packet
static inline void cc2420_rx_pkt(MCPS_DATA_INDICATION* pRecvMDI)
{
  Message* RxMsg;
  RxMsg = buff_to_msg(pRecvMDI->pMsdu);
  // The SOS_MSG_RELEASE is set. Kernel will handle it when it receives SOS_OK;
  //  ker_free(pRecvMDI);
  if (RxMsg != NULL)
  {
	if (RxMsg->type == MSG_TIMESTAMP){
	memcpy(((uint8_t*)(RxMsg->data) + 4),(uint8_t*)(&timestamp),4);
	}
    if(bTsEnable) {
      timestamp_incoming(RxMsg, ker_systime32());
    }
    
    handle_incoming_msg(RxMsg, SOS_MSG_RADIO_IO);
  }
}


//-------------------------------------------------------------------------------------------------------
//	mlme indication-functions
//
//	DESCRIPTION:
//      mlme indication-functions are generated by the MAC layer.
//      These are application specific, and the ones included here are written for this demo program
//      only.
//
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
//	void mlmeAssociateIndication(ADDRESS deviceAddress, BYTE capabilityInformation, BOOL securityUse, BOOL ACLEntry)
//
//	DESCRIPTION:
//      mlmeAssociateIndication is generated by the MAC layer upon reception of an associate request
//      command frame.
//
//  PARAMETERS:
//      MAC_ENUM status
//          The data confirm status.
//      BYTE msduHandle
//          The handle of the confirmed frame
//
//-------------------------------------------------------------------------------------------------------
void mlmeAssociateIndication(ADDRESS deviceAddress, BYTE capabilityInformation, BOOL securityUse, BOOL ACLEntry) {
  WORD temp_associated_address;
  temp_associated_address = (WORD)(deviceAddress.Extended);
  mlmeAssociateResponse(&deviceAddress, temp_associated_address, 0x00, FALSE);
} // mlmeAssociateIndication

//-------------------------------------------------------------------------------------------------------
//	void mlmeBeaconNotifyIndication(void)
//
//	DESCRIPTION:
//      mlmeBeaconNotifyIndication is generated by the MAC layer upon reception of a beacon when
//          - sduLength (beacon payload) > 0
//          - MAC PIB -> macAutoRequest is FALSE
//
//      The parameters associated with this primitive can be accessed through the pBeaconNotifyIndication
//      pointer (which must be set at startup for beacon-enabled PANs).
//-------------------------------------------------------------------------------------------------------
void mlmeBeaconNotifyIndication(void) {}

//-------------------------------------------------------------------------------------------------------
//	mlmeDisassociateIndication(QWORD deviceAddress, BYTE disassociateReason, BOOL securityUse, BOOL ACLEntry)
//
//	DESCRIPTION:
//      mlmeDisassociateIndication is generated by the MAC layer upon reception of an disassociate request
//      command frame. Not used in this demo application.
//
//  PARAMETERS:
//      QWORD deviceAddress
//          The extended address of the device wishing to disassociate.
//      BYTE disassociateReason
//          The disassociate reason (device wishes to leave, coord. wishes the device to leave).
//      BOOL securityUse
//          Security is enabled?
//      BOOL ACLEntry
//          MAC security mode of the sending device, 0x08 if the sender was not found in the ACL
//
//-------------------------------------------------------------------------------------------------------
void mlmeDisassociateIndication(QWORD deviceAddress, BYTE disassociateReason, BOOL securityUse, BOOL ACLEntry) {}

//-------------------------------------------------------------------------------------------------------
//	mlmeCommStatusIndication(WORD panId, BYTE srcAddrMode, ADDRESS *pSrcAddr, BYTE dstAddrMode, ADDRESS *pDstAddr, BYTE status)
//
//	DESCRIPTION:
//      mlmeCommStatusIndication is generated by the MAC layer when indicating communication status
//      Not yet implemented throughout the MAC
//
//  PARAMETERS:
//      WORD panId
//          The 16 bit PAN identifier of the device from which the frame was received or to
//          which the frame was being sent.
//      BYTE srcAddrMode
//          The source addressing mode for this primitive.
//      ADDRESS *pSrcAddr
//          The individual device address of the entity from which the frame causing the
//          error originated.
//      BYTE dstAddrMode
//          The destination addressing mode for this primitive.
//      ADDRESS *pDstAddr
//          The individual device address of the device for which the frame was intended.
//      BYTE status
//          The communications status.
//      
//
//-------------------------------------------------------------------------------------------------------
void mlmeCommStatusIndication(WORD panId, BYTE srcAddrMode, ADDRESS *pSrcAddr, BYTE dstAddrMode, ADDRESS *pDstAddr, BYTE status) {}

//-------------------------------------------------------------------------------------------------------
//	void mlmeSyncLossIndication(MAC_ENUM lossReason)
//
//	DESCRIPTION:
//      mlmeSyncLossIndication is generated by the MAC layer when indicating the loss of synchronization 
//      with a coordinator. Set the LEDs to indicate reason
//
//  PARAMETERS:
//      MAC_ENUM lossReason
//          The reason for the synchronisation loss.
//          (PAN_ID_CONFLICT | REALIGNMENT | BEACON_LOST)
//
//-------------------------------------------------------------------------------------------------------
void mlmeSyncLossIndication(MAC_ENUM lossReason) {}

//-------------------------------------------------------------------------------------------------------
//	void mlmeOrphanIndication(QWORD orphanAddress, BOOL securityUse, BOOL aclEntry)
//
//	DESCRIPTION:
//      mlmeOrphanIndication is generated by the MAC layer when an orphaning device is detected.
//      Not used by this demo application
//
//  PARAMETERS:
//      QWORD orphanAddress
//          The extended address of the orphaning device
//      BOOL securityUse
//          Security enabled?
//      BOOL aclEntry
//          MAC security mode of the sending device, 0x08 if the sender was not found in the ACL
//-------------------------------------------------------------------------------------------------------
void mlmeOrphanIndication(QWORD orphanAddress, BOOL securityUse, BOOL aclEntry) {}

//-------------------------------------------------------------------------------------------------------
//	void mcpsDataIndication(void)
//
//	DESCRIPTION:
//      mcpsDataIndication is generated by the MAC layer when a data frame is successfully received.
//      If higher layers need time to process data, multi-buffering may be implemented by changing 
//      the rxInfo.pMDI pointer inside the mcpsDataIndication function.                                    
//      In this demo application, this is used to set the LEDs equal to the first data byte received.
//
//  PARAMETERS:
//      None (parameters are returned through the rxInfo.pMDI pointer, set up by the layer above the MAC)
//-------------------------------------------------------------------------------------------------------
void mcpsDataIndication(void) {
  MCPS_DATA_INDICATION* pRecvMDI = ker_malloc(128, RADIO_PID);
  if (pRecvMDI != NULL){
    post_long(RADIO_PID, RADIO_PID, MSG_RADIO_PKT_RECV, 128, rxInfo.pMDI, SOS_MSG_RELEASE);      
    rxInfo.pMDI = pRecvMDI;
  }
} // mcpsDataIndication

//-------------------------------------------------------------------------------------------------------
//	mlme confirm-functions                               
//
//	DESCRIPTION:
//      mlme confirm-functions are called by the MAC layer as a response to request function calls.
//      Note that some confirms are generated directly by the MAC layer as a return value in the
//      request function. The confirm functions are highly application specific, and the ones
//      included here are used for this demo application only.
//
//-------------------------------------------------------------------------------------------------------




//-------------------------------------------------------------------------------------------------------
//	void mcpsDataConfirm(MAC_ENUM status, BYTE msduHandle)
//
//	DESCRIPTION:
//      mcpsDataConfirm is called by the MAC layer when the mcpsDataRequest has been executed.
//      This example program ligths up the led number read from the transmitted MSDU (leds 0 through
//      7 are in the PAN coordinator, leds 8 through 15 are in the associated device) if the
//      transmission was successful. Otherwise, the returned status is displayed on the LEDs.
//
//  PARAMETERS:
//      MAC_ENUM status
//          The data confirm status.
//      BYTE msduHandle
//          The handle of the confirmed frame
//      
//-------------------------------------------------------------------------------------------------------
void mcpsDataConfirm(MAC_ENUM status, BYTE msduHandle) {
  ker_free(txbuff);
  switch (status) {
  case SUCCESS:
    {
	    if(bTsEnable) {
		    timestamp_outgoing(TxMsgPtr, ker_systime32());
	    }
      msg_send_senddone(TxMsgPtr, true, RADIO_PID);
      break;
    }
  default: 
    {
	    if(bTsEnable) {
		    timestamp_outgoing(TxMsgPtr, ker_systime32());
	    }
      msg_send_senddone(TxMsgPtr, false, RADIO_PID);
      break;
    }
  }
  TxMsgPtr = mq_dequeue(&pq);
  if (TxMsgPtr)
    post_short(RADIO_PID, RADIO_PID, MSG_RADIO_PKT_SEND, 0, 0, 0);    
  else
    bTxBusy = FALSE;  
} // mcpsDataConfirm
  
//-------------------------------------------------------------------------------------------------------
//	void mlmeAssociateConfirm(WORD AssocShortAddress, MAC_ENUM status)
//
//	DESCRIPTION:
//      Update myShortAddr variable with the returned short address, if successful.
//      Set LEDs to show returned status          
//
//  PARAMETERS:
//      WORD AssocShortAddress
//          The assigned short address. This value is automatically written to the MAC_SHORT_ADDRESS PIB
//          by the MAC layer.
//      MAC_ENUM status
//          The associate confirm status.
//          
//-------------------------------------------------------------------------------------------------------
void mlmeAssociateConfirm(WORD AssocShortAddress, MAC_ENUM status) {
  //! Ram - All the lower layer stuff such as setup of the PAN ID, Coord ID, Node Address etc. are set in the stack itself
  if (status == SUCCESS)
    myShortAddr = AssocShortAddress;
} // mlmeAssociateConfirm

//-------------------------------------------------------------------------------------------------------
//	void mlmeDisassociateConfirm(MAC_ENUM status)
//
//	DESCRIPTION:
//      In this demo application, set LEDs to show returned status
//
//  PARAMETERS:
//      MAC_ENUM status
//          The disassociate confirm status.
//          
//-------------------------------------------------------------------------------------------------------
void mlmeDisassociateConfirm(MAC_ENUM status) {}

//-------------------------------------------------------------------------------------------------------
//	void mlmeRxEnableConfirm(MAC_ENUM status)
//
//	DESCRIPTION:
//      Set LEDs to show returned status
//      Currently not used in this demo application.
//
//  PARAMETERS:
//      MAC_ENUM status
//          The RX enable confirm status.
//          
//-------------------------------------------------------------------------------------------------------
void mlmeRxEnableConfirm(MAC_ENUM status) {}

//-------------------------------------------------------------------------------------------------------
//	void mlmePollConfirm(MAC_ENUM status)
//
//	DESCRIPTION:
//      Set LEDs to show returned status
//      Currently not used in this demo application.
//
//  PARAMETERS:
//      MAC_ENUM status
//          The poll confirm status.
//          
//-------------------------------------------------------------------------------------------------------
void mlmePollConfirm(MAC_ENUM status) { }


//-------------------------------------------------------------------------------------------------------
// SOS SPECIFIC FUNCTIONS
//-------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize the Radio
 */
//-------------------------------------------------------------------------------------------------------

// SOS Radio Init Steps:
// 1. First we initialize the addresses
// 2. Next we perform an active scan for any PANs in the neighborhood
// 3. If we do not find a PAN, we start one
//-------------------------------------------------------------------------------------------------------
int8_t radio_init(uint8_t radio_mode)
{
  //  BYTE temp;
  // SOS Related Initialization
  //watchdog_reset();
  mq_init(&pq);
  bTxBusy = FALSE;
  sched_register_kernel_module(&cc2420_module, sos_get_header_address(mod_header), NULL);

  // MAC Related Initialization
  myShortAddr = 0xFFFF;
  // Set the extended address of the device
  // This must be done prior to the reset request
  // the avr-gcc treats unsigned long long as a uint32_t on 8 bit archetectures
  // unsigned long long as a uint64_t on 16 bit archetectures
  // there appears to be a missmatch between the compiler used for the mac lib and avr-gcc
  //aExtendedAddress = (0x0000004722950000 + (unsigned long long)(ker_id()));
  aExtendedAddress = (0x22950000 + (unsigned long long)(ker_id()));
  
  // IMPORTANT! These pointers must point to a location held by the for incoming data!
  //rxInfo.pMDI = &dataIndication;
  rxInfo.pMDI = (MCPS_DATA_INDICATION*)ker_malloc(128, RADIO_PID);
  pBeaconNotifyIndication = &beaconNotifyIndication;

  // Ram: I am trying out a new mode where every node is a PAN co-ordinator.
  // Since active scan is no longer being performed, every node will become the co-ordinator and start a NON_BEACONED pan
  myShortAddr = ker_id();
  if (radio_non_beacon_PAN_init(NON_BEACONED_PAN_COORDINATOR) != SOS_OK){
    while(1){
      // PANIC SIGNAL !!
      //! Note that if the watchdog is enabled, we might not always see this pattern
      //! To ensure that this is the reason for failure, disable watchdog and then re-compile and re-install a new image to see this pattern
      LED_DBG(LED_RED_ON);
      LED_DBG(LED_YELLOW_ON);
      LED_DBG(LED_GREEN_ON);
      WaitFor(20);
      LED_DBG(LED_RED_OFF);
      LED_DBG(LED_YELLOW_OFF);
      LED_DBG(LED_GREEN_OFF);
      WaitFor(20);
    }
  }
  return SOS_OK;

  // Temporarily removed the radio active scan
  // Ram: LONG BLOCKING FUNCTION !!
  /*  LED_DBG(LED_YELLOW_ON); */
  /*   radio_active_scan(); */
  /*  LED_DBG(LED_YELLOW_OFF); */
  
  
  /*   // Ram: We assume that the only reason the Scan can fail is due to the absence of any other PAN in POS   */
  /*   if (myShortAddr == 0xFFFF){ */
  /*     myShortAddr = ker_id(); */
  /*     if (radio_mode == NON_BEACONED_PAN){ */
  /*       LED_DBG(LED_RED_ON); */
  /*       radio_non_beacon_PAN_init(NON_BEACONED_PAN_COORDINATOR); */
  /*       //      WaitFor(20); */
  /*       LED_DBG(LED_RED_OFF); */
  /*       return SOS_OK; */
  /*     } */
  /*     if (radio_mode == BEACONED_PAN) */
  /*       return radio_beacon_PAN_init(); */
  /*   } */
  /*   else{ */
  /*     // Always in RX */
  /*     temp = TRUE;  */
  /*     mlmeSetRequest(MAC_RX_ON_WHEN_IDLE, &temp); */
  /*     //    ker_set_id(myShortAddr); */
  /*     LED_DBG(LED_GREEN_ON); */
  /*     WaitFor(20); */
  /*     LED_DBG(LED_RED_OFF); */
  /*     LED_DBG(LED_YELLOW_OFF); */
  /*     LED_DBG(LED_GREEN_OFF); */
  /*     return SOS_OK; */
  /*   } */
  /*   // Should never get here */
  /*   return -EINVAL; */
}

//-------------------------------------------------------------------------------------------------------
/**
 * @brief Perform an active scan for any existing PAN 
 */
/* static int8_t radio_active_scan() */
/* { */
/*   uint8_t i; */
/*   BYTE temp; */

/*   // Perform active scan */
/*   // Synchronise to the beacons */
/*   // Associate on the first PAN found (note that other PANs may be in the neighbourhood) */
/*   // Note: it takes about 5 seconds to scan all channels and associate, */
    
/*   // Start by resetting the MAC */
/*   mlmeResetRequest(TRUE); */

/*   // Set up pointer to return data from ScanConfirm */
/*   pScanConfirm = (MLME_SCAN_CONFIRM*) ker_malloc(128, RADIO_PID); */
/*   //  pScanConfirm = (MLME_SCAN_CONFIRM*) pBuffer; */
/*   status = mlmeScanRequest(ACTIVE_SCAN, 0xFFFFFFFF, 5); */
  
/*   // Halt if scan was unsuccessful, i.e. if no beacons were found. */
/*   //  while (status != SUCCESS); */
/*   if (status != SUCCESS) return -EIO; */

/*   // Find a PAN with association permit = TRUE */
/*   pPanDescriptor = NULL; */
/*   for (i = 0; i < pScanConfirm->resultListSize; i++) { */
/*     if (pScanConfirm->pPANDescriptorList[i].superframeSpec & SS_ASSOCIATION_PERMIT_BM) { */
/*       pPanDescriptor = &pScanConfirm->pPANDescriptorList[i]; */
/*       break; */
/*     }                 */
/*   } */
  
/*   // If there isn't any, then crash */
/*   //if (!pPanDescriptor) EXCEPTION(0xFF); */
/*   if (!pPanDescriptor) { */
/*     ker_free((void*)pScanConfirm); */
/*     return -EIO; */
/*   } */
  
/*   // First set the PAN ID to the PAN we want to associate to */
/*   NodePanId = pPanDescriptor->coordPanId; */
/*   mlmeSetRequest(MAC_PAN_ID, &NodePanId); */
/*   temp = BF(pPanDescriptor->superframeSpec, SS_BEACON_ORDER_BM, SS_BEACON_ORDER_IDX); */
/*   mlmeSetRequest(MAC_BEACON_ORDER, &temp); */
/*   temp = BF(pPanDescriptor->superframeSpec, SS_SUPERFRAME_ORDER_BM, SS_SUPERFRAME_ORDER_IDX); */
/*   mlmeSetRequest(MAC_SUPERFRAME_ORDER, &temp); */
  
/*   coordAddr = pPanDescriptor->coordAddress; */
/*   SOSPanChannel = pPanDescriptor->logicalChannel; */
  
/*   // If associating to a beacon network, synchronise first */
/*   if ((pPanDescriptor->superframeSpec & SS_BEACON_ORDER_BM) != 15) { */
/*     mlmeSyncRequest(pPanDescriptor->logicalChannel, TRUE); */
/*   } */
  
/*   // Request association to PAN coordinator */
/*   //  mlmeAssociateRequest(pPanDescriptor->logicalChannel, pPanDescriptor->coordAddrMode, NodePanId, &coordAddr, 0x0E, FALSE); */
/*   mlmeAssociateRequest(pPanDescriptor->logicalChannel, pPanDescriptor->coordAddrMode, NodePanId, &coordAddr, 0x8E, FALSE); */
  
/*   // Ram: Spinlock !! Need to remove it sometime ... sigh !! */

/*   // Wait for association to successfully complete. */
/*   while ((myShortAddr == 0xfffe)||(myShortAddr == 0xffff)); */

/*   // Free the allocated memory */
/*   ker_free((void*)pScanConfirm); */

/*   return SOS_OK; */
/* } */
//-------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize a non-beaconed PAN
 */
static int8_t radio_non_beacon_PAN_init(uint8_t mode)
{
  BYTE temp;
  // Start a non-beacon PAN
  // Reset MAC sublayer
  mlmeResetRequest(TRUE);
  
  // Set the MAC_SHORT_ADDRESS in the MAC PIB.
  // This must be done before starting the PAN
  mlmeSetRequest(MAC_SHORT_ADDRESS, (BYTE*) &myShortAddr);
  
  // Always in RX
  temp = TRUE; 
  mlmeSetRequest(MAC_RX_ON_WHEN_IDLE, &temp);


#ifdef RADIO_XMIT_POWER
  halTxSetPower(RADIO_XMIT_POWER);
#else
  halTxSetPower(31);
#endif

#ifdef RADIO_CHANNEL
  SOSPanChannel = RADIO_CHANNEL;
#else
  SOSPanChannel = 25;
#endif

  // Start non-beaconing PAN
  //  NodePanId = PANID;
  // Ram: I am changing the PAN ID to be same as the node address
  NodePanId = ker_id();
  if (mode == NON_BEACONED_PAN_COORDINATOR){
    // DisAllow association
    temp = FALSE; 
    mlmeSetRequest(MAC_ASSOCIATION_PERMIT, &temp);
#ifdef SOS_NIC
    temp = TRUE;
    mlmeSetRequest(MAC_PROMISCUOUS_MODE, &temp);
#endif
    //    watchdog_reset();
    if (mlmeStartRequest(NodePanId, SOSPanChannel, 15, 15, TRUE, FALSE, FALSE, FALSE) == SUCCESS)
      return SOS_OK;
  }
  /*   else{ */
  /*     // Dis-Allow association */
  /*     temp = FALSE;  */
  /*     mlmeSetRequest(MAC_ASSOCIATION_PERMIT, &temp); */
  /*     if (mlmeStartRequest(NodePanId, SOSPanChannel, 15, 15, FALSE, FALSE, FALSE, FALSE) == SUCCESS) */
  /*       return SOS_OK; */
  /*   } */
  return -EIO;
}
//-------------------------------------------------------------------------------------------------------

/**
 * @brief Initialize a beaconed PAN
 */
/* static int8_t radio_beacon_PAN_init() */
/* { */
/*   BYTE temp; */
/*   // Start a beacon-enabled PAN */
  
  
/*   // Reset MAC sublayer */
/*   mlmeResetRequest(TRUE); */
  
/*   // Set the MAC_SHORT_ADDRESS in the MAC PIB. */
/*   // This must be done before starting the PAN */
/*   mlmeSetRequest(MAC_SHORT_ADDRESS, (BYTE*) &myShortAddr); */
  
/*   // Start beaconing PAN with beacon and superframe order 4. */
/*   NodePanId = PANID; */
/*   //! Ram - The beaconed mode needs to be fixed */
/*   //  if (myShortAddr == COORD_SHORT){ */
/*     // Allow association */
/*     temp = TRUE;  */
/*     mlmeSetRequest(MAC_ASSOCIATION_PERMIT, &temp); */
/*     if (mlmeStartRequest(NodePanId, CHANNEL, 4, 4, TRUE, FALSE, FALSE, FALSE) == SUCCESS) */
/*       return SOS_OK; */
/*  /\*  } *\/ */
/* /\*   else { *\/ */
/* /\*     // Dis-Allow association *\/ */
/* /\*     temp = FALSE;  *\/ */
/* /\*     mlmeSetRequest(MAC_ASSOCIATION_PERMIT, &temp); *\/ */
/* /\*     if (mlmeStartRequest(NodePanId, SOSPanChannel, 4, 4, FALSE, FALSE, FALSE, FALSE) == SUCCESS) *\/ */
/* /\*        return SOS_OK; *\/ */
/* /\*   } *\/ */
/*   return -EIO; */
/* } */

//-------------------------------------------------------------------------------------------------------
/**
 * @brief Deassociate the radio with the currently associated PAN
 */
int8_t radio_deassociate()
{
  mlmeGetRequest(MAC_COORD_EXTENDED_ADDRESS, &coordExtAddr);
  mlmeDisassociateRequest((QWORD*) &coordExtAddr, 0x02, FALSE);
  return SOS_OK;
}

//-------------------------------------------------------------------------------------------------------
static void WaitFor(uint8_t time)
{
  while (time > 0){
    halWait(60000);
    time--;
  }
}

//-------------------------------------------------------------------------------------------------------
/**
 * @brief Post the packet to be sent out
 */
void radio_msg_alloc(Message* m)
{
  if (flag_msg_release(m->flag))
    ker_change_own(m->data, RADIO_PID);
  if (bTxBusy == FALSE){
    bTxBusy = TRUE;
    TxMsgPtr = m;
    post_short(RADIO_PID, RADIO_PID, MSG_RADIO_PKT_SEND, 0, 0, 0);
  }
  else
    mq_enqueue(&pq, m);
}

//-------------------------------------------------------------------------------------------------------
/**
 * @brief Radio Message Handler
 */
int8_t cc2420_radio_handler(void *state, Message *e)
{
  switch (e->type){
  case MSG_RADIO_PKT_SEND:
    {
      cc2420_tx_pkt();
      break;
    }
  case MSG_RADIO_PKT_RECV:
    {
      cc2420_rx_pkt((MCPS_DATA_INDICATION*)e->data);
      break;
    }
  default:
    break;
  }
  return SOS_OK;
}

//-------------------------------------------------------------------------------------------------------
/**
 * @brief link layer ack
 */
void ker_radio_ack_enable()
{   
}   
    
void ker_radio_ack_disable()
{
}     

int8_t radio_set_timestamp(bool on)
{
	return -EPERM;
    //bTsEnable = on;
    //return SOS_OK;
}  
//_________________________________________________________________________________________________________
// END OF SOS SPECIFIC FUNCTIONS
//-------------------------------------------------------------------------------------------------------
