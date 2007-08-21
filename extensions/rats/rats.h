#ifndef _RATS_H_
#define _RATS_H_

#include <inttypes.h>

#define SCALING_FACTOR 4
#define LOWER_THRESHOLD 0.75F
#define HIGHER_THRESHOLD 0.9F
#define TIME_CONSTANT 120 //2 minutes (in seconds)
#define BUFFER_SIZE 4

#define MIN_SAMPLING_PERIOD 2 //sec
#define MAX_SAMPLING_PERIOD 4100 //64 minutes (in seconds)
#define INITIAL_TRANSMISSION_PERIOD 2 //sec
#define PANIC_TIMER_RETRANSMISSIONS 5
#define UNICAST_VALIDATION_RETRANSMISSIONS 5
#define BROADCAST_VALIDATION_RETRANSMISSIONS 2

#define MSG_RATS_CLIENT_START (MOD_MSG_START + 0)
#define MSG_RATS_GET_TIME (MOD_MSG_START + 1)
#define MSG_RATS_CLIENT_STOP (MOD_MSG_START + 2)

uint32_t convert_from_mine_to_parent_time(uint32_t time, uint16_t parent_node_id);
uint32_t convert_from_parent_to_my_time(uint32_t time, uint16_t child_node_id);
	
typedef struct
{
	uint8_t mod_id;
	uint16_t source_node_id;
	uint16_t target_node_id;	
	uint32_t time_at_source_node;
	uint32_t time_at_target_node;	
	uint32_t error; //msec
	uint8_t msg_type;
} PACK_STRUCT rats_t;

#endif // _RATS_H_
