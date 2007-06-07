/* -*- Mode: C; tab-width:4 -*-                                */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent:            */
/* LICENSE: See $(SOSROOT)/LICENSE.ucla                        */

#include <sys_module.h>

// 
// This is for randomly pinging node 2-5 at node 1
//#define TEST_PING

enum {
	MSG_SEND_PING      =    ( MOD_MSG_START + 0 ),
	MSG_PING      =    ( MOD_MSG_START + 1 ),
	MSG_PING_REPLY      =    ( MOD_MSG_START + 2 ),
	MAX_PING_REPEAT  = 6,
};

typedef struct _sos_state_t {
	uint16_t node_pinged;
	uint8_t repeated;
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
				DEBUG("Ping to %d failed!\n", s->node_pinged);
				if( s->repeated < MAX_PING_REPEAT) {
					DEBUG("Send Ping to %d\n", s->node_pinged);
					sys_post_net( sys_pid(), MSG_PING, 0, NULL, 0, s->node_pinged);
					s->repeated++;
				} else {
					// Ping failed
					sys_timer_stop(0);
					s->node_pinged = BCAST_ADDRESS;
				}
			} 
#ifdef TEST_PING
			else {
				if( s->node_pinged == BCAST_ADDRESS ) {
				uint16_t *dest = sys_malloc(2);
				*dest = sys_rand() % 20 + 2;
				sys_post(sys_pid(), MSG_SEND_PING, 2, dest, SOS_MSG_RELEASE);
				}
			}
#endif
			return SOS_OK;
		}
		case MSG_PING_REPLY: {
			if( s->node_pinged == msg->saddr ) {
				DEBUG("Ping to %d successful!\n", s->node_pinged);
				sys_timer_stop(0);
				s->node_pinged = BCAST_ADDRESS;	
			} else {
				DEBUG("Receive reply from %d\n", msg->saddr );
			}
			return SOS_OK;
		}
		case MSG_INIT: {
		   s->node_pinged = BCAST_ADDRESS;
#ifdef TEST_PING
		   if( sys_id() == 1 ) {
			   sys_timer_start ( 1, 5 * 1024L, TIMER_REPEAT );
		   }
#endif
			return SOS_OK;
		}
		case MSG_FINAL: {
			return SOS_OK;
		}
		case MSG_SEND_PING:
		{
			s->repeated = 0;
			s->node_pinged = *(uint16_t*)msg->data;
			DEBUG("Send Ping to %d\n", s->node_pinged);
			sys_timer_start ( 0, 5 * 1024L, TIMER_REPEAT );
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

