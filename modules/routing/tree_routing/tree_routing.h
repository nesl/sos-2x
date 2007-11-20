#ifndef _TREE_ROUTING_H
#define _TREE_ROUTING_H

#define SHM_TR_VALUE   0 

typedef uint8_t (*get_hdr_size_proto)(func_cb_ptr);

enum {
    BASE_STATION_ADDRESS        = 1,   //! Address of the base-station node
	MAX_ALLOWABLE_LINK_COST     = 1536, //! Max. value of the link cost
};

typedef struct {
	uint16_t parent;
	uint8_t hop_count;
} tr_shared_t;

//-------------------------------------------------------------
// MODULE MESSAGES
//-------------------------------------------------------------
#define TREE_ROUTING_MSG_START MOD_MSG_START
enum {
   MSG_TR_DATA_PKT = TREE_ROUTING_MSG_START + 2,  //! beacon packet type
   MSG_NEW_CHILD,
   MSG_SEND_TO_CHILDREN,
			MSG_REMOVE_CHILD,
};

//-------------------------------------------------------------
// MODULE PUBLISHED FUNCTION FIDS
//-------------------------------------------------------------
enum {  
   MOD_GET_HDR_SIZE_FID = 1, //! Function ID of the get header size function
   TREE_ROUTE_MSG_FID   = 2, //! Function ID of the tree route function
			MOD_SET_CHLD_MSG_FID = 3,
};


typedef struct {
      uint16_t originaddr;
      int16_t seqno;
      uint8_t hopcount;
      uint8_t originhopcount;
      sos_pid_t dst_pid;
      uint8_t reserved;
      uint16_t parentaddr; // Ram - This field is only for visualization of topology
						uint16_t children[10];
} PACK_STRUCT tr_hdr_t;



#ifndef _MODULE_
extern mod_header_ptr tree_routing_get_header();
#endif

#endif
