/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

#include "interpreter.h"
#include <sys_module.h>
#include <string.h>
//#include <routing/tree_routing/tree_routing.h>
#define LED_DEBUG
#include <led_dbg.h>


/*
typedef struct {
	uint16_t qid;
	uint32_t interval;
	uint16_t total_samples;
	uint8_t num_queries;
	uint8_t num_qualifiers;
	trigger_t trigger;
	uint8_t queries[];
	qualifier_t qualifiers[];
} new_query_msg_t;
*/

typedef struct{
  uint16_t qid;
	uint16_t total_samples;
	uint32_t interval;
	uint8_t num_queries;
	uint8_t num_qualifiers;
	trigger_t trigger;
	uint8_t query;
	uint8_t query2;
	qualifier_t qual1;
	qualifier_t qual2;
} test_query_t;


typedef struct {
	func_cb_ptr get_hdr_size;
  uint8_t pid;
	uint8_t num_queries;
	query_details_t *queries[8]; // each time a query comes in, we link it into one of these pointers
							                 // when a query finishes, the pointer is returned to being null
	uint8_t sensor_timers[8];    // save the timer id for each sensor value we respond to
	                             // a timer id of 0 signifies that the sensor is not involved in a query
															 // the timer id is a combination between two indexes
															 // the upper 4 bits is the index to the queries of this data structur
															 // the lower 4 bits is the index to both queries, results and recieved of 
															 // query_details_t data structure
} mote_state_t;


static int8_t interpreter_msg_handler(void *state, Message *msg);
static int8_t free_query(query_details_t* query, uint8_t q_index);
static int8_t is_value_qualified(qualifier_t *qual, uint16_t value);
static uint8_t perform_rel_op(bool prev, bool curr, uint8_t rel_op);
static uint8_t execute_trigger(trigger_t trig);
static query_details_t* recieve_new_query(uint8_t *new_query, uint8_t msg_len);

static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id = MOTE_INTERPRETER_PID,
    .state_size = sizeof(mote_state_t),
    .num_timers = 0,
    .num_sub_func = 1,
    .num_prov_func = 0,
    .platform_type = HW_TYPE,
    .processor_type = MCU_TYPE,
    .code_id = ehtons(MOTE_INTERPRETER_PID),
    .module_handler = interpreter_msg_handler,
		.funct = {
			[0] = {error_8, "Cvv0", ROUTING_PID, MOD_GET_HDR_SIZE_FID},
		},
	//	.funct = {
	//		[0] = {error_8, "Cvv0", TREE_ROUTING_PID, MOD_GET_HDR_SIZE_FID},
	//	},
};

