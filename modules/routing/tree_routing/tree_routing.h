#ifndef _TREE_ROUTING_H
#define _TREE_ROUTING_H

typedef uint8_t (*get_hdr_size_proto)(func_cb_ptr);
enum 
  {
    NBRFLAG_VALID    = 0x01,
    NBRFLAG_NEW      = 0x02,
    NBRFLAG_EST_INIT = 0x04
  };




enum 
  {
    BASE_STATION_ADDRESS        = 1,   //! Address of the base-station node
    MAX_NB_CNT                  = 10,  //! Max number of neighbors maintained in the local neighborhood table
    ESTIMATE_TO_ROUTE_RATIO     = 5,   //! Number of beacons transmitted before link estimates are updated
    ACCEPTABLE_MISSED           = -20, //! Number of missed sequence numbers before the entry is purged
    DATA_TO_ROUTE_RATIO         = 2,   //! Number of data packets transmitted before a becon is sent out
    DATA_FREQ                   = 10 * 1024, //! Frequency of data being transmitted
    MAX_ALLOWABLE_LINK_COST     = 1536, //! Max. value of the link cost
    MIN_LIVELINESS              = 2, //! Min number of beacons to be received between periods of estimation
    MAX_EST_ENTRIES_IN_BEACON   = 16, //! Max. entries that can be inserted in a beacon packet
  };

enum 
{
   ROUTE_INVALID    = 0xff
};



//-------------------------------------------------------------
// MODULE MESSAGES
//-------------------------------------------------------------
enum {
   MSG_BEACON_SEND = MOD_MSG_START,      //! beacon task
   MSG_BEACON_PKT  = MOD_MSG_START + 1,  //! beacon packet type
   MSG_TR_DATA_PKT = MOD_MSG_START + 2,  //! beacon packet type
};

//-------------------------------------------------------------
// MODULE PUBLISHED FUNCTION FIDS
//-------------------------------------------------------------
enum {  
   MOD_GET_HDR_SIZE_FID = 1, //! Function ID of the get header size function
   TREE_ROUTE_MSG_FID   = 2, //! Function ID of the tree route function
};


typedef struct neighbor_entry_str {
  uint16_t id;  // Node Address
  uint16_t parent;
  int16_t lastSeqno;
  uint8_t missed;
  uint8_t received;
  uint8_t flags;
  uint8_t liveliness;
  uint8_t hop;
  uint8_t receiveEst;
  uint8_t sendEst;
  struct neighbor_entry_str *next;
} neighbor_entry_t;

typedef struct neighbor_short {
  uint16_t id;  // Node Address
  uint8_t receiveEst;
  uint8_t sendEst;
} __attribute__ ((packed))
  neighbor_short_t;

typedef struct est_entry_str {
  uint16_t id;
  uint8_t receiveEst;
} __attribute__ ((packed)) est_entry_t;

typedef struct beacon_str {
  int16_t seqno;
  uint16_t parent;
  uint8_t hopcount;
  uint8_t estEntries;
  est_entry_t estList[1];
} __attribute__ ((packed)) tr_beacon_t;

typedef struct {
      uint16_t originaddr;
      int16_t seqno;
      uint8_t hopcount;
      uint8_t originhopcount;
      sos_pid_t dst_pid;
      uint8_t reserved;
      uint16_t parentaddr; // Ram - This field is only for visualization of topology
} __attribute__ ((packed)) tr_hdr_t;



#ifndef _MODULE_
extern mod_header_ptr tree_routing_get_header();
#endif

#endif
