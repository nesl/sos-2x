#ifndef WIRING_CONFIG_PRIVATE_H
#define WIRING_CONFIG_PRIVATE_H

// The message from loader uses first two bytes for
// module ID and data type (or handle). Skip these
// two bytes to read the main content.
#define LOADER_METADATA_SKIP_OFFSET	2

// The first byte in the elements table contains
// number of elements currently active in the 
// application graph.
#define ELEMENTS_TABLE_SKIP_OFFSET	1

#define PARAM_TABLE_SKIP_OFFSET		2

// Maximum number of elements that can be currently supported
// by the Vire run-time engine.
#define MAX_NUM_ELEMENTS			16

// Task to reset the graph invoked by a run-time error.
#define MSG_RESET_TASK			(KER_MSG_START + 22)
// Task to process enqueued tokens.
#define MSG_CONTINUE_TASK		(KER_MSG_START + 23)
// Task to apply the parameter update once all the 
// elements have been initialized.
#define MSG_UPDATE_PARAMETERS	(KER_MSG_START + 24)

enum {
	ELEMENT_BUSY = 0,
	ELEMENT_READY,
};

enum {
	NO_INSTANCE_EXISTS = 0,
	THIS_INSTANCE_EXISTS,
	ANOTHER_INSTANCE_EXISTS,
};

enum {
	LOAD	= 0,
	DEREGISTER,
};

enum {
	UNMARKED	= 0,
	MARKED,
	ALL,
};

// Status for enqueued tokens.
enum {
	TOKEN_QUEUED = 0,
	TOKEN_POSTED,
	TOKEN_HANDLED,
};

typedef int8_t (*input_func_t)(func_cb_ptr p, token_type_t *t);
typedef int8_t (*update_param_func_t)(func_cb_ptr p, void *data, uint16_t length);

// Common header for all structures implementing a queue.
typedef struct queue_header_t {
	struct queue_header_t *next;
} queue_header_t;

// Common record header shared between wiring table
// and parameter table for identifying an element
// using description from back-end server.
typedef struct {
	uint8_t type;
	sos_code_id_t cid;
	uint8_t instance_id;
} PACK_STRUCT common_table_header_t;

// Each section in the wiring configuration file
// begins with this header. 'length' also includes
// the end section record (END_TABLE or BEGIN_PARAMETER).
typedef struct {
	uint8_t type;
	uint16_t length;
	uint8_t empty[3];
} PACK_STRUCT section_header_t;

// Structure to read in a row of the wiring configuration sent
// by the backend PC.
typedef struct {
	common_table_header_t e;
	uint8_t port_id;
	uint8_t gid_or_end;
} PACK_STRUCT wiring_table_row_t;

// Structure to read in a row of the parameter configuration sent
// by the backend PC.
typedef struct {
	common_table_header_t e;
	uint8_t length;
} PACK_STRUCT param_table_row_t;

// Linked list to maintain the set of elements instantiated
// This list is finally written on to flash, and is used
// later for parameter updates, and deregistering modules to stop 
// the graph.
typedef struct graph_element_t {
	queue_header_t h;
	sos_code_id_t cid;
	uint8_t instance_id;
	sos_pid_t pid;
	uint8_t marked;
} PACK_STRUCT graph_element_t;

// This temporary structure is used to maintain a list
// of input ports (their corresponding function control blocks)
// connected to an output port. After the output port
// is completely processed, the <cb, pid_port> information
// from this is written on to flash in the form of
// a table.
typedef struct routing_table_elm_t {
	queue_header_t h;
	uint8_t pid_port;
	func_cb_ptr cb;	
} PACK_STRUCT routing_table_elm_t;

typedef struct routing_table_ram_t {
	union {
		uint8_t pid_port;
		uint8_t num_ports;
	} index;
	func_cb_ptr cb;	
} PACK_STRUCT routing_table_ram_t;

// The pid_port entry of the routing table has the following
// format:
// Element PID {5 bits} | Input port ID {3 bits}
// This restricts the max number of input ports per element to 8.
#define ROUTING_TABLE_ENTRY_SIZE (sizeof(routing_table_ram_t))
#define RTABLE_INPUT_PORT_BITS		3
#define MAX_INPUT_PORTS_PER_ELEMENT	(1 << RTABLE_INPUT_PORT_BITS)
#define RTABLE_INPUT_PORT_MASK		0x07
#define RTABLE_PID_MASK				0xF8
#define get_epid_from_pid_port(p) ( ((p) & RTABLE_PID_MASK) >> RTABLE_INPUT_PORT_BITS )
#define get_epid_from_pid(p) ( (p) % MAX_NUM_ELEMENTS )
#define get_port_from_pid_port(p) ( (p) & RTABLE_INPUT_PORT_MASK )

typedef struct token_queue_t {
	queue_header_t h;
	token_type_t *t;
	func_cb_ptr cb;
	uint8_t portID;
	uint8_t status;
} PACK_STRUCT token_queue_t;

// This structure holds the tokens (data, length) that have been
// queued for later delivery. Destination function CB
// and PID are included for execution efficiency as
// it saves an extra fetch from the flash.
typedef struct mesg_queue_t {
	queue_header_t h;
	sos_pid_t epid;
	token_queue_t *tokens;
} PACK_STRUCT mesg_queue_t;

// This is used to post a CONTINUATION task to the wiring
// engine when a module completes a long running operation,
// outputs a token, and is ready to accept new tokens.
// It holds a pointer to the queued token.
typedef struct msg_continue_t {
	sos_pid_t epid;
	token_queue_t *mptr;
} PACK_STRUCT msg_continue_t;

#endif
