/* -*- Mode: C; tab-width:4 -*-                                */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent:            */
/* LICENSE: See $(SOSROOT)/LICENSE.ucla                        */

#include <sys_module.h>
#include <mts310sb.h>

/*
 * TODO : Define Message here
 */
enum {
	MSG_APPMSG1      =    ( MOD_MSG_START + 0 ),
	MSG_APPMSG2      =    ( MOD_MSG_START + 1 ),
	// WARNING: number of message cannot be more than 32
	MSG_LAST_MSG     =    ( MOD_MSG_START + 31 ),  
};

typedef struct _sos_state_t {
	// TODO : Add subscribed function here, first example is given
	// func_cb_ptr func1;
	// TODO : Add application specific state
} _sos_state_t;

static int8_t _sos_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER =
{
	.mod_id          = DFLT_APP_ID0,
	.state_size      = sizeof ( _sos_state_t ),
	.num_timers      = 0,
	.num_sub_func    = 0,
	.num_prov_func   = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id         = ehtons(DFLT_APP_ID0),
	.module_handler  = _sos_handler,
	/* TODO : uncomment to use function pointer pointer 
	 * TODO : update num_sub_func as needed
	.funct = {
		[0] = {error_16, "Svv0", <Module ID>, <Function ID>},
	}
	*/
};


static int8_t _sos_handler ( void *state, Message *msg )
{
	//_sos_state_t *s = ( _sos_state_t * ) state;
	switch ( msg->type ) {
		case MSG_TIMER_TIMEOUT: {
			sys_sensor_get_data(MTS310_PHOTO_SID);
			return SOS_OK;
		}
		case MSG_DATA_READY: {
			MsgParam* param = (MsgParam*) (msg->data);
			uint16_t *data;

			data = sys_malloc(sizeof(uint16_t));
			*data = param->word;

			sys_post_uart(DFLT_APP_ID0, 32, 2, data, SOS_MSG_RELEASE, BCAST_ADDRESS);
			return SOS_OK;
		}
		case MSG_INIT: {
			// TODO : uncomment to use the timer 0 of 1 second (1024 counts)
			sys_timer_start ( 0, 64L, TIMER_REPEAT );
			return SOS_OK;
		}
		case MSG_FINAL: {
			return SOS_OK;
		}
	}
	return -EINVAL;
}

#ifndef _MODULE_
mod_header_ptr test_photo_get_header ( )
{
	return sos_get_header_address ( mod_header );
}
#endif

