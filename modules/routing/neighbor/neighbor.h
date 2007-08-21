
#ifndef _NEIGHBOR_H_
#define _NEIGHBOR_H_

#define SHM_NBR_LIST   0

//
// Typedefs
//
typedef struct nbr_entry_t {
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
  	struct nbr_entry_t *next;
} nbr_entry_t;

typedef struct est_entry_str {
	uint16_t id;
	uint8_t receiveEst;
} PACK_STRUCT est_entry_t;

typedef struct nbr_beacon_t {
	int16_t seqno;
	uint16_t parent;
	uint8_t hopcount;
	uint8_t estEntries;
	est_entry_t estList[];
} PACK_STRUCT nbr_beacon_t;

enum 
{
	NEIGHBOR_DISCOVERY_TIMER = 0,
	BACKOFF_TIMER  = 1,
};

enum 
{
    NBRFLAG_VALID    = 0x01,
    NBRFLAG_NEW      = 0x02,
    NBRFLAG_EST_INIT = 0x04,
	ROUTE_INVALID    = 0xff,
	MAX_NB_CNT                  = 10,  //! Max number of neighbors maintained in the local neighborhood table
	ACCEPTABLE_MISSED           = -20, //! Number of missed sequence numbers before the entry is purged
	ESTIMATE_TO_ROUTE_RATIO     = 5,   //! Number of beacons transmitted before link estimates are updated
	MIN_LIVELINESS              = 2, //! Min number of beacons to be received between periods of estimation
};


#endif

