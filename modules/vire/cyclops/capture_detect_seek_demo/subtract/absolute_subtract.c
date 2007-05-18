/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file absolute_subtract.c
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

#define ABSOLUTE_SUBTRACT_PID (DFLT_APP_ID0 + 4)

enum {
	SUB_STATE_INIT = 0,
	SUB_STATE_IDLE,
	SUB_STATE_WAIT_1,
	SUB_STATE_FAIL,
};

//--------------------------------------------------------
// ABSOLUTE SUBTRACT
//--------------------------------------------------------
typedef struct {
  func_cb_ptr output0;
  //func_cb_ptr put_token;
  CYCLOPS_Matrix *objMat; 	//!< Matrix that stores the foreground
  CYCLOPS_Matrix *input;	//!< Input matrix A 
  uint8_t state;
  sos_pid_t pid;
} absolute_subtract_state_t;


//--------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------
static int8_t absolute_subtract_module(void *state, Message *e);
static int8_t input0 (func_cb_ptr p, token_type_t *t);
static int8_t input1 (func_cb_ptr p, token_type_t *t);
static CYCLOPS_Matrix* create_cyclops_matrix(sos_pid_t pid);
static void destroy_cyclops_matrix(CYCLOPS_Matrix *m);

//--------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = ABSOLUTE_SUBTRACT_PID,
  .code_id		  = ehtons(ABSOLUTE_SUBTRACT_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(absolute_subtract_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 1,
  .num_prov_func  = 2,
  .module_handler = absolute_subtract_module,
  .funct			= {
	{error_8, "cyC2", ABSOLUTE_SUBTRACT_PID, INVALID_GID},
	//{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
	{input0, "cyC2", ABSOLUTE_SUBTRACT_PID, INPUT_PORT_0},
	{input1, "cyC2", ABSOLUTE_SUBTRACT_PID, INPUT_PORT_1},
  },  
};

//--------------------------------------------------------
// MODULE HANDLER
//--------------------------------------------------------
static int8_t absolute_subtract_module(void *state, Message *msg)
{
	absolute_subtract_state_t *s = (absolute_subtract_state_t *) state;

	switch (msg->type){
		case MSG_INIT:
		{		
			s->state = SUB_STATE_INIT;
			s->input = NULL;			
			s->pid = msg->did;
			s->objMat = NULL;
			break;
		}
		case MSG_FINAL:
		{
			vire_free(s->input, sizeof(CYCLOPS_Matrix));
			destroy_cyclops_matrix(s->objMat);
			break;
		}
		default:
			return -EINVAL;
	}
	return SOS_OK;
}

static int8_t process_input(absolute_subtract_state_t *s, token_type_t *t) {
	switch (s->state) {
		case SUB_STATE_IDLE:
		{
			// Application specific hack.
			// Assuming the matrix pointer will stay the same.
			CYCLOPS_Matrix *in = (CYCLOPS_Matrix *)get_token_data(t);
			token_length_t length = get_token_length(t);
			if (in == NULL) return -EINVAL;

			s->input = (CYCLOPS_Matrix *)vire_malloc(length, s->pid);
			if (s->input == NULL) 
				return -ENOMEM;
			memcpy(s->input, in, length);
			s->state = SUB_STATE_WAIT_1;
			return -EBUSY;
		/*
			token_type_t *my_token = capture_token(t);
			if (my_token == NULL) return -ENOMEM;

			s->input = (CYCLOPS_Matrix *)get_token_data(my_token);
			if (s->input == NULL) return -EINVAL;

			free_token_header(my_token);
			s->state = SUB_STATE_WAIT_1;
			return -EBUSY;
		*/
		}
		case SUB_STATE_WAIT_1:
		{
			CYCLOPS_Matrix *A = (CYCLOPS_Matrix *)get_token_data(t);
			if (A == NULL) return -EINVAL;
			uint16_t i;			//used uint16_t, so max matrix size is 2^16
			uint16_t size = A->rows * A->cols;
			//only perform operations if all three matrices of same size
			if ((A->rows != s->input->rows) || 
				(A->cols != s->input->cols)) { 
					return -EINVAL;
			} else {		
				uint8_t *A_ptr8;
				uint8_t *B_ptr8;
				uint8_t *C_ptr8;
				s->objMat = create_cyclops_matrix(s->pid);
				if (s->objMat == NULL) {
					s->state = SUB_STATE_FAIL;
					return -ENOMEM;
				}
				A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
				B_ptr8 = ker_get_handle_ptr (s->input->data.hdl8);
				C_ptr8 = ker_get_handle_ptr (s->objMat->data.hdl8);

				if ((A_ptr8 == NULL) || (B_ptr8 == NULL) || (C_ptr8 == NULL)) {
					return -EINVAL;
				}
				
				if (A->depth == CYCLOPS_1BYTE && s->input->depth == CYCLOPS_1BYTE
								&& s->objMat->depth == CYCLOPS_1BYTE) {
						// adding two matrices with elements uint8_t, 
						// and result also uint8_t                    
						for (i = 0; i < size; i++) {
								if (A_ptr8[i] < B_ptr8[i])
										C_ptr8[i] = B_ptr8[i] - A_ptr8[i];
								else
										C_ptr8[i] = A_ptr8[i] - B_ptr8[i];
						}		    
						// Putting data on a port
						token_type_t *my_token = create_token(s->objMat, sizeof(CYCLOPS_Matrix), s->pid);
						if (my_token == NULL) return -ENOMEM;
						set_token_type(my_token, CYCLOPS_MATRIX);
						release_token(my_token);
						//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
						dispatch(s->output0, my_token);
						destroy_token(my_token);

						vire_free(s->input, sizeof(CYCLOPS_Matrix));
						s->input = NULL;
						s->objMat = NULL;
						s->state = SUB_STATE_IDLE;

						return SOS_OK;
				} else {
					return -EINVAL;
				}
			}	

			break;
		}
		default:
			return -EINVAL;
	}

	return SOS_OK;
}
  
//--------------------------------------------------------
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length) 
static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	absolute_subtract_state_t *s = (absolute_subtract_state_t *)sys_get_state();

	if (s->state == SUB_STATE_FAIL) return -EINVAL;

	return process_input(s, t);
}

