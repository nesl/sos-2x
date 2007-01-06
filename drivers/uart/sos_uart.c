/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief    sos_uart messaging layer
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
#include <uart_system.h>

#include <sos_uart_mgr.h>
#include <sos_uart.h>

//#define LED_DEBUG
#include <led_dbg.h>

#define SOS_UART_BACKOFF_TIME 256L
#define SOS_UART_TID        0

#ifndef NO_SOS_UART
enum {
  SOS_UART_IDLE=0,
	SOS_UART_BACKOFF,
  SOS_UART_TX_MSG,
};

typedef struct sos_uart_state {
  uint8_t state;
	Message *msg_ptr;
} sos_uart_state_t;

//! priority queue
static mq_t uartpq;
static sos_uart_state_t s;

static int8_t sos_uart_msg_handler(void *state, Message *e);
static void sos_uart_msg_senddone( bool failed );

static mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = KER_UART_PID,
	.state_size     = 0,  // need to keep state localy
	.num_timers     = 1,
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.module_handler = sos_uart_msg_handler,
};


void sos_uart_init()
{
  s.state = SOS_UART_IDLE;
	s.msg_ptr = NULL;

	// set uart_address 
	set_uart_address(ker_id());
	ker_register_module(sos_get_header_address(mod_header));

	mq_init(&uartpq);
}


static void uart_try_reserve_and_send(Message *m) {
	if (ker_uart_reserve_bus(KER_UART_PID, UART_SYS_TX_FLAG|UART_SYS_SOS_MSG_FLAG) != SOS_OK) {
		LED_DBG(LED_RED_ON);
		ker_uart_release_bus(KER_UART_PID);
		goto queue_and_backoff;
	}

	if (ker_uart_send_data((uint8_t*)m, m->len, KER_UART_PID) != SOS_OK) {
		goto queue_and_backoff;
	}
	s.state = SOS_UART_TX_MSG;
	return;

queue_and_backoff:
	//DEBUG("UART backoff\n");
	s.state = SOS_UART_BACKOFF;
	mq_enqueue(&uartpq, m);
	ker_timer_restart(KER_UART_PID, SOS_UART_TID, SOS_UART_BACKOFF_TIME);
}

// Use this when the bus is already reserved
static void uart_try_send_reserved_bus(Message *m)
{
	if (ker_uart_send_data((uint8_t*)m, m->len, KER_UART_PID) != SOS_OK) {
		//DEBUG("UART backoff\n");
		s.state = SOS_UART_BACKOFF;
		mq_enqueue(&uartpq, m);
		ker_timer_restart(KER_UART_PID, SOS_UART_TID, SOS_UART_BACKOFF_TIME);
		return;
	} 
	//DEBUG("end of try send reserved bus\n");
	s.state = SOS_UART_TX_MSG;
}


void uart_msg_alloc(Message *m)
{
	HAS_CRITICAL_SECTION;
	//! change ownership
	if(flag_msg_release(m->flag)){
		ker_change_own(m->data, KER_UART_PID);
	}

  ENTER_CRITICAL_SECTION();
	//DEBUG("uart_msg_alloc %d\n", s.state);
	if(s.state == SOS_UART_IDLE) {
		s.msg_ptr = m;
		uart_try_reserve_and_send(s.msg_ptr);
	} else {
		mq_enqueue(&uartpq, m);
	}
	//DEBUG("end uart_msg_alloc %d\n", s.state);
	LEAVE_CRITICAL_SECTION();
}


int8_t sos_uart_msg_handler(void *state, Message *msg) 
{
	HAS_CRITICAL_SECTION;
  ENTER_CRITICAL_SECTION();
	switch (msg->type) {
		case MSG_INIT:
				s.state = SOS_UART_IDLE;
				ker_timer_init(KER_UART_PID, SOS_UART_TID, TIMER_ONE_SHOT);
				break;

		case MSG_FINAL:
				ker_timer_stop(KER_UART_PID, 0);
				break;

		case MSG_UART_SEND_DONE:
		{
				//DEBUG("end uart_send_done %d\n", s.state);
				//s.state = SOS_UART_IDLE;
				sos_uart_msg_senddone(flag_send_fail(msg->flag));

				break;
		}
		case MSG_TIMER_TIMEOUT:
		{
				// if message in queue start transmission
				//DEBUG("uart_timeout %d\n", s.state);
				s.msg_ptr = mq_dequeue(&uartpq);
				if (s.msg_ptr) {
					uart_try_reserve_and_send(s.msg_ptr);
				} else { // else free bus
					ker_uart_release_bus(KER_UART_PID);
					s.state = SOS_UART_IDLE;
				}
				//DEBUG("end uart_timeout %d\n", s.state);
				break;
		}
		case MSG_ERROR:
		{
					Message *msg_txed;
					//DEBUG("uart_error %d\n", s.state);
					// post error message to calling module
					ker_uart_release_bus(KER_UART_PID);

					msg_txed = s.msg_ptr;
					s.msg_ptr = NULL;
					msg_send_senddone(msg_txed, false, KER_UART_PID);

					s.state = SOS_UART_BACKOFF;
					ker_timer_restart(KER_UART_PID, SOS_UART_TID, SOS_UART_BACKOFF_TIME);
					//DEBUG("end uart_error %d\n", s.state);
					break;
		}

		default: 
		{
			LEAVE_CRITICAL_SECTION();
			return -EINVAL;
		}
	}

	LEAVE_CRITICAL_SECTION();
	return SOS_OK;
}

// this is from Interrupt handler
static void sos_uart_msg_senddone( bool failed )
{
	Message *msg_txed;   //! message just transmitted
	msg_txed = s.msg_ptr;
	s.msg_ptr = NULL;
	//DEBUG("uart_send_done %d\n", s.state);
	LED_DBG(LED_GREEN_TOGGLE);
	// post send done message to calling module
	msg_send_senddone(msg_txed, !failed, KER_UART_PID);
	s.msg_ptr = mq_dequeue(&uartpq);
	if (s.msg_ptr) {
		uart_try_send_reserved_bus(s.msg_ptr);
	} else {
		ker_uart_release_bus(KER_UART_PID);
		s.state = SOS_UART_IDLE;
	}
	//DEBUG("end_uart_send_done %d\n", s.state);
}
#endif

