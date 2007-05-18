/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>

//#define LED_DEBUG
#include <led_dbg.h>
#include <hardware.h>
#include <matrix.h>
#include <adcm1700Control.h>
#include <adcm1700Const.h>
#include "../include/app_config.h"

#ifdef USE_SERIAL_DUMP
#include <serialDump.h>
#else
#include <radioDump.h>
#endif

// Private
#define TRANSMIT_MOD_PID (DFLT_APP_ID0 + 8)

// Element state.
// The pointers to function control blocks corresponding to
// the output(s) should appear first in the state. Other 
// function CB's should follow after that.
typedef struct {
  func_cb_ptr output0;
  //func_cb_ptr put_token;	//!< This function control block is used in SOS_CALL 
  							//!< when the element 
  							//!< needs to put a token on any output port.
							//!< The output port is identified by passing
							//!< one of the function control blocks defined
							//!< above (output0 or output1).
  func_cb_ptr signal_error;	//!< Used with split-phase operations to indicate
  							//!< an error to the wiring engine after accepting 
							//!< the input token.
  sos_pid_t pid;
} element_state_t;

static int8_t element_module(void *state, Message *msg);

// Function corresponding to the input port. Each port will
// have its own input function which will be published by the module.
// The prototypes of the input and output functions should match,
// else linking through the wiring engine will also not work.
// Since, all the input functions have a common prototype (for
// the indirect call through engine to work), the output
// functions should also have "cyC2" as their prototype.
// This is currently kept for simplicity, but can easily be
// used to enforce type checking at run-time too without any overhead.
// Split-phase operations should return -EBUSY after accepting
// an input token so that the wiring engine knows that the module
// will take long to complete that operation and it should queue
// all the tokens meant for that module till the module puts
// a result on any of its output ports.
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length);
static int8_t input0 (func_cb_ptr p, token_type_t *t);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = TRANSMIT_MOD_PID,
  .code_id		  = ehtons(TRANSMIT_MOD_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(element_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 2,
  .num_prov_func  = 1,
  .module_handler = element_module,
  .funct		  = {
		{error_8, "cyC2", TRANSMIT_MOD_PID, INVALID_GID},
        //{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
        {error_8, "ccv1", MULTICAST_SERV_PID, SIGNAL_ERR_FID},
		{input0, "cyC2", TRANSMIT_MOD_PID, INPUT_PORT_0},
  },  
};

static int8_t element_module(void *state, Message *msg) {
	element_state_t *s = (element_state_t *)state;

	switch (msg->type) {
		case MSG_INIT:
		{		
			s->pid = msg->did;
			break;
		}
		case MSG_FINAL:
		{
			break;
		}
#ifdef USE_SERIAL_DUMP
		case MSG_SERIAL_DUMP_DONE:
#else
		case MSG_RADIO_DUMP_DONE:
#endif
		{
			token_type_t *my_token = create_token(NULL, 0, s->pid);
			if (my_token == NULL) {
				SOS_CALL(s->signal_error, signal_error_func_t, -ENOMEM);
				break;
			}
			//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
			dispatch(s->output0, my_token);
			destroy_token(my_token);
			break;
		}
		default: return -EINVAL;
	}
  	return SOS_OK;
}

//--------------------------------------------------------

static int8_t input0 (func_cb_ptr p, token_type_t *t) {
	element_state_t *s = (element_state_t *)sys_get_state();

	CYCLOPS_Image *img = (CYCLOPS_Image *)capture_token_data(t, s->pid);
	if (img == NULL) return -ENOMEM;

#ifdef USE_SERIAL_DUMP
	if (sys_post(SERIAL_DUMP_PID, MSG_DUMP_BUFFER_TO_SERIAL,
					sizeof(CYCLOPS_Image), img, SOS_MSG_RELEASE) == SOS_OK) {
#else
	if (sys_post(RADIO_DUMP_PID, MSG_DUMP_BUFFER_TO_RADIO,
					sizeof(CYCLOPS_Image), img, SOS_MSG_RELEASE) == SOS_OK) {
#endif
		return -EBUSY;
	} else {
		destroy_token_data(img, CYCLOPS_IMAGE, sizeof(CYCLOPS_Image));
		return SOS_OK;
	}
}


//--------------------------------------------------------	
#ifndef _MODULE_
mod_header_ptr send_image_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

