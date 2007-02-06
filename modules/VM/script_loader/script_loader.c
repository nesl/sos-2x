/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <module.h>
 */
#include <module.h>
#include <sys_module.h>
#include <codemem.h>
#include <loader/loader.h>

/*
 * TODO: change mod_id to correct value.
 */
#define THIS_MODULE_ID  DFLT_APP_ID3
/**
 * Module can define its own state
 */
typedef struct {
	// TODO: Define module specific state
} app_state_t;

/*
 * Forward declaration of module 
 */
static int8_t module(void *state, Message *e);

/*
 * This is the only global variable one can have.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = THIS_MODULE_ID,
	.state_size     = 0 /*sizeof(app_state_t)*/,
    .num_timers     = 0,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(THIS_MODULE_ID),
	.module_handler = module,
};


static void load_script( uint8_t *script, uint8_t size );
static int8_t module(void *state, Message *msg)
{
    //app_state_t *s = (app_state_t *) state;

    /**
     * Switch to the correct message handler
     */
    switch (msg->type){

        case MSG_INIT:
		{
			//sys_timer_start(0, 5L, TIMER_ONE_SHOT);			
			// TODO: Initialize module specific state.
			//uint8_t script_buf[17] = { 0x38,0x1,0x4,0x2,0xa,0x0,0x0,0x4,0x1f,0x65,0x4,0x1,0x20,0x1,0x80,0x11,0x0, };
			// timer1 no NOP
			//uint8_t script_buf[11] = { 0x38,0x1,0x4,0x2,0x4,0x0,0x0,0x1,0x1a,0x3,0x0,  };
			// timer1 with NOP
			//uint8_t script_buf[12] = { 0x38,0x1,0x4,0x2,0x5,0x0,0x0,0x1,0x1a,0x3,0x70,0x0, };
			// timer1 with get_data and bcast
			//uint8_t script_buf[20] = { 0x38,0x1,0x4,0x2,0xd,0x0,0x0,0x1,0x1a,0x3,0x4,0x1f,0x65,0x24,0x1,0x20,0x1,0x80,0x11,0x0, };
			uint8_t script_buf[48] = { 0x38,0x1,0x4,0x2,0x29,0x0,0x0,0x4,0x30,0xc,0x41,0x13,0x5,0x41,0x55,0x2,0x5,0x1f,0x66,0x24,0x41,0x55,0x4,0x1f,0x65,0x1,0x1f,0x4,0x2a,0x19,0x2,0x0,0x1,0x0,0x16,0x70,0x1,0x20,0x1,0x80,0x11,0x1,0x0,0x65,0x1,0x20,0x16,0x0, };
			// reboot context
			uint8_t script_buf2[11] = { 0x38,0x0,0x0,0x0,0x4,0x0,0x0,0x1,0x20,0x16,0x0, };
			DEBUG("load_script\n");
			load_script(script_buf, 48);
			load_script(script_buf2, 11);
			return SOS_OK;
		}
		case MSG_TIMER_TIMEOUT:
		{
			//uint8_t script_buf[11] = { 0x38,0x1,0x4,0x2,0x4,0x0,0x0,0x1,0x1a,0x3,0x0, };
			/*
			uint8_t script_buf[17] = { 0x38,0x1,0x4,0x2,0xa,0x0,0x0,0x4,0x1f,0x65,0x4,0x1,0x20,0x1,0x80,0x11,0x0, };
			uint8_t script_buf2[11] = { 0x38,0x0,0x0,0x0,0x4,0x0,0x0,0x1,0x13,0x16,0x0, };
			DEBUG("load_script\n");
			load_script(script_buf, 17);
			load_script(script_buf2, 11);
			*/
			
			// Use timer_get_tid( msg ) to get the timer instance that was fired
			return SOS_OK;
		}
        case MSG_FINAL:
        {
			// TODO: free up the resource used by the module.  
			return SOS_OK;
        }
        default:
			return -EINVAL;
	}

    /**
     * Return SOS_OK for those handlers that have successfully been handled.
     */
    return SOS_OK;
}

static void load_script( uint8_t *script, uint8_t size )
{
	codemem_t cm = ker_codemem_alloc( size, CODEMEM_TYPE_EXECUTABLE );

	ker_codemem_write( cm, THIS_MODULE_ID, script, size, 0 );

	ker_codemem_flush( cm, THIS_MODULE_ID);

	post_short(script[0], THIS_MODULE_ID, MSG_LOADER_DATA_AVAILABLE,
			script[1], cm, 0);
}

#ifndef _MODULE_
// TODO: rename module_template_get_header() to <module_name>_get_header() 
// to avoid name collision.  
mod_header_ptr script_loader_get_header()
{
    return sos_get_header_address(mod_header);
}
#endif

