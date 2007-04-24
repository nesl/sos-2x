#ifndef _TPSN_H_
#define _TPSN_H_

//Size of buffer, in which TPSN requests are stored
#define TPSN_BUFFER_SIZE 7

//Return error messages
#define REPLY_INVALID_PROPAGATION_DELAY 0
#define REPLY_REQUEST_OVERWRITTEN 1

//If defined, then propagation delay must be between 
//[AVG_PROPAGATION_DELAY - SIGMA, AVG_PROPAGATION_DELAY + SIGMA]
//in order to be considered valid
//#define SECURITY_ENABLED

//Msg to start instant time synchronization
#define MSG_GET_INSTANT_TIMESYNC (MOD_MSG_START + 0)

//Data structure that is passed when MSG_GET_INSTANT_TIMESYNC is sent
typedef struct
{
	uint8_t mod_id;
	uint8_t msg_type;
	uint16_t node_id;
	int32_t clock_drift;
} __attribute__ ((packed)) tpsn_t;

#ifndef _MODULE_
extern mod_header_ptr tpsn_get_header();
#endif //_MODULE_

#endif //_TPSN_H_
