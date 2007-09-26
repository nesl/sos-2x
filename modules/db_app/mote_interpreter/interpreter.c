/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include <sys_module.h>
#include <string.h>
#include <routing/tree_routing/tree_routing.h>
#define LED_DEBUG
#include <led_dbg.h>

#define MSG_NEW_QUERY (MOD_MSG_START + 1)
#define MSG_QUERY_REPLY (MOD_MSG_START + 2)

#define NUM_SENSORS 8
#define MOTE_INTERPRETER_PID DFLT_APP_ID0

enum {
	EQUAL_OP = 1,
	GREATER_THAN_OP,
	LESS_THAN_OP
};

/* a qid of 0 declares that this particular query is unused
 * a compare_op of 0 declares that there is no comparison needed
 * if compare_op is non-zero, then we will check against compare_val
 * the interval simply declares the period of each timer
 * and num_remaining declares how many samples are left for this query
 */
typedef struct {
  uint16_t qid;     //the query id
  uint8_t  compare_op; // operators to compare values against
  uint16_t  compare_val; // value to compare against
  uint32_t interval;
  uint16_t num_remaining;
} sensor_query_t;

// this holds the the query information for each sensor
// we will use the sensor id to also be the timer id for each 
// query, when it comes in
typedef struct {
  uint8_t pid;
  sensor_query_t queries[NUM_SENSORS];
	func_cb_ptr get_hdr_size;
} mote_state_t;

typedef struct {
	uint8_t sid;
	uint16_t qid;
	uint16_t num_remaining;
	uint8_t sensor;
	uint16_t value;
} query_result_t;

typedef struct {
	uint8_t sensor;
	uint16_t value;
} sensor_msg_t;

/* this part of the message that will be recieved by the mote
 * it describes all the information needed for a particular query
 * and the values match that of sensor_query_t in meaing
 */
typedef struct {
	uint8_t stype_and_op;
	uint16_t compare_val;
} query_details_t;

/* this will be the data format for message types of MSG_NEW_QUERY
 * which will be sent from the master node
 */
typedef struct {
	uint16_t qid;
	uint32_t interval;
	uint16_t num_samples;
	uint8_t num_sensor_query;
	query_details_t queries[];
} new_query_msg_t;

static int8_t interpreter_msg_handler(void *state, Message *msg);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id = MOTE_INTERPRETER_PID,
    .state_size = sizeof(mote_state_t),
    .num_timers = 0,
    .num_sub_func = 0,
    .num_prov_func = 0,
    .platform_type = HW_TYPE,
    .processor_type = MCU_TYPE,
    .code_id = ehtons(MOTE_INTERPRETER_PID),
    .module_handler = interpreter_msg_handler,
	//	.funct = {
	//		[0] = {error_8, "Cvv0", TREE_ROUTING_PID, MOD_GET_HDR_SIZE_FID},
	//	},
};

static int8_t reply_sender(uint16_t qid, sensor_msg_t *msg){
	query_result_t *d;
	uint8_t *pkt;
	int8_t hdr_size;
	
	DEBUG("<INTERPRETER> sending data through tree routing\n");
		mote_state_t *s;

		//sys_led(LED_GREEN_TOGGLE);

		s = (mote_state_t*) sys_get_state();
		DEBUG("<INTERPRETER> GET STATE ok\n");

    hdr_size = sizeof(tr_hdr_t);
		DEBUG("<INTERPRETER> get header size ok, size = %d\n", hdr_size);
		if (hdr_size < 0) {return SOS_OK;}

		pkt = (uint8_t *) sys_malloc(hdr_size + sizeof(query_result_t));
		DEBUG("<interpreter> malloc ok\n");
		d = (query_result_t *) (pkt+hdr_size);
		d->sid = sys_id();
		d->qid = qid;
		d->num_remaining = s->queries[msg->sensor].num_remaining;
		d->sensor = msg->sensor;
		d->value = msg->value;

		DEBUG("<MOTE INTERPRETER> Sending sensor data: \nsid = %d\nqid = %d\nsensor = %d\nvalue = %d\npacket size = %d\n",
				d->sid,
				d->qid,
				d->sensor,
				d->value,
				hdr_size + sizeof(query_result_t));

		sys_post(TREE_ROUTING_PID, MSG_SEND_PACKET, hdr_size + sizeof(query_result_t), 
				(void *) pkt, SOS_MSG_RELEASE);
	return SOS_OK;
}

