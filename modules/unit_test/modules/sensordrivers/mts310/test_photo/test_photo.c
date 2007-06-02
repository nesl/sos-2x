/* -*- Mode: C; tab-width:4 -*-                                */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent:            */
/* LICENSE: See $(SOSROOT)/LICENSE.ucla                        */

#include <sys_module.h>
#include <mts310sb.h>

static int8_t _sos_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER =
{
	.mod_id          = DFLT_APP_ID0,
	.state_size      = 0,
	.num_timers      = 0,
	.num_sub_func    = 0,
	.num_prov_func   = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id         = ehtons(DFLT_APP_ID0),
	.module_handler  = _sos_handler,
};


static int8_t _sos_handler ( void *state, Message *msg )
{
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