//--------------------------------------------------------
//static int8_t input1 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input1 (func_cb_ptr p, token_type_t *t) {
	absolute_subtract_state_t *s = (absolute_subtract_state_t *)sys_get_state();

	if (s->state == SUB_STATE_FAIL) return -EINVAL;

  	if (s->state == SUB_STATE_INIT) {
		s->state = SUB_STATE_IDLE;				
		return SOS_OK;
	}

	return process_input(s, t);
}
	
//--------------------------------------------------------	
static CYCLOPS_Matrix* create_cyclops_matrix(sos_pid_t pid)
{
	CYCLOPS_Matrix* pMat;
	uint16_t matrixSize;

	pMat = (CYCLOPS_Matrix*)vire_malloc(sizeof(CYCLOPS_Matrix), pid);
	if (pMat == NULL) return NULL;

	pMat->rows = ROWS;
	pMat->cols = COLS;
	pMat->depth= CYCLOPS_1BYTE;

	matrixSize = (pMat->rows)*(pMat->cols)*(pMat->depth);
	pMat->data.hdl8 = ker_get_handle(matrixSize, pid);
	if (pMat->data.hdl8 < 0) {
			vire_free(pMat, sizeof(CYCLOPS_Matrix));
			return NULL;
	}
	return pMat;
}

static void destroy_cyclops_matrix(CYCLOPS_Matrix *m) {
	if (m == NULL) return;
	ker_free_handle(m->data.hdl8);
	vire_free(m, sizeof(CYCLOPS_Matrix));
}

//--------------------------------------------------------
#ifndef _MODULE_
mod_header_ptr absolute_subtract_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

