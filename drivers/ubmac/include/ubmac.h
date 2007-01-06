#ifndef _UBMAC_H_
#define _UBMAC_H_

#include <inttypes.h>

#define CSMA_TIMEOUT     3125// 27 msec in clock ticks

typedef struct
{
	uint16_t node_id;
	uint8_t timesync_mod_id;
	uint8_t sync_precision;
} __attribute__ ((packed)) ubmac_init_t;

#define MSG_START_TIMESYNC (MOD_MSG_START + 0)
#define MSG_STOP_TIMESYNC (MOD_MSG_START + 1)

uint16_t ubmacGetTime(uint16_t node_id);
uint32_t ubmacGetPreamble(uint16_t node_id);
void notifyUbmac(uint32_t wakeup_time);

#endif // _UBMAC_H_

