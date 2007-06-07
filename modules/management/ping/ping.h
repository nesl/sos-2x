#ifndef __PING_H__
#define __PING_H__

enum {
	MSG_SEND_PING      =    ( MOD_MSG_START + 0 ),
	MSG_PING      =    ( MOD_MSG_START + 1 ),
	MSG_PING_REPLY      =    ( MOD_MSG_START + 2 ),
	PING_REPEAT_TIME    = (5 * 1024L),
	MAX_PING_REPEAT  = 6,
};     

typedef struct ping_req_t {
	uint16_t count;
	uint16_t addr;
} ping_req_t;

#endif


