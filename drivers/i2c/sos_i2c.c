/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief    sos_i2c messaging layer
 * @author	 Naim Busek <ndbusek@gmail.com>
 *
 */
#include <hardware.h>
#include <message_queue.h>
#include <net_stack.h>
#include <sos_info.h>
#include <crc.h>
#include <measurement.h>
#include <malloc.h>
#include <sos_timer.h>
#include <i2c_system.h>

#include "sos_i2c.h"

#ifndef NO_SOS_I2C

//#define LED_DEBUG
#include <led_dbg.h>

#define SOS_I2C_BACKOFF_TIME 256L
#define SOS_I2C_TID        0
#define MAX_RETRY_COUNT        3

enum {
  SOS_I2C_IDLE=0,
	SOS_I2C_BACKOFF,
  SOS_I2C_TX_MSG,
};

static int8_t sos_i2c_msg_handler(void *state, Message *e);

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = KER_I2C_PID,
	.state_size     = 0,
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = sos_i2c_msg_handler,
};


typedef struct sos_i2c_state {
  uint8_t state;
  uint8_t i2cRetryCount;
	Message *msg_ptr;
} sos_i2c_state_t;

//! priority queue
static mq_t i2c_pq;
static sos_i2c_state_t s;

void sos_i2c_init()
{
  s.state = SOS_I2C_IDLE;
	s.msg_ptr = NULL;

	ker_register_module(sos_get_header_address(mod_header));

	mq_init(&i2c_pq);
}


static inline void i2c_try_send(Message *m) {
	
	if (ker_i2c_reserve_bus(KER_I2C_PID, I2C_ADDRESS, (I2C_SYS_SOS_MSG_FLAG|I2C_SYS_TX_FLAG|I2C_SYS_MASTER_FLAG)) != SOS_OK) {

		ker_i2c_release_bus(KER_I2C_PID);
		
		if( (m->daddr != BCAST_ADDRESS) && (s.i2cRetryCount<MAX_RETRY_COUNT) )  		// Kevin - Best effor delivery for BCAST address
		{			
			goto queue_and_backoff;					
		}
		else
		{		
			s.state = SOS_I2C_IDLE;			
			msg_send_senddone(m, false, KER_I2C_PID);		
			s.i2cRetryCount = 0;
			ker_timer_start(KER_I2C_PID, SOS_I2C_TID, SOS_I2C_BACKOFF_TIME);
		}
	}

	if (ker_i2c_send_data(I2C_BCAST_ADDRESS, (uint8_t*)m, m->len, KER_I2C_PID) != SOS_OK) {
		LED_DBG(LED_RED_TOGGLE);		
		ker_i2c_release_bus(KER_I2C_PID);		
		if( (m->daddr != BCAST_ADDRESS) && (s.i2cRetryCount<MAX_RETRY_COUNT) )  		// Kevin - Best effor delivery for BCAST address
		{			
			goto queue_and_backoff;					
		}
		else
		{		
			s.state = SOS_I2C_IDLE;			
			msg_send_senddone(m, false, KER_I2C_PID);													
			s.i2cRetryCount = 0;
			ker_timer_start(KER_I2C_PID, SOS_I2C_TID, SOS_I2C_BACKOFF_TIME);
		}		
	}
	s.state = SOS_I2C_TX_MSG;
	return;

queue_and_backoff:
	
	s.i2cRetryCount++;
	s.state = SOS_I2C_BACKOFF;
	mq_enqueue(&i2c_pq, m);
	ker_timer_start(KER_I2C_PID, SOS_I2C_TID, SOS_I2C_BACKOFF_TIME);
}


void i2c_msg_alloc(Message *m)
{
	HAS_CRITICAL_SECTION;
	// change ownership
	if(flag_msg_release(m->flag)){
		ker_change_own(m->data, KER_I2C_PID);
	}

  ENTER_CRITICAL_SECTION();
	if(s.state == SOS_I2C_IDLE) {
		s.msg_ptr = m;
		i2c_try_send(s.msg_ptr);
	} else {
		mq_enqueue(&i2c_pq, m);
	}
	LEAVE_CRITICAL_SECTION();
}


int8_t sos_i2c_msg_handler(void *state, Message *msg) {

	switch (msg->type) {
		case MSG_INIT:
			s.state = SOS_I2C_IDLE;												
			s.i2cRetryCount = 0;
			ker_timer_init(KER_I2C_PID, SOS_I2C_TID, TIMER_ONE_SHOT);
			break;

		case MSG_FINAL:
			ker_timer_stop(KER_I2C_PID, 0);
			break;

		case MSG_I2C_SEND_DONE:
			{
				LED_DBG(LED_GREEN_TOGGLE);
				Message *msg_txed;   //! message just transmitted
				msg_txed = s.msg_ptr;
				s.msg_ptr = NULL;
				// post send done message to calling module
				if (flag_send_fail(msg->flag)) {
					msg_send_senddone(msg_txed, false, KER_I2C_PID);
				} else {
					msg_send_senddone(msg_txed, true, KER_I2C_PID);
				}
			} // fall through

		case MSG_TIMER_TIMEOUT:
			// if message in queue start transmission
			s.msg_ptr = mq_dequeue(&i2c_pq);
			if (s.msg_ptr) {
				i2c_try_send(s.msg_ptr);
				break;
			} else { // else free bus
				ker_i2c_release_bus(KER_I2C_PID);
				s.state = SOS_I2C_IDLE;
			}
			break;

		case MSG_ERROR:
			// post error message to calling module
			ker_i2c_release_bus(KER_I2C_PID);
			ker_timer_stop(KER_I2C_PID, 0);

			Message *msg_txed = s.msg_ptr;
			s.msg_ptr = NULL;
			msg_send_senddone(msg_txed, false, KER_I2C_PID);
			s.state = SOS_I2C_IDLE;

		default:
			return -EINVAL;
	}

	return SOS_OK;
}

#endif // NO_SOS_I2C
