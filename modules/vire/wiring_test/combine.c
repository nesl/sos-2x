/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>

//#define LED_DEBUG
#include <led_dbg.h>

// Private
#define COMBINE_MOD_PID 	(DFLT_APP_ID0+5)

enum {
	IDLE	= 0,
	WAITING,
};

// Element state.
// The pointers to function control blocks corresponding to
// the output(s) should appear first in the state. Other 
// function CB's should follow after that.
typedef struct {
	func_cb_ptr output0;		//!< Function control block for output port 0
	//func_cb_ptr put_token;		//!< This function control block is used in SOS_CALL 
								//!< when the element 
								//!< needs to put a token on any output port.
								//!< The output port is identified by passing
								//!< one of the function control blocks defined
								//!< above (output0 or output1).
	func_cb_ptr signal_error;	//!< Used with split-phase operations to indicate
								//!< an error to the wiring engine after accepting 
								//!< the input token.
	uint8_t status;
	sos_pid_t pid;
	uint8_t stored_input;		//!< Global variable to hold one input while waiting for
								//!< the other.
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
//static int8_t input1 (func_cb_ptr p, void *data, uint16_t length);
static int8_t input0 (func_cb_ptr p, token_type_t *t);
static int8_t input1 (func_cb_ptr p, token_type_t *t);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = COMBINE_MOD_PID,
  .code_id		  = ehtons(COMBINE_MOD_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(element_state_t),
  .num_out_port	  = 1,
  .num_sub_func   = 2,
  .num_prov_func  = 2,
  .module_handler = element_module,
  .funct		  = {
        {error_8, "cyC2", COMBINE_MOD_PID, INVALID_GID},
        //{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
        {error_8, "ccv1", MULTICAST_SERV_PID, SIGNAL_ERR_FID},
		{input0, "cyC2", COMBINE_MOD_PID, INPUT_PORT_0},
		{input1, "cyC2", COMBINE_MOD_PID, INPUT_PORT_1},
  },  
};

static int8_t element_module(void *state, Message *msg) {
	element_state_t *s = (element_state_t *)state;

	switch (msg->type) {
		case MSG_INIT: 
		{	
			s->status = IDLE;
			s->stored_input = 0;
			s->pid = msg->did;
			break;
	    }
		case MSG_FINAL:
		{
		   	break;
		}
		default: return -EINVAL;
	}
	return SOS_OK;
}

//--------------------------------------------------------
static int8_t process_input(element_state_t *s, uint8_t data) {
	switch(s->status) {
		case IDLE:
		{
			s->stored_input = data;
			s->status = WAITING;
			return -EBUSY;
		}
		case WAITING:
		{
			// Process input: Combine (OR) the two inputs and pass the result on to the next function.
			// We need a separate place to hold the output as we are modifying the input.
			// Remember, this module does not own the input token, so should not
			// overwrite it.
			uint8_t out_value = data | s->stored_input;
			token_type_t *my_token = create_token(&out_value, sizeof(uint8_t), s->pid);
			if (my_token == NULL) return -ENOMEM;
			//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
			dispatch(s->output0, my_token);
			s->status = IDLE;
			destroy_token(my_token);
			return SOS_OK;
		}
		default:
			break;
	}

	return -EINVAL;
}

static int8_t input0 (func_cb_ptr p, token_type_t *t) {
  	element_state_t *s = (element_state_t *)sys_get_state();

	return process_input(s, *((uint8_t *)get_token_data(t)));
}

static int8_t input1 (func_cb_ptr p, token_type_t *t) {
  	element_state_t *s = (element_state_t *)sys_get_state();

	return process_input(s, *((uint8_t *)get_token_data(t)));
}

//--------------------------------------------------------	
#ifndef _MODULE_
mod_header_ptr combine_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