static int8_t interpreter_msg_handler(void *state, Message *msg){
    mote_state_t *s = (mote_state_t *) state;

    switch(msg->type){
			case MSG_INIT:
				{
					uint8_t i;
					s->pid = msg->did;

					// initialize all the queries to be unused
					// and enable all the sensors
					s->num_queries = 8;
					for (i = 0; i < 8; i++){
						s->queries[i] = NULL;
						s->sensor_timers[i] = 0xff;
					}

					sys_led(LED_RED_OFF);
					sys_led(LED_YELLOW_OFF);
					sys_led(LED_GREEN_OFF);
				}
				break;

			case MSG_REMOVE:
				{
					// this message will remove all current queries from the node
					//uint8_t msg_len = msg->len;
					//uint8_t *payload = sys_msg_take_data(msg);
					int i;
					for (i = 0; i < s->num_queries;i++){
						free_query(s->queries[i], i);
						s->queries[i] = NULL;
					}
					//sys_post(ROUTING_PID, MSG_SEND_TO_CHILDREN, msg_len, payload, SOS_MSG_RELEASE);
				}
				break;

			case MSG_NEW_QUERY:
				{
					//if (msg->data == NULL)
						//sys_led(LED_YELLOW_TOGGLE);
					
					uint8_t msg_len = msg->len; 
					uint8_t *new_query = sys_msg_take_data(msg);

					//if (new_query == NULL)
						//sys_led(LED_RED_TOGGLE);
					sys_led(LED_GREEN_TOGGLE);

					DEBUG("<INTERPRETER> new query\n");
					uint8_t i=0, j=0;
					while (i < s->num_queries && s->queries[i] != NULL)
						i++;

					if (i < s->num_queries){
						query_details_t *query; 

						// this will set up the query based on the recieved message
						query = recieve_new_query(new_query, msg_len);

						DEBUG("<INTERPRETER>\nqid = %d\nnum_queries = %d\ninterval = %d\n", query->qid,
								query->num_queries, query->interval);

						for (j = 0; j < query->num_queries; j++){
							//sys_led(LED_YELLOW_TOGGLE);
							DEBUG("<INTERPRETER> adding query for sensor: %d\n", query->queries[j]);
							if (query->queries[j] < NUM_SENSORS){
                // test to make sure that the sensor isn't already involved in a query
								if (s->sensor_timers[query->queries[j]] != 0xff){
									free_query(query, i);
									sys_led(LED_RED_TOGGLE);
									query = NULL;
									break;
								}

								uint8_t tid;
								tid = (i << 4) | j;
								s->sensor_timers[query->queries[j]] = tid;
								sys_timer_start(tid, query->interval, TIMER_REPEAT);
							}
							query->results[j] = 0;
						}
						s->queries[i] = query;
					} else {
						//sys_led(LED_GREEN_TOGGLE);
						DEBUG("<INTERPRETER> No room for a new query\n");
					}

					sys_post(ROUTING_PID, MSG_SEND_TO_CHILDREN, msg_len, new_query, SOS_MSG_RELEASE);
				}
				break;

			case MSG_TIMER_TIMEOUT:
				{
					/* for this message type, the byte value in MsgParam holds the timer id
					 * we can now also use this as the sensor id, and as an index for our query type
					 */
					MsgParam *param = (MsgParam *) msg->data;

					DEBUG("<INTERPRETER> Timer Timeout, for timer%d\n", param->byte);

					//sys_led(LED_YELLOW_TOGGLE);
					if (param->byte > NUM_SENSORS && param->byte != 255)
						break;

					uint8_t q_index;
					uint8_t s_index;
					uint8_t sid;

					q_index = param->byte >> 4;
					s_index = param->byte & 0x0F;
					if (q_index < NUM_SENSORS && s_index < s->queries[q_index]->num_queries){
						sid = s->queries[q_index]->queries[s_index];

						DEBUG("<INTERPRETER> getting data for sensor: %d\n", sid);
						if (sid > NUM_SENSORS){
							// do some stuff for special ops;
							// this shouldn't really happen anyways
						}	else{ 
#ifndef SOS_SIM
							sys_sensor_get_data(sid);
#else
							sys_post_value(s->pid, MSG_DATA_READY, 0x00ffff00 | sid, 0);
#endif
						}
					} else if (param->byte == 255){ // our test case
						DEBUG("<INTERPRETER> test case\n");
					} else {
						//sys_led(LED_RED_TOGGLE);
					}
					
					sys_free(msg->data);
				}
				break;

			case MSG_DATA_READY:
				{
					sensor_msg_t *data = (sensor_msg_t *)sys_msg_take_data(msg);

					if (data == NULL)
						DEBUG("data ready fail\n");
					uint8_t q_index;
					uint8_t s_index;

					DEBUG("Data Ready\nsensor = %d\nvalue = %d\n", data->sensor, data->value);

					if (data->sensor < NUM_SENSORS){
						q_index = s->sensor_timers[data->sensor] >> 4;
						s_index = s->sensor_timers[data->sensor] & 0x0F;

						DEBUG("<INTERPRETER> q_index=%d  s_index = %d\n", q_index, s_index);
						DEBUG("<INTERPRETER> recieved=%d num_queries=%d\n", s->queries[q_index]->recieved, s->queries[q_index]->num_queries);

						s->queries[q_index]->results[s_index] = data->value;
						s->queries[q_index]->recieved++;

						if (s->queries[q_index]->recieved == s->queries[q_index]->num_queries){
							sys_post_value(s->pid, MSG_VALIDATE, q_index, SOS_MSG_RELEASE); // this message will dispatch the results since we've gotten all the results
					    s->queries[q_index]->total_samples--;
					  }
					}
					sys_free(data);
				}
				break;

			case MSG_VALIDATE:
				{
				  uint32_t q_index = *((uint32_t *) msg->data);
          query_details_t *q = s->queries[q_index];

					DEBUG("msg validate: q_index = %d\n", q_index);
					if (q->num_qualifiers == 0){
						execute_trigger(q->trigger);
						sys_post_value(s->pid, MSG_DISPATCH, q_index, SOS_MSG_RELEASE);
					}	else {
						uint8_t i;
						bool is_curr_valid = false;
						bool is_prev_valid = true;
						uint8_t rel_op;
						for (i=0; i<q->num_qualifiers; i++){
							uint8_t sid_index = s->sensor_timers[q->qualifiers[i].sid] & 0x0F;
							rel_op = q->qualifiers[i].comp_op_and_relation & 0x0F;
							is_curr_valid = is_value_qualified(&(q->qualifiers[i]), q->results[sid_index]);
              is_prev_valid = perform_rel_op(is_prev_valid, is_curr_valid, rel_op);
						}

						DEBUG("<INTERPRETER> is_prev_valid=%d\n", is_prev_valid);
            if (is_prev_valid){
							execute_trigger(q->trigger);
							sys_post_value(s->pid, MSG_DISPATCH, q_index, 0);
						} else {
								q->recieved = 0;
								if (q->total_samples == 0){
									free_query(q, (uint8_t) q_index);
									s->queries[q_index] = NULL;
								}
						}
					}
				} 
				break;

			case MSG_DISPATCH:
				{
					uint32_t q_index = *((uint32_t *) msg->data);
					query_details_t *q = s->queries[q_index];
					query_result_t *reply;
					uint8_t *pkt;
					int8_t hdr_size;
					uint8_t msg_len;
					uint8_t i;

					DEBUG("<INTERPRETER> msg dispatch, with queries remaining=%d\n", q->total_samples);

					//hdr_size = sizeof(tr_hdr_t);
					hdr_size = SOS_CALL(s->get_hdr_size, get_hdr_size_proto);
					DEBUG("<INTERPRETER> hdr size = %d\n", hdr_size);
					if (hdr_size < 0) {return SOS_OK;}

					msg_len =  hdr_size + sizeof(query_result_t) + sizeof(sensor_msg_t)*q->num_queries;
					pkt = (uint8_t *) sys_malloc(msg_len);

					if (!pkt){
						DEBUG("<INTERPRETER> malloc not ok\n");
						return -EINVAL;
					}

					reply = (query_result_t *) (pkt+hdr_size);
					for (i = 0; i < q->num_queries; i++){
						reply->results[i].sensor = q->queries[i];
						reply->results[i].value = q->results[i];
					}

					reply->qid = q->qid;
					reply->num_remaining = q->total_samples;
					reply->num_results = q->num_queries;
					reply->node_id = sys_id();

					sys_post(ROUTING_PID, MSG_SEND_PACKET, msg_len, 
								(void *) pkt, SOS_MSG_RELEASE);

					q->recieved = 0;
					if (q->total_samples == 0){
						free_query(q, (uint8_t) q_index);
						s->queries[q_index] = NULL;
					}
				}
				break;					

			case MSG_TR_DATA_PKT:
				{
					if (sys_id() == BASE_STATION_ADDRESS){
						uint8_t *payload;
						uint8_t msg_len;

					  //sys_led(LED_GREEN_TOGGLE);
						msg_len = msg->len;
						payload = sys_msg_take_data(msg);

						DEBUG("<MOTE INTERPRETER> tree routing packet recieved, size = %d\n",msg_len);

#ifndef SOS_SIM
						//sys_post_uart(s->pid, MSG_QUERY_REPLY, sizeof(query_result_t), reply, 0, BCAST_ADDRESS);
						sys_post_uart(s->pid, MSG_QUERY_REPLY, msg_len, payload, SOS_MSG_RELEASE, BCAST_ADDRESS);
#else
						sys_free(payload);
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

static query_details_t* recieve_new_query(uint8_t *new_query, uint8_t msg_len){
	//sys_led(LED_GREEN_TOGGLE);
	query_details_t *q;

	uint8_t i;
	for (i = 0; i < msg_len; i++){
		DEBUG("<INTERPRETER> new_query at i = %d\nvalue = %d\n", i, *(new_query + i));
	}
	q = (query_details_t *)sys_malloc(sizeof(query_details_t));

	memcpy(q, new_query, sizeof(uint8_t) * 12);

	new_query += STATIC_QUERY_SIZE;
  q->queries = (uint8_t *) sys_malloc(sizeof(uint8_t) * q->num_queries);
	q->qualifiers = (qualifier_t *) sys_malloc(sizeof(qualifier_t) * q->num_qualifiers);
	q->results = (uint16_t *) sys_malloc(sizeof(uint16_t) * q->num_queries);

	DEBUG("<INTERPRETER> forming new query\nnum_queries = %d\nnum_qualifiers = %d\n", q->num_queries, q->num_qualifiers);
	memcpy(q->queries, new_query, sizeof(uint8_t) * q->num_queries);

	if (q->num_queries > 0){
		DEBUG("<INTERPRETER> sensor value 1: %d\n", q->queries[0]);
	  DEBUG("<INTERPRETER> value from new_query: %d\n", *new_query);
	}
	
	new_query += q->num_queries;
	memcpy(q->qualifiers, new_query, sizeof(qualifier_t) * q->num_qualifiers);

	DEBUG("<INTERPRETER> qualifiers: comp_value=%d\nsid=%d\n", q->qualifiers[0].comp_value, q->qualifiers[0].sid);

	q->recieved = 0;

	return q;
}

static int8_t free_query(query_details_t* query, uint8_t q_index){
  uint8_t i;
	//sys_led(LED_RED_TOGGLE);
	if (query == NULL)
		return SOS_OK;
	mote_state_t *s = (mote_state_t *) sys_get_state();

  DEBUG("<INTERPRETER> freeing query\n");

	for (i = 0; i < query->num_queries; i++){
		uint8_t tid = 0;
		tid = q_index << 4;
		tid |= (i & 0x0F);
		sys_timer_stop(tid);

		if (query->queries[i] < NUM_SENSORS)
			s->sensor_timers[query->queries[i]] = 0xff;
	}

	sys_free(query->queries);
	sys_free(query->qualifiers);
	sys_free(query->results);
	sys_free(query);

	query = NULL;
	return SOS_OK;
}

static uint8_t execute_trigger(trigger_t trig){
  switch (trig.command){
		case 1:
			sys_led(trig.value);
			break;
		default:
			break;
	}
	return SOS_OK;
}

static int8_t is_value_qualified(qualifier_t *qual, uint16_t value){
	uint8_t comp_op = (qual->comp_op_and_relation & 0xF0) >> 4;

	DEBUG("<INTERPRETER> qualifier: valu=%d comp_value=%d comparitor=%d\n", value, qual->comp_value, comp_op);
	switch (comp_op){
		case LESS_THAN:
		 return value < qual->comp_value;
	   break;
	  case GREATER_THAN:
		 DEBUG("<INTERPRETER> greater than=%d\n", value>qual->comp_value);
		 return value > qual->comp_value;
		 break;
		case EQUAL:
		 return value == qual->comp_value;
		 break;
		case NOT_EQUAL:
		 return value != qual->comp_value;
		 break;
		case GREATER_THAN_EQUAL:
		 return value >= qual->comp_value;
		 break;
		case LESS_THAN_EQUAL:
		 return value <= qual->comp_value;
		 break;
	  default:
		 return true;
	}
}

static uint8_t perform_rel_op(bool prev, bool curr, uint8_t rel_op){
	uint8_t ret;
	switch (rel_op){
		case AND:
		  ret = prev && curr;
			break;
	  case OR:
			ret = prev || curr;
			break;
	  case AND_NOT:
			ret = prev && ~curr;
			break;
		case OR_NOT:
			ret = prev || ~curr;
      break;
		case NOT:
			ret = ~curr;
		  break;
	  default:
			ret = true;
	}
	DEBUG("<INTERPRETER> rel_op = %d, rel_val = %d\n",rel_op,  ret);
	return ret;
}
	   	 
