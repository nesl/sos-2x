/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>
#include <string.h>

#define LED_DEBUG
#include <led_dbg.h>

#define BASE_NODE_ID 0

#define TEST_PID DFLT_APP_ID0
#define OTHER_PID DFLT_APP_ID1
/* this is a new message type which specifies our test driver's packet type
 * both the python test script, and the message handler will need to handle messages of this type
 */

#define MSG_TEST_DATA (MOD_MSG_START + 1)

/* this is the timer specifications */
#define TEST_APP_TID 0
#define TEST_APP_INTERVAL 50
/* a set of messages which the python script will be checking for.  the start and final data messages are sent
 * when this module beings and ends respectively.  the other two messages are used to signify when a specific test
 * has passed or failed on the mote.  If you wish to send other values, the python script will need a corresponding change
 */
#define START_DATA 100
#define FINAL_DATA 200
#define TEST_FAIL  155
#define TEST_PASS  255

/* if your driver has more than one sensor, or device, which can be polled
 * include more states here
 */
enum {
	TEST_APP_INIT=0,
	TEST_APP_FINAL,
};

/* if you wish to store more information, such as a history of previous values
 * include the appropriate data structure here
 */
typedef struct {
	uint8_t pid;
	uint8_t count;
	uint8_t state;
} app_state_t;

/* struct specifying how the data will be sent throug the network and the uart.  
 * this specifies an id, which wil be unique to each node
 * and the other field will hold the data recieved from the sensor
 * feel free to add more fields such as a packet counter or other information to assist
 * you testing
 */
typedef struct {
	uint8_t id;
	uint8_t state;
	uint8_t data;
} data_msg_t;

static int8_t generic_test_msg_handler(void *state, Message *msg);

/* for most tests, this won't need changing, except for possibly the name of the module handler
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = TEST_PID,
	.state_size     = sizeof(app_state_t),
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type = HW_TYPE,
	.processor_type = MCU_TYPE,
	.code_id = ehtons(TEST_PID),
	.module_handler = generic_test_msg_handler,
};

static int8_t generic_test_msg_handler(void *state, Message *msg)
{
	app_state_t *s = (app_state_t *) state;

	switch ( msg->type ) {

		/* do any initialization steps here, 
		 * in general it is good to set all the leds to off so that you can analyze what happens later more accurately
		 * also be sure to start and enable any timers which your driver might need 
		 */
		case MSG_INIT:
			sys_led(LED_GREEN_OFF);
			sys_led(LED_YELLOW_OFF);
			sys_led(LED_RED_OFF);

			s->state = TEST_APP_INIT;
			s->count = 0;
			s->pid = msg->did;

			sys_timer_start(TEST_APP_TID, TEST_APP_INTERVAL, SLOW_TIMER_REPEAT);
			break;

		case MSG_ERROR:
			s->state = TEST_APP_INIT;
			s->count = 0;
			s->pid = msg->did;

			sys_timer_start(TEST_APP_TID, TEST_APP_INTERVAL, SLOW_TIMER_REPEAT);
			break;

		case MSG_FINAL:
			sys_timer_stop(TEST_APP_TID);
			s->state = TEST_APP_FINAL;
			break;

		case MSG_TIMER_TIMEOUT:
			{
				switch(s->state){
				  case TEST_APP_INIT:
					  {
							s->count++;
							sys_led(LED_GREEN_TOGGLE);
							sys_led(LED_YELLOW_TOGGLE);
						}
						break;

					default:
						return -EINVAL;
						break;
				}
			} 
			break;

		default:
			return -EINVAL;
			break;
	}
	return SOS_OK;
}

#ifndef _MODULE_
mod_header_ptr generic_test_get_header() {
	return sos_get_header_address(mod_header);
}
#endif

