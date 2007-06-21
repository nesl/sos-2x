/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: message_queue.c,v 1.12 2006/12/26 07:38:03 simonhan Exp $
 */
/**
 * @brief     SOS message queue implementation
 * @author    Simon Han (simonhan@cs.ucla.edu)
 */
#include <message_queue.h>
#include <message.h>
#include <malloc.h>
#include <hardware_types.h>
#include <monitor.h>
#include <string.h>
#include <sos_sched.h>
#include <slab.h>
#include <hardware.h>

#if defined (SOS_UART_CHANNEL)
#include <sos_uart.h>
#include <sos_uart_mgr.h>
#endif

#if defined (SOS_I2C_CHANNEL)                                
#include <sos_i2c.h>                                         
#include <sos_i2c_mgr.h>                                     
#endif    

// Comment out the lines below to enable debug messages from this component
#undef DEBUG
#define DEBUG(...)

#define MSG_QUEUE_NUM_ITEMS 4
//----------------------------------------------------------------------------
//  Global data declarations
//----------------------------------------------------------------------------
static slab_t msg_slab;

//----------------------------------------------------------------------------
//  Funcation declarations
//----------------------------------------------------------------------------
int8_t msg_queue_init()
{
	ker_slab_init( MSG_QUEUE_PID, &msg_slab, sizeof(Message), 
			MSG_QUEUE_NUM_ITEMS, 0 );
    return 0;
}

/**
 * @brief initialize the message queue
 */
void mq_init(mq_t *q)
{
#ifdef SOS_USE_PREEMPTION
  q->head = NULL;
  q->msg_cnt = 0;
#else 
  q->msg_cnt = 0;
  q->hm_cnt = 0;
  q->sm_cnt = 0;
  q->lm_cnt = 0;
  q->hq_head = NULL;
  q->hq_tail = NULL;
  q->sq_head = NULL;
  q->sq_tail = NULL;
  q->lq_head = NULL;
  q->lq_tail = NULL;
#endif
}

/**
 * @brief enqueue message to message queue
 * We enqueue the message based on the flag in the message
 */
void mq_enqueue(mq_t *q, Message *m)
{
  HAS_CRITICAL_SECTION;

#ifdef SOS_USE_PREEMPTION
  Message *cur;
  Message *prev;
  ENTER_CRITICAL_SECTION();

  // If head is empty, insert here
  if(q->head == NULL) {
	q->head = m;
	m->next = NULL;
	q->msg_cnt++;
	LEAVE_CRITICAL_SECTION();
	return;
  }

  // Insertion at the head of the list
  if(q->head->priority < m->priority) {
	m->next = q->head;
	q->head = m;
	q->msg_cnt++;
	LEAVE_CRITICAL_SECTION();
	return;
  }

  // Traverse through the list looking for the 
  // right insertion point based on priority
  cur = q->head->next;
  prev = q->head;
  while(cur != NULL) {
	if(cur->priority < m->priority) {
	  m->next = cur;
	  prev->next = m;
	  q->msg_cnt++;
	  LEAVE_CRITICAL_SECTION();
	  return;
	}
	prev = cur;
	cur = cur->next;
  }
  // End of list, insert at last point
  m->next = NULL;
  prev->next = m;
  q->msg_cnt++;

#else
  ENTER_CRITICAL_SECTION();
  
  m->next = NULL;
  
  if(flag_high_priority(m->flag)){
	  //! high priority message
	  if(q->hq_head == NULL) {
		  //! empty head
		  q->hq_head = m;	
		  q->hq_tail = m;
	  } else {
		  //! insert to tail
		  q->hq_tail->next = m;
		  q->hq_tail = m;
	  }
	  q->hm_cnt++;
  } else if(flag_system(m->flag)){
	  //! system msgs
	  if(q->sq_head == NULL) {
		  //! empty head
		  q->sq_head = m;	
		  q->sq_tail = m;
	  } else {
		  //! insert to tail
		  q->sq_tail->next = m;
		  q->sq_tail = m;
	  }
	  q->sm_cnt++;
  } else {
	  //! low priority message
	  if(q->lq_head == NULL) {
		  //! empty head
		  q->lq_head = m;	
		  q->lq_tail = m;
	  } else {
		  //! insert to tail
		  q->lq_tail->next = m;
		  q->lq_tail = m;
	  }
	  q->lm_cnt++;
  }
  q->msg_cnt++;
#endif
  LEAVE_CRITICAL_SECTION();
}

