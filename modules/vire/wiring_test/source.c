/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <module.h>
#include <sys_module.h>
#include <wiring_config.h>

//#define LED_DEBUG
#include <led_dbg.h>

// Private
#define SOURCE_MOD_PID 	DFLT_APP_ID0
#define TIMER_TID		0

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
	uint8_t cnt;
	sos_pid_t pid;
	uint8_t sample_rate_in_sec;	//!< Exposed parameter. Controls timer period of application.
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
// all the tokens meant for that port till the module puts
// a result on any of its output ports.
//static int8_t input0 (func_cb_ptr p, void *data, uint16_t length);

// Each module that wants to provide access to its parameters for
// update should publish this function. The parameters will be
// bundled in the order as defined by its corresponding MoML file
// in Ptolemy.
static int8_t update_param (func_cb_ptr p, void *data, uint16_t length);

static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = SOURCE_MOD_PID,
  .code_id		  = ehtons(SOURCE_MOD_PID),
  .platform_type  = HW_TYPE,
  .processor_type = MCU_TYPE,  
  .state_size     = sizeof(element_state_t),
  .num_out_port   = 1,
  .num_sub_func   = 2,
  .num_prov_func  = 1,
  .module_handler = element_module,
  .funct		  = {
        {error_8, "cyC2", SOURCE_MOD_PID, INVALID_GID},
        //{error_8, "cyC4", MULTICAST_SERV_PID, DISPATCH_FID},
        {error_8, "ccv1", MULTICAST_SERV_PID, SIGNAL_ERR_FID},
		{update_param, "cwS2", SOURCE_MOD_PID, UPDATE_PARAM_FID},
  },  
};

static int8_t element_module(void *state, Message *msg) {
	element_state_t *s = (element_state_t *)state;

	switch (msg->type) {
		case MSG_INIT: 
		{		
			LED_DBG(LED_RED_ON);
			s->cnt = 0;
			// Default parameter value = 5 sec.
			s->sample_rate_in_sec = 5;
			s->pid = msg->did;
			sys_timer_start(TIMER_TID, ((uint32_t)s->sample_rate_in_sec) * 1024L, TIMER_ONE_SHOT);
			break;
	    }
		case MSG_TIMER_TIMEOUT:
		{
			LED_DBG(LED_RED_TOGGLE);
			s->cnt++;
			DEBUG("Timer fired. Put token %d on output port. Function CB output = 0x%x.\n", 
														s->cnt, s->output0);
			token_type_t *my_token = create_token(&s->cnt, sizeof(uint8_t), s->pid);
			if (my_token == NULL) return -ENOMEM;
			//SOS_CALL(s->put_token, put_token_func_t, s->output0, my_token);
			dispatch(s->output0, my_token);
			destroy_token(my_token);
			sys_timer_start(TIMER_TID, ((uint32_t)s->sample_rate_in_sec) * 1024L, TIMER_ONE_SHOT);
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

static int8_t update_param (func_cb_ptr p, void *data, uint16_t length) {
	element_state_t *s = (element_state_t *)sys_get_state();
	s->sample_rate_in_sec = *((uint8_t *)data);
	DEBUG("Sample rate updated to %d seconds.\n", s->sample_rate_in_sec);
	return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr source_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

