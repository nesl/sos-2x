/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file update_background.c
 */
 
#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>
//#define LED_DEBUG
#include <led_dbg.h>

#include <matrix.h>
#include <string.h>
#include "../include/app_config.h"

#define UPDATE_BACKGROUND_PID DFLT_APP_ID2

enum {
	UPD_STATE_INIT	= 0,
	UPD_STATE_PROCESS,
};

//--------------------------------------------------------
// UPDATE BACKGROUND STATE
//--------------------------------------------------------
typedef struct {
	func_cb_ptr output0;
	//func_cb_ptr put_token;	
	sos_pid_t pid;
	CYCLOPS_Matrix *backMat; //Matrix for storing the background
	uint8_t state;
} update_background_state_t;

//--------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------
static int8_t update_background_module(void *state, Message *e);
static int8_t input0 (func_cb_ptr p, token_type_t *t);

//static void destroy_cyclops_matrix(CYCLOPS_Matrix *m);

//--------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = UPDATE_BACKGROUND_PID,
  .code_id		  = ehtons(UPDATE_BACKGROUND_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(update_background_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 1,
  .num_prov_func  = 1,
  .module_handler = update_background_module,
  .funct			= {
	{error_8, "cyC2", UPDATE_BACKGROUND_PID, INVALID_GID},
	//{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
	{input0, "cyC2", UPDATE_BACKGROUND_PID, INPUT_PORT_0},
  },  
};

//--------------------------------------------------------
// MODULE HANDLER
//--------------------------------------------------------
static int8_t update_background_module(void *state, Message *msg)
{
	update_background_state_t *s = (update_background_state_t *) state;

	switch (msg->type){	
		case MSG_INIT:
		{					
			s->state = UPD_STATE_INIT;
			s->pid = msg->did;
			break;
		}
		case MSG_FINAL:
		{
			destroy_token_data(s->backMat, CYCLOPS_MATRIX, sizeof(CYCLOPS_Matrix));
			//destroy_cyclops_matrix(s->backMat);
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
	update_background_state_t *s = (update_background_state_t *)sys_get_state();	

	// Get token from port.

	if (s->state == UPD_STATE_INIT) {	

		s->backMat = (CYCLOPS_Matrix *)capture_token_data(t, s->pid);
		if (s->backMat == NULL) return -ENOMEM;
		
		s->state = UPD_STATE_PROCESS;								
	} else {		
		CYCLOPS_Matrix *M = (CYCLOPS_Matrix *)get_token_data(t);
		//check that input matrix's depth is 1 byte
		if( (M->depth != CYCLOPS_1BYTE) || (s->backMat->depth != CYCLOPS_1BYTE) )
				return -EINVAL;
		token_type_t *my_token = create_token(s->backMat, sizeof(CYCLOPS_Matrix), s->pid);
		if (my_token == NULL) return -EINVAL;
		set_token_type(my_token, CYCLOPS_MATRIX);
		//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
		dispatch(s->output0, my_token);
		destroy_token(my_token);
	}
	return SOS_OK;
}

/*
static void destroy_cyclops_matrix(CYCLOPS_Matrix *m) {
	if (m == NULL) return;
	ker_free_handle(m->data.hdl8);
	vire_free(m, sizeof(CYCLOPS_Matrix));
}
*/
//--------------------------------------------------------
#ifndef _MODULE_
mod_header_ptr update_background_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

