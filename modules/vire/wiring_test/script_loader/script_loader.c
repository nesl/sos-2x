/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * Module needs to include <module.h>
 */
#include <module.h>
#include <sys_module.h>
#include <codemem.h>
#include <loader.h>


#define THIS_MODULE_ID  DFLT_APP_ID3


static int8_t module(void *state, Message *e);


static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         = THIS_MODULE_ID,
  .state_size     = 0,
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
	switch (msg->type){

		case MSG_INIT: {
#ifdef WIRING_TEST_STAGE_1
			uint8_t reboot_script_buf[26] = { 22, 0x80,
				5,18,0,0,0,0,
				1,128,0,0,0,0,
				2,129,0,0,0,3,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 26);
#else
#ifdef WIRING_TEST_STAGE_2
			uint8_t reboot_script_buf[44] = { 22,0x80,
				5,36,0,0,0,0,
				1,128,0,0,0,0,
				2,129,0,0,0,0,
				2,130,0,0,0,3,
				1,130,0,0,0,3,
				2,131,0,0,0,3,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 44);
#else
#ifdef WIRING_TEST_STAGE_3
			uint8_t reboot_script_buf[38] = { 22,0x80,
				5,30,0,0,0,0,
				1,128,0,0,0,0,
				2,131,0,0,0,3,
				1,128,0,1,0,2,
				2,129,0,0,0,3,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 38);
#else
#ifdef WIRING_TEST_STAGE_4
			uint8_t reboot_script_buf[62] = { 22,0xC0,
				5,36,0,0,0,0,
				1,128,0,0,0,0,
				2,132,0,0,0,0,
				2,131,0,0,0,3,
				1,132,0,0,0,3,
				2,129,0,0,0,3,
				6,18,0,0,0,0,
				7,128,0,0,1,3,
				7,132,0,0,1,4,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 62);
#else
#ifdef WIRING_TEST_STAGE_5
			uint8_t reboot_script_buf[99] = { 22,0xC0,
				5,66,0,0,0,0,
				1,128,0,0,0,0,
				2,130,0,0,0,0,
				2,131,0,0,0,0,
				2,132,0,0,0,3,
				1,130,0,0,0,0,
				2,133,0,0,0,3,
				1,132,0,0,0,0,
				2,133,0,0,1,3,
				1,133,0,0,0,0,
				2,129,0,0,0,3,
				6,25,0,0,0,0,
				7,128,0,0,1,3,
				7,132,0,0,2,4,0x07,
				7,130,0,0,1,0xF8,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 99);
#else
#ifdef WIRING_TEST_STAGE_6
			uint8_t reboot_script_buf[124] = { 22,0xC0,
				5,84,0,0,0,0,
				1,128,0,0,0,0,
				2,130,0,0,0,0,
				2,131,0,0,0,0,
				2,132,0,0,0,0,
				2,132,0,1,0,3,
				1,130,0,0,0,0,
				2,134,0,0,0,3,
				1,132,0,0,0,0,
				2,134,0,0,1,3,
				1,134,0,0,0,0,
				2,129,0,0,0,3,
				1,132,0,1,0,0,
				2,134,0,0,2,3,
				6,32,0,0,0,0,
				7,128,0,0,1,3,
				7,132,0,0,2,4,0x07,
				7,130,0,0,1,0xE0,
				7,132,0,1,2,5,0x18,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 124);
#else
			uint8_t reboot_script_buf[38] = { 22, 0x80,
				5,30,0,0,0,0,
				1,128,0,0,0,0,
				2,130,0,0,0,0,
				2,131,0,0,0,0,
				2,129,0,0,0,3,
				4,0,0,0,0,0, };
			load_script(reboot_script_buf, 38);
#endif //stage 6
#endif //stage 5
#endif //stage 4
#endif //stage 3
#endif //stage 2
#endif //stage 1

			break;
		}
		case MSG_TIMER_TIMEOUT: {
			break;
		}
		case MSG_FINAL: {
			break;
		}
		default: return -EINVAL;
	}

	return SOS_OK;
}

static void load_script( uint8_t *script, uint8_t size )
{
  codemem_t cm = ker_codemem_alloc(size, CODEMEM_TYPE_EXECUTABLE);
  ker_codemem_write( cm, THIS_MODULE_ID, script, size, 0);
  ker_codemem_flush( cm, THIS_MODULE_ID);
  post_short(script[0], THIS_MODULE_ID, MSG_LOADER_DATA_AVAILABLE, script[1], cm, 0);
  return;
}

#ifndef _MODULE_
mod_header_ptr script_loader_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif

