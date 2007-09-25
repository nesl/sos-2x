#ifndef _TPSN_NET_H_
#define _TPSN_NET_H_

// message to get global time conversion from tpsn_net module
#define MSG_GET_GLOBAL_TIME (MOD_MSG_START + 0)
// reply to the global time conversion
#define MSG_GLOBAL_TIME_REPLY (MOD_MSG_START + 1)
// advertisement message that is sent out to request a level reply
#define MSG_ADV_LEVEL (MOD_MSG_START + 2)
// advertisement reply
#define MSG_ADV_REPLY (MOD_MSG_START + 3)
// reply from tpsn module
#define MSG_TPSN_REPLY (MOD_MSG_START + 4)

#define NOT_SYNCED  0xFFFFFFFFL

// Definitions for getting synced time using 
// SOS_CALL
#define GET_GLOBAL_TIME_FID		0
typedef uint32_t (*get_global_time_func_t)(func_cb_ptr, uint32_t);

typedef struct
{
    uint32_t time;
    uint32_t refreshed;
} PACK_STRUCT msg_global_time_t;

typedef struct
{
    uint32_t time[2];
    uint8_t type;
    uint8_t seq_no;
} PACK_STRUCT tpsn_req_t;

typedef struct
{
    uint32_t time[2];
    uint8_t type;
    uint8_t seq_no;
    uint32_t previous_time[2];
} PACK_STRUCT tpsn_reply_t;

typedef struct
{
    int8_t level;
} PACK_STRUCT msg_adv_level_t;

#ifndef _MODULE_
extern mod_header_ptr tpsn_net_get_header();
#endif //_MODULE_

#endif //_TPSN_NET_H_