/**
 * @brief dequeue message
 * @return pointer to message, or NULL for empty queue
 * First we check high priority queue.
 * if it is empty, we check for low priority queue
 */
Message *mq_dequeue(mq_t *q)
{
	HAS_CRITICAL_SECTION;
	Message *tmp = NULL;

	ENTER_CRITICAL_SECTION();

#ifdef SOS_USE_PREEMPTION
	if((tmp = q->head) != NULL) {
	  q->head = tmp->next;
	  q->msg_cnt--;	  
	}
	LEAVE_CRITICAL_SECTION();
#else
	if ((tmp = q->hq_head) != NULL) { 
	//! high priority message
		q->hq_head = tmp->next;
		q->hm_cnt--;
	//! system msgs
	} else if ((tmp = q->sq_head) != NULL) {
		q->sq_head = tmp->next;
		q->sm_cnt--;
	} else if ((tmp = q->lq_head) != NULL) { 
	//! low priority message
		q->lq_head = tmp->next;
		q->lm_cnt--;
	} else {
		LEAVE_CRITICAL_SECTION();
		return NULL;
	}
	q->msg_cnt--;
#endif
	LEAVE_CRITICAL_SECTION();
	return tmp;
}

#ifdef SOS_USE_PREEMPTION
static Message *mq_real_get(Message **head, Message *m)
#else
static Message *mq_real_get(Message **head, Message **tail, Message *m)
#endif
{
  Message *prev;
  Message *curr;

  prev = *head;
  curr = *head;

  // Traverse through the queue
  while(curr != NULL) {
	// Try to match the header
	if(m->did == curr->did &&
	   m->sid == curr->sid &&
	   m->daddr == curr->daddr &&
	   m->saddr == curr->saddr &&
	   m->type == curr->type  &&
	   m->len == curr->len ) {
	  uint8_t i = 0;
	  Message *ret = curr;
	  bool msg_matched = true;
	  // Try to match the data
	  for(i = 0; i < m->len; i++) {
		if(m->data[i] != curr->data[i]) {
		  msg_matched = false;
		  break;
		}	
	  }
	  // A match is found
	  if(msg_matched == true) {
		// The match is at the head
		if(ret == (*head)) {
		  *head = curr->next;
#ifndef SOS_USE_PREEMPTION
		  if( (*head) == NULL ) {
			*tail = NULL;
		  } 
		} else if(ret == (*tail)) {
		  prev->next = NULL;
		  *tail = prev;
#endif
		} else {
		  // The match is not at the head
		  prev->next = curr->next;
		}
		return ret;
	  }
	}
	// increment the pointers
	prev = curr;
	curr = curr->next;
  }
  return NULL;
}

/**
 * @brief get message that matches the header in the queue
 *
 * NOTE it matches only daddr, saddr, did, sid, type
 * NOTE it only gets the first that matches the description
 */
Message *mq_get(mq_t *q, Message *m)
{
  HAS_CRITICAL_SECTION;
  Message *ret;

#ifdef SOS_USE_PREEMPTION
  if(q->head == NULL) return NULL;
  ENTER_CRITICAL_SECTION();

  // Search the queue
  ret = mq_real_get(&(q->head), m);
#else
  if(q->msg_cnt == 0) return NULL;
  ENTER_CRITICAL_SECTION();
	
  //! first search high priority queue
  ret = mq_real_get(&(q->hq_head), &(q->hq_tail), m);

  if(ret) {
	q->msg_cnt--;
	LEAVE_CRITICAL_SECTION();
	return ret;
  }
  //! search low priority queue
  ret = mq_real_get(&(q->lq_head), &(q->lq_tail), m);
  if(ret) {
	q->msg_cnt--;
  }
#endif
  LEAVE_CRITICAL_SECTION();
  return ret;
}

//
// mark memory in the message
//
void mq_gc_mark_payload( mq_t *q, sos_pid_t pid )
{
	Message *m;

#ifdef SOS_USE_PREEMPTION
	for( m = q->head; m != NULL; m = m->next ) {
	  if( flag_msg_release( m->flag ) ) {
		ker_gc_mark( pid, m->data );
	  }
	}
#else
	for( m = q->hq_head; m != NULL; m = m->next ) {
		if( flag_msg_release( m->flag ) ) {
			ker_gc_mark( pid, m->data );
		}
	}
	
	for( m = q->sq_head; m != NULL; m = m->next ) {
		if( flag_msg_release( m->flag ) ) {
			ker_gc_mark( pid, m->data );
		}
	}
	
	for( m = q->lq_head; m != NULL; m = m->next ) {
		if( flag_msg_release( m->flag ) ) {
			ker_gc_mark( pid, m->data );
		}
	}
#endif
}

