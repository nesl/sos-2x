/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file select_transmit.c
 */
 
#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>
//#define LED_DEBUG
#include <led_dbg.h>

#include <hardware.h>
#include <matrix.h>
#include <string.h>
#include "../include/app_config.h"

#define SELECT_TRANSMIT_PID (DFLT_APP_ID0 + 7)

enum {
	SUB_STATE_IDLE	= 0,
	SUB_STATE_WAIT_1,
};

//--------------------------------------------------------
// ABSOLUTE SUBTRACT
//--------------------------------------------------------
typedef struct {
  func_cb_ptr output0;
  //func_cb_ptr put_token;
  uint8_t actionFlag;
  sos_pid_t pid;
} select_transmit_state_t;


//--------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------
static int8_t select_transmit_module(void *state, Message *e);
static int8_t input0 (func_cb_ptr p, token_type_t *t);
static int8_t input1 (func_cb_ptr p, token_type_t *t);

//--------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = SELECT_TRANSMIT_PID,
  .code_id		  = ehtons(SELECT_TRANSMIT_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(select_transmit_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 1,
  .num_prov_func  = 2,
  .module_handler = select_transmit_module,
  .funct			= {
	{error_8, "cyC2", SELECT_TRANSMIT_PID, INVALID_GID},
	//{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
	{input0, "cyC2", SELECT_TRANSMIT_PID, INPUT_PORT_0},
	{input1, "cyC2", SELECT_TRANSMIT_PID, INPUT_PORT_1},
  },  
};

//--------------------------------------------------------
// MODULE HANDLER
//--------------------------------------------------------
static int8_t select_transmit_module(void *state, Message *msg)
{
	select_transmit_state_t *s = (select_transmit_state_t *) state;

	switch (msg->type){
		case MSG_INIT:
		{		
			s->pid = msg->did;
			s->actionFlag = NONE;
			break;
		}
		case MSG_FINAL:
		{
			break;
		}
		default:
			return -EINVAL;
	}
	return SOS_OK;
}

//--------------------------------------------------------
static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	select_transmit_state_t *s = (select_transmit_state_t *)sys_get_state();

	if (t == NULL) return -EINVAL;

	objectInfo *obj = (objectInfo *)get_token_data(t);

	s->actionFlag = obj->actionFlag;

	return SOS_OK;
}

//--------------------------------------------------------
static int8_t input1 (func_cb_ptr p, token_type_t *t) {
	select_transmit_state_t *s = (select_transmit_state_t *)sys_get_state();

	if (t == NULL) return -EINVAL;

	if (s->actionFlag == OBJ_DETECT) {
		//SOS_CALL(s->put_token, put_token_func_t, s->output0, t);
		dispatch(s->output0, t);
	}

	return SOS_OK;
}
	
//--------------------------------------------------------
#ifndef _MODULE_
mod_header_ptr select_transmit_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

