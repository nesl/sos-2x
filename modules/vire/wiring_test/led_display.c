/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>

#define LED_DEBUG
#include <led_dbg.h>

// Private
#define LED_DISP_MOD_PID DFLT_APP_ID3

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
  .mod_id         = LED_DISP_MOD_PID,
  .code_id		  = ehtons(LED_DISP_MOD_PID),
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
		{input0, "cyC2", LED_DISP_MOD_PID, INPUT_PORT_0},
  },  
};

static int8_t element_module(void *state, Message *msg) {
	switch (msg->type) {
		case MSG_INIT:
		{		
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
	uint8_t value = *((uint8_t *)get_token_data(t));

	LED_DBG(LED_RED_OFF);
	LED_DBG(LED_GREEN_OFF);
	LED_DBG(LED_YELLOW_OFF);

	if (value & 0x01) LED_DBG(LED_RED_ON);
	if (value & 0x02) LED_DBG(LED_GREEN_ON);
	if (value & 0x04) LED_DBG(LED_YELLOW_ON);
	return SOS_OK;
}

//--------------------------------------------------------	
#ifndef _MODULE_
mod_header_ptr led_disp_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

