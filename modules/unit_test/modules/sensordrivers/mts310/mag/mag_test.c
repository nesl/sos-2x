/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>
#include <string.h>

#define LED_DEBUG
#include <led_dbg.h>

/* driver specific values
 * be sure to include the .h file for the driver you are testing, and assign DRIVER_0_ID to the drivers PID value
 * if the driver has more sensors to poll, include addition macros  such as DRIVER_1_ID
 */
#include <mts310sb.h>

#define TEST_APP_TID 2
#define TEST_APP_INTERVAL 20
#define DRIVER_0_ID MTS310_MAG_0_SID  
#define DRIVER_1_ID MTS310_MAG_1_SID

#define TEST_PID DFLT_APP_ID0

/* this is a new message type which specifies our test driver's packet type
 * both the python test script, and the message handler will need to handle messages of this type
 */

#define MSG_TEST_DATA (MOD_MSG_START + 1)

/* this is the number of bytes which will be sent to the sever
 * if you are going to be sending more, modify this value
 */
#define MSG_LEN 3

/* if your driver has more than one sensor, or device, which can be polled
 * include more states here
 */
enum {
	TEST_APP_INIT=0,
	TEST_APP_IDLE,
	TEST_APP_ACCEL_0,
	TEST_APP_ACCEL_0_BUSY,
	TEST_APP_ACCEL_1,
	TEST_APP_ACCEL_1_BUSY,
};

/* if you wish to store more information, such as a history of previous values
 * include the appropriate data structure here
 */
typedef struct {
	uint8_t pid;
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
	uint8_t data[MSG_LEN];
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
		 * in this case, we are starting  a timer and enabling a sensor
		 */
		case MSG_INIT:
			sys_led(LED_GREEN_OFF);
			sys_led(LED_YELLOW_OFF);
			sys_led(LED_RED_OFF);

			s->state = TEST_APP_INIT;
			s->pid = msg->did;
			sys_timer_start(TEST_APP_TID, TEST_APP_INTERVAL, SLOW_TIMER_REPEAT);
			if(sys_sensor_enable(DRIVER_0_ID) != SOS_OK) {
				sys_timer_stop(TEST_APP_TID);
			}
			break;

		/* disable and sensors your enables previously, and stop any timers which you started here
		 */
		case MSG_FINAL:
			sys_sensor_disable(DRIVER_0_ID);
			sys_timer_stop( TEST_APP_TID);
			break;

		/* be sure to have a case statement for each of your states that you have made 
		 * also be sure to get data from the driver when needed
		 * */
		case MSG_TIMER_TIMEOUT:
			{
			  //sys_led(LED_YELLOW_TOGGLE);
				switch (s->state) {
					case TEST_APP_INIT:
						// do any necessary init here
						s->state = TEST_APP_IDLE;
						break;

					case TEST_APP_IDLE:
						s->state = TEST_APP_ACCEL_0;
						break;

					/* copy and paste these two case statements and change the number if you have more states
					 */
					case TEST_APP_ACCEL_0:
						s->state = TEST_APP_ACCEL_0_BUSY;
						sys_sensor_get_data(DRIVER_0_ID);
						break;

					case TEST_APP_ACCEL_0_BUSY:
						break;

					case TEST_APP_ACCEL_1:
						s->state = TEST_APP_ACCEL_1_BUSY;
						sys_sensor_get_data(DRIVER_1_ID);
						break;

					case TEST_APP_ACCEL_1_BUSY:
						break;

				}
			}
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

	  /* when your data is ready, it is now time to be sent to the server either via the UART 
		 * or the network depending on which node this is
		 * the value for this case might be changed depending on which type value the sensor sends
		 */
		case MSG_DATA_READY:
			{
				/* out data message that will be sent through the uart or network */
				data_msg_t *data_msg;

				// create a message with a appropriate size
				data_msg = (data_msg_t *) sys_malloc ( sizeof(data_msg_t) );

				if ( data_msg ) {
				  sys_led(LED_GREEN_TOGGLE);

					// copy all the data you wish to send
					data_msg->id = sys_id();
					memcpy((void*)data_msg->data, (void*)msg->data, MSG_LEN);

					/* if you are running this test on multiple nodes at the same time you will need this
					 * but if you are running it on just one node at a time, you only need the call to sys_post_uart
					 */
					if (sys_id() == 0){
						sys_post_uart ( 
								s->pid,
								MSG_TEST_DATA,
								sizeof(data_msg_t),
								data_msg,
								SOS_MSG_RELEASE,
								BCAST_ADDRESS);
					} else {
						sys_post_net (
								s->pid, 
								MSG_TEST_DATA,
								sizeof(data_msg_t),
								data_msg,
								SOS_MSG_RELEASE,
								0);
					}
				} else
					sys_led(LED_RED_ON);

				/* be sure to reset your state machine to the state you want it to go back to after sending data
				 */
				switch(s->state) {
					case TEST_APP_ACCEL_0_BUSY:
						s->state = TEST_APP_ACCEL_1;
						break;
					case TEST_APP_ACCEL_1_BUSY:
						s->state = TEST_APP_ACCEL_0;
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

