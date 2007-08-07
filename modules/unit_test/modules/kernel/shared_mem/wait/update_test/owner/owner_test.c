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
#define MSG_DATA_WAIT (MOD_MSG_START + 2)
#define MSG_TRANS_READY (MOD_MSG_START + 3)

/* this is the timer specifications */
#define TEST_APP_TID 0
#define TEST_APP_INTERVAL 1024

/* messagees for when MSG_INIT and MSG_FINAL are sent
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
	TEST_APP_WAIT,
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

static int8_t send_new_data(uint8_t state, uint8_t data){
		/* out data message that will be sent through the uart or network */
		data_msg_t *data_msg;

		// create a message with a appropriate size
		data_msg = (data_msg_t *) sys_malloc ( sizeof(data_msg_t) );

		if ( data_msg ) {
			sys_led(LED_GREEN_TOGGLE);

			// copy all the data you wish to send
			data_msg->id = sys_id();
			data_msg->state = state;
			data_msg->data = data;

			/* if you are running this test on multiple nodes at the same time you will need this
			 * but if you are running it on just one node at a time, you only need the call to sys_post_uart
			 */
			if (sys_id() == 0){
				sys_post_uart ( 
						TEST_PID,
						MSG_TEST_DATA,
						sizeof(data_msg_t),
						data_msg,
						SOS_MSG_RELEASE,
						BCAST_ADDRESS);
			} else {
				sys_post_net (
						TEST_PID, 
						MSG_TEST_DATA,
						sizeof(data_msg_t),
						data_msg,
						SOS_MSG_RELEASE,
						BASE_NODE_ID);
			}
		} else
			sys_led(LED_RED_ON);

		return SOS_OK;
}

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
      send_new_data(START_DATA, 0);
			break;

		case MSG_ERROR:
			s->state = TEST_APP_INIT;
			s->count = 0;
			s->pid = msg->did;

			sys_timer_start(TEST_APP_TID, TEST_APP_INTERVAL, SLOW_TIMER_REPEAT);
			send_new_data(START_DATA, 0);
			break;

		case MSG_FINAL:
			sys_timer_stop(TEST_APP_TID);
			s->state = TEST_APP_FINAL;
			send_new_data(FINAL_DATA, 1);
			break;

	  /* here we handle messages of type MSG_TEST_DATA
		 * in most cases, only the base station node should be doing this since it is the only one connected to the uart
		 * if your test does not use multiple nodes, or your messages are sent via another module, this is not needed
		 */
		case MSG_TEST_DATA:
			{
				uint8_t *payload;
				uint8_t msg_len;

				msg_len = msg->len;
				payload = sys_msg_take_data(msg);

				sys_post_uart(
						s->pid, 
						MSG_TEST_DATA,
						msg_len,
						payload,
						SOS_MSG_RELEASE,
						BCAST_ADDRESS);
    	}
			break;

		case MSG_DATA_WAIT:
			{
				s->state = TEST_APP_WAIT;
			}
			break;

		case MSG_TIMER_TIMEOUT:
			{
				switch(s->state){
				  case TEST_APP_INIT:
					  {
							uint8_t *d;
							d = (uint8_t *) sys_malloc(sizeof(uint8_t));
							*d = s->count;

							sys_shm_open(sys_shm_name(TEST_PID, 0), d);
							sys_shm_open(sys_shm_name(TEST_PID, 1), d);

							s->state = TEST_APP_FINAL;
						}
						break;

					case TEST_APP_WAIT:
						{
							uint8_t *d;
							d = (uint8_t*) sys_shm_get(sys_shm_name(TEST_PID, 0));

							*d = s->count;

							sys_shm_update(sys_shm_name(TEST_PID, 0), d);
							sys_shm_update(sys_shm_name(TEST_PID, 1), d);
									
							s->count++;
						}
						break;

					case TEST_APP_FINAL:
						{
							sys_post_value(
									OTHER_PID,
									MSG_TRANS_READY,
									0,
									0);
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

