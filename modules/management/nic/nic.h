#ifndef _NIC_H
#define _NIC_H



enum {
	MSG_SEND_GROUP = MOD_MSG_START,
	MSG_SET_GROUP,
	MSG_SOS_GROUP,
};

typedef struct {
	uint8_t group;
} __attribute__((packed)) sos_group_t;



#endif //_NIC_H


