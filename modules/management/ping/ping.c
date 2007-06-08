/* -*- Mode: C; tab-width:4 -*-                                */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent:            */
/* LICENSE: See $(SOSROOT)/LICENSE.ucla                        */

#include <sys_module.h>
#include "ping.h"

// 
// This is for randomly pinging node 2-5 at node 1
//#define TEST_PING

#ifdef TEST_PING
// this memory is used by Avrora to collect ping statistic
static uint8_t ping_succ;
#endif

typedef struct _sos_state_t {
	uint16_t node_pinged;
	uint16_t count;
	uint16_t repeated;
	uint32_t ping_timestamp;  // the timestamp
	bool succ;
} _sos_state_t;

static int8_t _sos_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER =
{
	.mod_id          = PING_PID,
	.state_size      = sizeof ( _sos_state_t ),
	.num_timers      = 0,
	.num_sub_func    = 0,
	.num_prov_func   = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id         = ehtons(PING_PID),
	.module_handler  = _sos_handler,
};


static int8_t _sos_handler ( void *state, Message *msg )
{
	_sos_state_t *s = ( _sos_state_t * ) state;
	switch ( msg->type ) {
		case MSG_TIMER_TIMEOUT: {
			if( sys_timer_tid(msg) == 0 ) {
				if( s->succ == false ) {
#ifdef TEST_PING
					ping_succ = 0;
#endif
					DEBUG("Ping to %d failed!\n", s->node_pinged);
				}
				if( s->count != 0 ) {
					s->repeated++;
					if( s->repeated == s->count ) {
						sys_timer_stop(0);
						s->node_pinged = BCAST_ADDRESS;	
#ifdef SOS_EMU
						exit(0);
#endif
						return SOS_OK;
					}
				}
				s->succ = false;
				s->ping_timestamp = sys_time32();
				sys_post_net( sys_pid(), MSG_PING, 0, NULL, 0, s->node_pinged);
			} 
#ifdef TEST_PING
			else {
				if( s->node_pinged == BCAST_ADDRESS ) {
					ping_req_t *req = sys_malloc(sizeof(ping_req_t));
					req->addr = sys_rand() % 25;
					req->count = 1;
					sys_post(sys_pid(), MSG_SEND_PING, sizeof(ping_req_t), 
							req, SOS_MSG_RELEASE);
				}
			}
#endif
			return SOS_OK;
		}
		case MSG_PING_REPLY: {
			if( s->node_pinged == msg->saddr ) {
#ifdef TEST_PING
				ping_succ = 1;
#endif
				DEBUG("Ping to %d RTT = %d\n", s->node_pinged, (int32_t)sys_time32() - (int32_t)s->ping_timestamp);
				if( s->count != 0 ) {
					s->repeated++;
					if( s->repeated == s->count ) {
						sys_timer_stop(0);
						s->node_pinged = BCAST_ADDRESS;	
#ifdef SOS_EMU
						exit(0);
#endif
						return SOS_OK;
					}
				}
				s->succ = true;
			} else {
				DEBUG("Receive reply from %d\n", msg->saddr );
			}
			return SOS_OK;
		}
		case MSG_INIT: {
		   s->node_pinged = BCAST_ADDRESS;
#ifdef TEST_PING
		   if( sys_id() == 1 ) {
			   sys_timer_start ( 1, PING_REPEAT_TIME, TIMER_REPEAT );
		   }
#endif
			return SOS_OK;
		}
		case MSG_FINAL: {
			return SOS_OK;
		}
		case MSG_SEND_PING:
		{
			ping_req_t *r = (ping_req_t*) msg->data;
			s->repeated = 0;
			s->node_pinged = r->addr;
			s->count = r->count;
			s->succ = false;
			DEBUG("Send Ping to %d\n", s->node_pinged);
			sys_timer_start ( 0, PING_REPEAT_TIME, TIMER_REPEAT );
			sys_post_net( sys_pid(), MSG_PING, 0, NULL, 0, s->node_pinged);
			return SOS_OK;
		}
		case MSG_PING:
		{
			//
			// Receive ping: reply
			//
			DEBUG("Receive Ping from %d, Reply...\n", msg->saddr );
			sys_post_net( sys_pid(), MSG_PING_REPLY, 0, NULL, 0, msg->saddr );
			return SOS_OK;
		}
	}
	return -EINVAL;
}

#ifndef _MODULE_
mod_header_ptr ping_get_header ( )
{
	return sos_get_header_address ( mod_header );
}
#endif

