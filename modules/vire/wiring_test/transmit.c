/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>

//#define LED_DEBUG
#include <led_dbg.h>

// Private
#define TRANSMIT_MOD_PID DFLT_APP_ID1

// Element state.
// The pointers to function control blocks corresponding to
// the output(s) should appear first in the state. Other 
// function CB's should follow after that.
typedef struct {
  //func_cb_ptr put_token;	//!< This function control block is used in SOS_CALL 
  							//!< when the element 
  							//!< needs to put a token on any output port.
							//!< The output port is identified by passing
							//!< one of the function control blocks defined
							//!< above (output0 or output1).
  func_cb_ptr signal_error;	//!< Used with split-phase operations to indicate
  							//!< an error to the wiring engine after accepting 
							//!< the input token.
  uint8_t value;
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
static int8_t input0 (func_cb_ptr p, token_type_t *t);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = TRANSMIT_MOD_PID,
  .code_id		  = ehtons(TRANSMIT_MOD_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(element_state_t),
  .num_out_port   = 0,
  .num_sub_func   = 1,
  .num_prov_func  = 1,
  .module_handler = element_module,
  .funct		  = {
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
			s->value = 0;
			LED_DBG(LED_GREEN_ON);
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

static int8_t input0 (func_cb_ptr p, token_type_t *t) {
  element_state_t *s = (element_state_t *)sys_get_state();

  s->value = *((uint8_t *)get_token_data(t));

  // Get token from port.
  // If want to break the chain of calls here, then copy the token into a private
  // data structure(global), and return appropriate value (SOS_OK).

  //SOS_CALL(s->put_token, put_token_func_t, s->output0, &threshold, sizeof(uint8_t));
  LED_DBG(LED_GREEN_TOGGLE);

  sys_post_uart(TRANSMIT_MOD_PID, 0x80, sizeof(uint8_t), &(s->value), 0, BCAST_ADDRESS);
  return SOS_OK;
}

//--------------------------------------------------------	
#ifndef _MODULE_
mod_header_ptr transmit_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

