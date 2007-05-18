/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file average_background.c
 */
 
#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>
//#define LED_DEBUG
#include <led_dbg.h>

#include <hardware.h>
#include <matrix.h>
#include "../include/app_config.h"

#define AVERAGE_BACKGROUND_PID DFLT_APP_ID3

//--------------------------------------------------------
// AVERAGE BACKGROUND STATE
//--------------------------------------------------------
typedef struct {
  func_cb_ptr output0;
  //func_cb_ptr put_token;	
  sos_pid_t pid;
} average_background_state_t;

//--------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------
static int8_t average_background_module(void *state, Message *e);
static int8_t input0 (func_cb_ptr p, token_type_t *t);
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length);

//--------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = AVERAGE_BACKGROUND_PID,
  .code_id		  = ehtons(AVERAGE_BACKGROUND_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(average_background_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 1,
  .num_prov_func  = 1,
  .module_handler = average_background_module,
  .funct			= {
        {error_8, "cyC2", AVERAGE_BACKGROUND_PID, INVALID_GID},
        //{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
		{input0, "cyC2", AVERAGE_BACKGROUND_PID, INPUT_PORT_0},
  },  
};

//--------------------------------------------------------
// MODULE HANDLER
//--------------------------------------------------------
static int8_t average_background_module(void *state, Message *msg)
{
	average_background_state_t *s = (average_background_state_t *)state;

	switch (msg->type){
		case MSG_INIT:
		{		
			s->pid = msg->did;
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
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	average_background_state_t *s = (average_background_state_t *)sys_get_state();
	uint8_t skip = SKIP;

	// Get token from port.
	CYCLOPS_Matrix *A = (CYCLOPS_Matrix *)get_token_data(t);

	//check the depth
	if(A->depth != CYCLOPS_1BYTE)
		return -EINVAL;
	else {
		uint8_t threshold;
		uint8_t i,j;  //row and col counter
		uint16_t samples = 0;  //how many samples we collected
		uint32_t total = 0;
		uint8_t *A_ptr8;

		A_ptr8 = ker_get_handle_ptr(A->data.hdl8);
		if ( A_ptr8 == NULL) {
			return -EINVAL;
		}		

		for( i=0; i < A->rows; i += skip){
			for(j=0; j < A->cols; j += skip){
				total += A_ptr8[(i * A->cols) + j];
				samples++;
			}
		}

		threshold = (uint8_t)(( (double)total )/ samples * COEFFICIENT);
		token_type_t *my_token = create_token(&threshold, sizeof(uint8_t), s->pid);
		if (my_token == NULL) return -ENOMEM;
		
		// Putting data on a port
		//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
		dispatch(s->output0, my_token);

		destroy_token(my_token);

		return SOS_OK;
	}
}
//--------------------------------------------------------	
#ifndef _MODULE_
mod_header_ptr average_background_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