static int8_t interpreter_msg_handler(void *state, Message *msg){
    mote_state_t *s = (mote_state_t *) state;

    switch(msg->type){
			case MSG_INIT:
				{
					uint8_t i;
					s->pid = msg->did;

					// initialize all the queries to be unused
					// and enable all the sensors
					for (i = 0; i < NUM_SENSORS; i++){
						s->queries[i].qid = 0;
#ifndef SOS_SIM 
						sys_sensor_enable(i);
#endif
					}

					s->queries[4].qid = 2;
					s->queries[4].compare_op = 0;
					s->queries[4].interval = 1024; 
					s->queries[4].num_remaining = 20;

          sys_timer_start(4, 1024, TIMER_REPEAT);
					sys_led(LED_RED_OFF);
					sys_led(LED_YELLOW_OFF);
					sys_led(LED_GREEN_OFF);
				}
				break;

			case MSG_NEW_QUERY:
				{
					//if (msg->data == NULL)
						//sys_led(LED_YELLOW_TOGGLE);
					
					uint8_t msg_len = msg->len; 
					new_query_msg_t *new_query = (new_query_msg_t *) sys_msg_take_data(msg);

					//if (new_query == NULL)
						//sys_led(LED_RED_TOGGLE);

					uint8_t i;

					sys_led(LED_GREEN_TOGGLE);

					for (i = 0; i < new_query->num_sensor_query; i++){
						query_details_t *q = &( new_query->queries[i] );
						uint8_t sensor_type = (q->stype_and_op & 0xF0) >> 4;
						sensor_query_t *sq = &( s->queries[sensor_type] );

						if (sq->qid == 0){
							sq->qid = new_query->qid;
							sq->compare_op = (q->stype_and_op & 0x0f);
							sq->interval = new_query->interval;
							sq->num_remaining = new_query->num_samples;
							sq->compare_val = q->compare_val;

							sys_timer_start(sensor_type, sq->interval, TIMER_REPEAT);
						} else {
							//send error message, saying that we can't service this query
						}
					}
					sys_post(TREE_ROUTING_PID, MSG_SEND_TO_CHILDREN, msg_len, new_query, SOS_MSG_RELEASE);
				}
				break;

			case MSG_TIMER_TIMEOUT:
				{
					/* for this message type, the byte value in MsgParam holds the timer id
					 * we can now also use this as the sensor id, and as an index for our query type
					 */
					MsgParam *param = (MsgParam *) msg->data;

					DEBUG("<INTERPRETER> Timer Timeout, for sensor %d\n", param->byte);

					//sys_led(LED_YELLOW_TOGGLE);
					if (param->byte > NUM_SENSORS)
						break;

					s->queries[param->byte].num_remaining--;
					if (s->queries[param->byte].num_remaining > 0)
#ifdef SOS_SIM 
						sys_post_value(MOTE_INTERPRETER_PID, MSG_DATA_READY, 0x00ffff04, 0);
#else
						sys_sensor_get_data(param->byte);
#endif
					else{ 
						s->queries[param->byte].qid = 0;
						sys_timer_stop(param->byte);
					}
				}
				break;

			case MSG_DATA_READY:
				{
					sensor_msg_t *data = (sensor_msg_t *)sys_msg_take_data(msg);
					sensor_query_t *sq;

					//sys_led(LED_RED_TOGGLE);

					sq = &(s->queries[data->sensor]);

					DEBUG("<INTERPRETER> data ready for sensor: %d with value = %d\n", data->sensor, data->value);
					if (sq->compare_op == 0){
						DEBUG("<INTERPRETER> compare ok\n");
						reply_sender(sq->qid, data);
						// send the value
				  } else {
						switch(sq->compare_op){
							case LESS_THAN_OP:
								if (data->value < sq->compare_val){
									reply_sender(sq->qid, data);
									// send the value
								}
								break;
							case GREATER_THAN_OP:
								if (data->value > sq->compare_val){
									reply_sender(sq->qid, data);
									// send the value
								}
								break;
						  case EQUAL_OP:
								if (data->value == sq->compare_val){
									reply_sender(sq->qid, data);
									//send the value
								}
								break;
							default:
								break;
						}
					}
					sys_free(data);
				}
				break;

			case MSG_TR_DATA_PKT:
				{
					if (sys_id() == BASE_STATION_ADDRESS){
						uint8_t *payload;
						uint8_t msg_len;
						int8_t hdr_size;

					  sys_led(LED_GREEN_TOGGLE);
						msg_len = msg->len;
						payload = sys_msg_take_data(msg);

						query_result_t *reply;
						hdr_size = sizeof(tr_hdr_t);
						
						reply = (query_result_t*)(payload + hdr_size);

						DEBUG("<MOTE INTERPRETER> tree routing packet recieved, size = %d\n",msg_len);

#ifndef SOS_SIM
						//sys_post_uart(s->pid, MSG_QUERY_REPLY, sizeof(query_result_t), reply, 0, BCAST_ADDRESS);
						sys_post_uart(s->pid, MSG_QUERY_REPLY, msg_len, payload, SOS_MSG_RELEASE, BCAST_ADDRESS);
#else
						DEBUG("<INTERPRETED> data recieved with: sensor=%d\nvalue=%d\nqid=%d\n", reply->sensor, reply->value, reply->qid);
#endif
						//sys_free(payload);
					}
				}
				break;

			default:
				return -EINVAL;
		}
		// at this point, maybe go into power saving mode?
		return SOS_OK;

}

#ifndef _MODULE_
mod_header_ptr interpreter_get_header(){
	return sos_get_header_address(mod_header);
}
#endif
