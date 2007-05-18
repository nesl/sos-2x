/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file max_locate.c
 */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>
//#define LED_DEBUG
#include <led_dbg.h>

#include <hardware.h>
#include <matrix.h>
#include "../include/app_config.h"

#define MAX_LOCATE_PID (DFLT_APP_ID0 + 5)

//--------------------------------------------------------
// MAX LOCATE STATE
//--------------------------------------------------------
typedef struct {
  func_cb_ptr output0;
  func_cb_ptr output1;
  //func_cb_ptr put_token;
  sos_pid_t pid;
} max_locate_state_t;


//--------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------
static int8_t max_locate_module(void *state, Message *e);
static int8_t input0 (func_cb_ptr p, token_type_t *t);

//--------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = MAX_LOCATE_PID,
  .code_id		  = ehtons(MAX_LOCATE_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(max_locate_state_t),
  .num_out_port   = 2,
  .num_sub_func   = 2,
  .num_prov_func  = 1,
  .module_handler = max_locate_module,
  .funct			= {
	{error_8, "cyC2", MAX_LOCATE_PID, INVALID_GID},
	{error_8, "cyC2", MAX_LOCATE_PID, INVALID_GID},
	//{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
	{input0, "cyC2", MAX_LOCATE_PID, INPUT_PORT_0},
  },  
};

//--------------------------------------------------------
// MODULE HANDLER
//--------------------------------------------------------
static int8_t max_locate_module(void *state, Message *msg)
{
	max_locate_state_t *s = (max_locate_state_t *)state;

	switch (msg->type){
		case MSG_INIT:
		{
			s->pid = msg->did;
			break;
		}
		case MSG_FINAL:
			break;
		default:
			return -EINVAL;
	}
	return SOS_OK;
}

//--------------------------------------------------------
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	max_locate_state_t *s = (max_locate_state_t *)sys_get_state();

	// Get token from port.
	CYCLOPS_Matrix *A = (CYCLOPS_Matrix *)get_token_data(t);

	uint8_t possibleMax = 0;
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t row, col;

	if (A->depth==CYCLOPS_1BYTE){
		uint8_t *A_ptr8;
		A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
		if ( A_ptr8 == NULL) 
				return -EINVAL;
		for(i=0; i < A->rows; i++){
			for(j=0; j < A->cols; j++){
				if (A_ptr8[(i*A->cols) + j] > possibleMax) {
					possibleMax = A_ptr8[ (i*A->cols) + j];
					row = i;
					col = j;
				}
			}
		}
	}

	// Putting data on a port
	token_type_t *my_token = create_token(&(row), sizeof(uint8_t), s->pid);
	if (my_token == NULL) return -ENOMEM;
	//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
	dispatch(s->output0, my_token);
	destroy_token(my_token);

	my_token = create_token(&(col), sizeof(uint8_t), s->pid);
	if (my_token == NULL) return -ENOMEM;
	//SOS_CALL(s->put_token, put_token_func_t, s->output1, my_token);
	dispatch(s->output1, my_token);
	destroy_token(my_token);

	return SOS_OK;
}

//--------------------------------------------------------
#ifndef _MODULE_
mod_header_ptr max_locate_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