//
// mark the message header to slab
//
void mq_gc_mark_hdr( mq_t *q, sos_pid_t pid )
{
	Message *m;

#ifdef SOS_USE_PREEMPTION
	for( m = q->head; m != NULL; m = m->next ) {
		slab_gc_mark( &msg_slab, m );
	}
#else
	for( m = q->hq_head; m != NULL; m = m->next ) {
		slab_gc_mark( &msg_slab, m );
	}
	
	for( m = q->sq_head; m != NULL; m = m->next ) {
		slab_gc_mark( &msg_slab, m );
	}
	
	for( m = q->lq_head; m != NULL; m = m->next ) {
		slab_gc_mark( &msg_slab, m );
	}
#endif
}

void mq_gc_mark_one_hdr( Message *msg )
{
	slab_gc_mark( &msg_slab, msg );
}

//
// GC on all message queues
//
void mq_gc( void )
{
	// TODO: call all message queues
#ifdef SOS_USE_GC
	sched_msg_gc();
#ifdef SOS_RADIO_CHANNEL
	radio_msg_gc();
#endif

#ifdef SOS_UART_CHANNEL
	uart_msg_gc();
#endif
	slab_gc( &msg_slab, MSG_QUEUE_PID );
	malloc_gc( MSG_QUEUE_PID );
#endif
}

/**
 * @brief create message
 * @return pointer to message, or NULL for fail
 * get new message header from message repositary
 * msg->data is pointing to payload
 */
Message *msg_create()
{
	HAS_CRITICAL_SECTION;
	Message *tmp = NULL;
	//
	// Get from msg_pool
	//
	ENTER_CRITICAL_SECTION();
	tmp = ker_slab_alloc( &msg_slab, MSG_QUEUE_PID );
	if( tmp == NULL ) {
		LEAVE_CRITICAL_SECTION();
		return NULL;
	}
	
	LEAVE_CRITICAL_SECTION();
  	tmp->data = tmp->payload;
	tmp->flag = 0;
  
	return tmp;
}

/**
 * @brief dispose message
 * return message header back to message repostitary
 */
void msg_dispose(Message *m)
{
	HAS_CRITICAL_SECTION;
	
	if(flag_msg_release(m->flag)) { 
		ker_free(m->data); 
	}

	ENTER_CRITICAL_SECTION();
	ker_slab_free( &msg_slab, m );
	
	LEAVE_CRITICAL_SECTION();
}

/**
 * @brief handle the process of creating senddone message
 * @param msg_sent  the Message just sent or delivered
 * @param succ      is the delivery successful?
 * @param msg_owner the owner of the message 
 * NOTE the implementation will need to improve
 */
void msg_send_senddone(Message *msg_sent, bool succ, sos_pid_t msg_owner)
{
  uint8_t flag;
  
  if(flag_msg_reliable(msg_sent->flag) == 0) {
	msg_dispose(msg_sent);
	return;
  }
  
  /*
   * Release the memory 
   */
  if(flag_msg_release(msg_sent->flag)){
	ker_free(msg_sent->data);
	msg_sent->flag &= ~(SOS_MSG_RELEASE);
	msg_sent->data = NULL;
  }

  if(succ == false) {
	flag = SOS_MSG_SEND_FAIL; 
  } else {
	flag = 0;
  }
  if(post_long(msg_sent->sid, msg_owner, MSG_PKT_SENDDONE, 
			   sizeof(Message), msg_sent, flag) < 0) {
	msg_dispose(msg_sent);
  } 
}

/*
  void mq_print(mq_t *q)
  {
  Message *itr;
  DEBUG("Message Queue\n");
  for(itr = q->hq_head; itr != NULL; itr = itr->next)
  {
  msg_header_out("MQ H", itr);
  }	

  for(itr = q->lq_head; itr != NULL; itr = itr->next)
  {
  msg_header_out("MQ L", itr);
  }	
  }
*/


