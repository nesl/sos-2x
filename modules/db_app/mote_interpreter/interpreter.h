#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <sys_module.h>

#define MSG_NEW_QUERY (MOD_MSG_START + 1)
#define MSG_QUERY_REPLY (MOD_MSG_START + 2)
#define MSG_VALIDATE (MOD_MSG_START + 3)
#define MSG_DISPATCH (MOD_MSG_START + 4)
#define MSG_REMOVE (MOD_MSG_START + 5)

#define NUM_SENSORS 8
#define BASE_STATION_ADDRESS 1
#define MOTE_INTERPRETER_PID DFLT_APP_ID0

// define the routing protocol mesage types and pid
#define MSG_TR_DATA_PKT (MOD_MSG_START + 2)
#define MSG_NEW_CHILD   (MOD_MSG_START + 3)
#define MSG_SEND_TO_CHILDREN (MOD_MSG_START + 4)
#define ROUTING_PID TREE_ROUTING_PID

enum{
    LESS_THAN=1,
    GREATER_THAN,
    EQUAL,
    LESS_THAN_EQUAL,
    GREATER_THAN_EQUAL,
    NOT_EQUAL,
};

enum{
    AND=1,
    OR,
    AND_NOT,
    OR_NOT,
    NOT,
};

typedef uint8_t (*get_hdr_size_proto) (func_cb_ptr);

enum{
    MOD_GET_HDR_SIZE_FID = 1,
};

typedef struct {
	uint8_t sid;
	uint8_t comp_op_and_relation;
	uint16_t comp_value;
} qualifier_t;


typedef struct {
	uint8_t command;
	uint8_t value;
} trigger_t;

typedef struct {
	uint16_t qid;
	uint16_t total_samples;
	uint32_t interval;
	uint8_t num_queries;
	uint8_t num_qualifiers;
	trigger_t trigger;
	uint8_t *queries;
	qualifier_t *qualifiers;
	uint16_t *results;           // when a sensor value is recieved we save it here so that we can send them 
	                             // all out together, or do any comparisons needed
				     // this only needs to be used when number of qualifiers is non-zero
	uint8_t recieved;            // a non zero value marks the value as being recieved
                                     // a zero value marks it as non recieved, 
				     // upon every new epoch, recieved should be set to 0
				     // this only really has to be used when the number of qualifiers is non-zero	
} query_details_t;

typedef struct {
	uint8_t sensor;
	uint16_t value;
} sensor_msg_t;

#define STATIC_QUERY_SIZE 12
typedef struct {
	uint16_t qid;
	uint16_t num_remaining;
	uint8_t num_results;
	uint8_t node_id;
	sensor_msg_t results[];
} query_result_t;


#endif
