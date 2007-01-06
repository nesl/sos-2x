/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

/**
 * @brief Packet time stamping service 
 * @author Simon Han
 */
#include <sos_types.h>
#include <stddef.h>
#include <malloc.h>
#include <message_types.h>
#include <hardware.h>

/**
 * timestamp record
 */
typedef struct {
	void *ptr;
	uint32_t ts;
} ts_rec_t;

/**
 * timestamp service record
 */
typedef struct ts_svc{
	sos_pid_t pid;  //! module id
	uint8_t num_rec;
	uint8_t next_rec;
	ts_rec_t rec[];	
} ts_svc_t;

static ts_svc_t *ts_owner = NULL;

static void timestamp_do_timestamp(void* ptr, uint32_t t)
{
	ts_owner->rec[ts_owner->next_rec].ptr = ptr;
	ts_owner->rec[ts_owner->next_rec].ts = t;
	ts_owner->next_rec += 1;
	if(ts_owner->next_rec == ts_owner->num_rec) {
		ts_owner->next_rec = 0;
	}
}

void timestamp_incoming(Message *msg_in, uint32_t t)
{
	if(ts_owner && (ts_owner->pid == msg_in->did)) {
		timestamp_do_timestamp(msg_in->data, t);
	}
}

void timestamp_outgoing(Message *msg_out, uint32_t t)
{
	if(ts_owner && (ts_owner->pid == msg_out->sid)) {
		timestamp_do_timestamp(msg_out->data, t);
	}
}

int8_t ker_timestamp_register(sos_pid_t pid, uint8_t n)
{
	uint8_t i;
	if(ts_owner != NULL) {
		return -EBUSY;
	}
#ifndef NO_RADIO
	if(radio_set_timestamp(true) != SOS_OK) {
		return -ENXIO;
	}
#endif
	ts_owner = (ts_svc_t*)ker_malloc(offsetof(struct ts_svc, rec) + n * sizeof(ts_rec_t), KER_TS_PID);
	if(ts_owner == NULL){ 
#ifndef NO_RADIO
		radio_set_timestamp(false);
#endif
		return -ENOMEM;
	}
	ts_owner->pid = pid;
	ts_owner->num_rec = n;
	ts_owner->next_rec = 0;
	for(i = 0; i < n; i++) {
		(ts_owner->rec[i]).ptr = NULL;
		(ts_owner->rec[i]).ts = 0;
	}
	return SOS_OK;
}


int8_t ker_timestamp_deregister(sos_pid_t pid)
{
	if(ts_owner && ts_owner->pid == pid) {
		ker_free(ts_owner);
		ts_owner = NULL;
#ifndef NO_RADIO
		radio_set_timestamp(false);
#endif
		return SOS_OK;
	} else {
		return -ESRCH; 
	}
}

uint32_t ker_timestamp_query(sos_pid_t pid, void *data)
{
	uint8_t i;
	if(ts_owner->pid != pid) return 0;
	for(i = 0; i < ts_owner->num_rec; i++) {
		if((ts_owner->rec[i].ptr == data) && (ts_owner->rec[i].ts != 0)) {
			uint32_t ret = ts_owner->rec[i].ts;
			ts_owner->rec[i].ptr = NULL;
			ts_owner->rec[i].ts = 0;
			return ret;
		}
	}
	return 0;
}
