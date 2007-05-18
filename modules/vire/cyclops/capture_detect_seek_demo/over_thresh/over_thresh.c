/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @file over_thresh.c
 */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>
#define LED_DEBUG
#include <led_dbg.h>

#include <hardware.h>
#include <matrix.h>
#include <string.h>
#include "../include/app_config.h"

#define OVER_THRESH_PID (DFLT_APP_ID0 + 6)

//--------------------------------------------------------
// OVER THRESH STATE
//--------------------------------------------------------
typedef struct {
  func_cb_ptr output0;
  //func_cb_ptr put_token;
  uint8_t state;
  sos_pid_t pid;

  CYCLOPS_Matrix* A;
  uint8_t row;
  uint8_t col;
  uint8_t thresh;
} PACK_STRUCT over_thresh_state_t;


//--------------------------------------------------------
// STATIC FUNCTIONS
//--------------------------------------------------------
static int8_t over_thresh(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh);

//--------------------------------------------------------
// MODULE HANDLER PROTOTYPE
//--------------------------------------------------------
static int8_t over_thresh_module(void *state, Message *e);
//static CYCLOPS_Matrix* create_cyclops_matrix(sos_pid_t pid);
static void destroy_cyclops_matrix(CYCLOPS_Matrix *m);

// Functions for wiring configuration
static int8_t input0 (func_cb_ptr p, token_type_t *t);
static int8_t input1 (func_cb_ptr p, token_type_t *t);
static int8_t input2 (func_cb_ptr p, token_type_t *t);
static int8_t input3 (func_cb_ptr p, token_type_t *t);

//--------------------------------------------------------
// MODULE HEADER
//--------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = OVER_THRESH_PID,
  .code_id		  = ehtons(OVER_THRESH_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(over_thresh_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 1,
  .num_prov_func  = 4,
  .module_handler = over_thresh_module,
  .funct			= {
	{error_8, "cyC2", OVER_THRESH_PID, INVALID_GID},
	//{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID}, 	
	{input0, "cyC2", OVER_THRESH_PID, INPUT_PORT_0},
	{input1, "cyC2", OVER_THRESH_PID, INPUT_PORT_1},
	{input2, "cyC2", OVER_THRESH_PID, INPUT_PORT_2},
	{input3, "cyC2", OVER_THRESH_PID, INPUT_PORT_3},
  },  
};

//--------------------------------------------------------
// MODULE HANDLER
//--------------------------------------------------------
static int8_t over_thresh_module(void *state, Message *msg)
{
	over_thresh_state_t *s = (over_thresh_state_t *) state;

	switch (msg->type){		
		case MSG_INIT:
		{
			s->state = 0;
			s->pid = msg->did;
			s->A = NULL;
			s->row = 0;
			s->col = 0;
			s->thresh = 0;			

			break;
		}
		case MSG_FINAL:
		{
			destroy_cyclops_matrix(s->A);
			break;
		}
		default:
			return -EINVAL;
	}
	return SOS_OK;
}

//--------------------------------------------------------
static int8_t process_input(over_thresh_state_t *s) {
	uint8_t overTheThresh;
	objectInfo newObject;

	if (s->state < 4) {
		return -EBUSY;
	}

	s->state = 0;
	overTheThresh = over_thresh(s->A, s->row, s->col, RANGE, s->thresh);

	newObject.objectPosition.x = s->col;
	newObject.objectPosition.y = s->row;	  
	newObject.objectSize.x = ROWS;
	newObject.objectSize.y = COLS;	  
	newObject.actionFlag = NONE;

	if (overTheThresh > DETECT_THRESH) {
		LED_DBG(LED_RED_TOGGLE);	
		newObject.actionFlag = OBJ_DETECT;
	}  

	token_type_t *my_token = create_token(&newObject, sizeof(objectInfo), s->pid);
	if (my_token == NULL) return -ENOMEM;
	//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
	dispatch(s->output0, my_token);
	destroy_token(my_token);

	//destroy_cyclops_matrix(s->A);
	destroy_token_data(s->A, CYCLOPS_MATRIX, sizeof(CYCLOPS_Matrix));
	s->A = NULL;

	return SOS_OK;
}	

//--------------------------------------------------------
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	over_thresh_state_t *s = (over_thresh_state_t *)sys_get_state();

	if (t == NULL) return -EINVAL;

	// Get token from port.
	s->thresh = *((uint8_t *)get_token_data(t));
	s->state++;

	return process_input(s);
}

//--------------------------------------------------------
//static int8_t input1 (func_cb_ptr p, void *data, uint16_t length) 
static int8_t input1 (func_cb_ptr p, token_type_t *t) {
	over_thresh_state_t *s = (over_thresh_state_t *)sys_get_state();

	if (t == NULL) return -EINVAL;

	s->A = (CYCLOPS_Matrix *)capture_token_data(t, s->pid);
	if (s->A == NULL) return -ENOMEM;
	
	s->state++;

	return process_input(s);
}

//--------------------------------------------------------
//static int8_t input2 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input2 (func_cb_ptr p, token_type_t *t) {
	over_thresh_state_t *s = (over_thresh_state_t *)sys_get_state();

	if (t == NULL) return -EINVAL;

	// Get token from port.
	s->row = *((uint8_t *)get_token_data(t));
	s->state++;

	return process_input(s);
}

//--------------------------------------------------------
//static int8_t input3 (func_cb_ptr p, void *data, uint16_t length) {
static int8_t input3 (func_cb_ptr p, token_type_t *t) {
	over_thresh_state_t *s = (over_thresh_state_t *)sys_get_state();

	if (t == NULL) return -EINVAL;

	// Get token from port.
	s->col = *((uint8_t *)get_token_data(t));
	s->state++;

	return process_input(s);
}

//--------------------------------------------------------
static int8_t over_thresh(const CYCLOPS_Matrix* A, uint8_t row, uint8_t col, uint8_t range, uint8_t thresh)
{
  //check the depth, rows and cols
  if( (A->depth != CYCLOPS_1BYTE) || (A->rows < row) || (A->cols < col)){
	return 0xff;  //-EINVAL.  We used this instead of zero since zero might mean
	//that no pixel was above the threshold.
  }
  else{
	uint8_t startCol = col - range;
	uint8_t endCol = col + range;
	uint8_t startRow = row - range;
	uint8_t endRow = row + range;
	uint8_t i,j, counter=0; //counters
	
	uint8_t *A_ptr8;
	A_ptr8 = ker_get_handle_ptr (A->data.hdl8);
	if (A_ptr8 == NULL) return -EINVAL;
	if (row + range > A->rows)  //Boundary checks
	  endRow = A->rows;
	if (row < range)
	  startRow = 0;
	if (col + range > A->cols)
	  endCol = A->cols;
	if (col < range)
	  startCol = 0;
	for (i = startRow; i <=endRow; i++ )
	  for (j = startCol; j<=endCol; j++)
		if (A_ptr8[i * A->cols + j] > thresh)
		  counter++;
	return counter;
  }
}

static void destroy_cyclops_matrix(CYCLOPS_Matrix *m) {
	if (m == NULL) return;
	ker_free_handle(m->data.hdl8);
	vire_free(m, sizeof(CYCLOPS_Matrix));
}

//--------------------------------------------------------
#ifndef _MODULE_
mod_header_ptr over_thresh_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

