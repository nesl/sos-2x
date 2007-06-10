#ifndef __AODV_H__
#define __AODV_H__

#define CMN_PAYLOAD_SIZE  24

#define MSG_CMN_DATA_PKT  200
#define MOD_GET_BUFFER    1

enum {
	//algorithm parameters
	UNLIMITED_ENTRIES 						= 0,
	AODV_MAX_ROUTE_ENTRIES   				= 0,
	AODV_MAX_CACHE_ENTRIES					= 0,
	AODV_MAX_BUFFER_ENTRIES					= 0,
	AODV_MAX_NODE_ENTRIES					= 0,
	AODV_MAX_RREQ							= 0,
	
	//Timing constants  
	AODV_TIMER = 0, // Timer id
	
	TIMER_INTERVAL = 1024L, // Timer period = 1 sec
	  
  	ROUTE_DISCOVERY_TIMEOUT = 4,   // Time data remains in buffer
  	ROUTE_EXPIRATION_TIMEOUT = 10, // Time route stays in table
  	REVERSE_ROUTE_LIFE = 4, // Time route stays in cache

	//function return values
	FAIL = 0,
	NOT_FOUND = 0,
	FOUND = 1,
	SUCCESS = 1,
	NO_NEIGHBOR = 2,
	FOUND_BETTER = 1,
	FOUND_OLDER =2,
	FOUND_LONGER =3,
	//network variables
  	INVALID_NODE_ID = 0xffff,
  	AODV_PAYLOAD_SIZE = CMN_PAYLOAD_SIZE
};

//types of messages
enum {
	MSG_AODV_RECV_RREQ  = (MOD_MSG_START + 0), 
	MSG_AODV_RECV_RREP  = (MOD_MSG_START + 1),
	MSG_AODV_FWD_RREQ  = (MOD_MSG_START + 2),
	MSG_AODV_SEND_RREQ  = (MOD_MSG_START + 3),
	MSG_AODV_FWD_RREP  = (MOD_MSG_START + 4),
	MSG_AODV_SEND_RREP  = (MOD_MSG_START + 5),
	MSG_AODV_RECV_DATA  = (MOD_MSG_START + 6),
	MSG_AODV_FWD_DATA  = (MOD_MSG_START + 7),
	MSG_AODV_SEND_RERR = (MOD_MSG_START + 8),
	MSG_AODV_RECV_RERR = (MOD_MSG_START + 9),
	MSG_AODV2_ADD_TO_BUFFER = (MOD_MSG_START + 10),
	MSG_AODV2_GET_FROM_BUFFER = (MOD_MSG_START + 11),
	MSG_AODV2_UPDATE_BUFFER = (MOD_MSG_START + 12)
	
};

//registered functions (MOD_GET_BUFFER has number 1)
enum {
	MOD_ADD_TO_BUFFER = 2,
	MOD_GET_FROM_BUFFER = 3,
	MOD_REMOVE_EXPIRED_ENTRIES = 4,
	MOD_ADD_RREQ = 5,
	MOD_DEL_RREQ = 6,
	MOD_CHECK_RREQ = 7,
	MOD_GET_SEQ_NO = 8,
	MOD_UPDATE_SEQ_NO = 9
};

typedef struct AODV_rreq_pkt_str {
	uint16_t source_addr;
	uint16_t source_seq_no; 
	uint16_t broadcast_id;
	uint16_t dest_addr;
	uint16_t dest_seq_no;
	uint8_t hop_count; 
} __attribute__ ((packed))
AODV_rreq_pkt_t;

typedef struct AODV_rrep_pkt_str{
	uint16_t source_addr;
	uint16_t dest_addr;
	uint16_t dest_seq_no;
	uint8_t hop_count; 
} __attribute__ ((packed))
AODV_rrep_pkt_t;

typedef struct AODV_rerr_pkt_str{
	uint16_t addr;
	uint16_t seq_no;
} __attribute__ ((packed))
AODV_rerr_pkt_t;

typedef struct AODV_hdr_str{
	uint16_t dest_addr;
	uint16_t source_addr;
	uint8_t length;
	sos_pid_t dst_pid;
	sos_pid_t src_pid;
	uint8_t msg_type;
	uint8_t seq_no;
} __attribute__ ((packed))
AODV_hdr_t;

typedef struct AODV_pkt_str{
	AODV_hdr_t hdr;
	uint8_t data[AODV_PAYLOAD_SIZE]; 
} __attribute__ ((packed))
AODV_pkt_t;

typedef struct AODV_cache_entry_str {
	uint16_t dest_addr;
	uint16_t source_addr;
	uint16_t broadcast_id;
	uint16_t lifetime;
	uint16_t next_hop;
	uint16_t source_seq_no;
	uint8_t hop_count;
	struct AODV_cache_entry_str *next;
} AODV_cache_entry_t;

typedef struct AODV_route_entry_str{
	uint16_t dest_addr;
	uint16_t next_hop;
	uint8_t hop_count;
	uint16_t dest_seq_no;
	uint8_t lifetime;
	struct AODV_route_entry_str *next;
} AODV_route_entry_t;

typedef struct AODV_buf_pkt_entry_str{
	uint8_t lifetime;	
	AODV_pkt_t *buf_packet;
	struct AODV_buf_pkt_entry_str *next;
} 
AODV_buf_pkt_entry_t;

typedef struct AODV_node_entry_str{
	uint16_t addr;
	uint16_t seq_no;
	struct AODV_node_entry_str *next;
} AODV_node_entry_t;

typedef struct AODV_rreq_entry_str{
	uint16_t dest_addr;
	struct AODV_rreq_entry_str *next;
} AODV_rreq_entry_t;

typedef struct AODV_state_str{
	uint16_t seq_no;
	uint16_t broadcast_id;

	AODV_route_entry_t * AODV_route_ptr;
	AODV_cache_entry_t * AODV_cache_ptr;

	uint8_t num_of_route_entries;
	uint8_t num_of_cache_entries;

	uint8_t max_route_entries;
	uint8_t max_cache_entries;

	AODV_buf_pkt_entry_t * AODV_buf_ptr;
	AODV_rreq_entry_t * AODV_rreq_ptr;
	AODV_node_entry_t * AODV_node_ptr;

	uint8_t num_of_buf_packets;
	uint8_t num_of_rreq;
	uint8_t num_of_nodes;

	uint8_t max_buf_packets;
	uint8_t max_rreq;	
	uint8_t max_nodes;
} AODV_state_t;

#endif
