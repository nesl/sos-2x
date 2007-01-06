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

// Comment out the lines below to enable debug messages from this component
#undef DEBUG
#define DEBUG(...)

#define MSG_QUEUE_NUM_ITEMS  4    // NOTE: 
#define MSG_POOL_EMPTY      0x0F  // bit vector that specifies the pool to be 
                                  // empty.  This value corresponds to 
                                  // MSG_QUEUE_NUM_ITEMS.  For 8, 
                                  // the value is 0xFF.  For 7, it will be 0x7F
//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------
typedef struct msg_pool_item {
	struct msg_pool_item *next;
	Message pool[MSG_QUEUE_NUM_ITEMS];
	uint8_t alloc;   //!< allocation vector
} msg_pool_item;

//----------------------------------------------------------------------------
//  Global data declarations
//----------------------------------------------------------------------------
static msg_pool_item msg_pool;

//----------------------------------------------------------------------------
//  Funcation declarations
//----------------------------------------------------------------------------
int8_t msg_queue_init()
{
	msg_pool.next = NULL;
	
	msg_pool.alloc = 0;
    return 0;
}

/**
 * @brief initialize the message queue
 */
void mq_init(mq_t *q)
{
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
}

/**
 * @brief enqueue message to message queue
 * We enqueue the message based on the flag in the message
 */
void mq_enqueue(mq_t *q, Message *m)
{
  HAS_CRITICAL_SECTION;

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
	LEAVE_CRITICAL_SECTION();
	return tmp;
}

static Message *mq_real_get(Message **head, Message **tail, Message *m)
{
  Message *prev;
  Message *curr;

  prev = *head;
  curr = *head;

  while(curr != NULL) {
	if(m->did == curr->did &&
	   m->sid == curr->sid &&
	   m->daddr == curr->daddr &&
	   m->saddr == curr->saddr &&
	   m->type == curr->type  &&
	   m->len == curr->len ) {
	  uint8_t i = 0;
	  Message *ret = curr;
	  bool msg_matched = true;
	  for(i = 0; i < m->len; i++) {
		if(m->data[i] != curr->data[i]) {
		  msg_matched = false;
		  break;
		}	
	  }
	  if(msg_matched == true) {
		if(ret == (*head)) {
		  *head = curr->next;
		  if( (*head) == NULL ) {
			*tail = NULL;
		  } 
		} else if(ret == (*tail)) {
		  prev->next = NULL;
		  *tail = prev;
		} else {
		  prev->next = curr->next;
		}
		return ret;
	  }
	}
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
  LEAVE_CRITICAL_SECTION();
  return ret;
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
	msg_pool_item *prev = NULL;
	msg_pool_item *itr;
	Message *tmp = NULL;
	//
	// Get from msg_pool
	//
	ENTER_CRITICAL_SECTION();
	itr = &msg_pool;
	
	while( itr != NULL ) {
		if( itr->alloc != MSG_POOL_EMPTY) {
			break;
		}
		prev = itr;
		itr = itr->next;
	}

	if( itr == NULL ) {
		DEBUG("MQ: itr == NULL, allocate new\n");
		prev->next = ker_malloc(sizeof(msg_pool_item), MSG_QUEUE_PID);
		if( prev->next == NULL ) {
			LEAVE_CRITICAL_SECTION();
			return NULL;
		}
		itr = prev->next;
		itr->next = NULL;
		itr->alloc = 0x01;
		tmp = &(itr->pool[0]);
	} else {
		//
		// Find empty block
		//
		uint8_t i = 0;
		uint8_t mask = 0x01;

		for( i = 0; i < MSG_QUEUE_NUM_ITEMS; i++, mask<<=1 ) {
			if( (itr->alloc & mask) == 0 ) {
				DEBUG("MQ: allocate %x of item %d\n", (unsigned int)itr, i);
				itr->alloc |= mask;
				tmp = &(itr->pool[i]);
				break;
			}
		}
	}
	LEAVE_CRITICAL_SECTION();
  //tmp = (Message*)ker_malloc(sizeof(Message), MSG_QUEUE_PID);
  //if(tmp != NULL) {
	tmp->data = tmp->payload;
	tmp->flag = 0;
  //}
  return tmp;
}

#ifdef FAULT_TOLERANT_SOS
// Move a given message from the kernel to the module domain
Message *msg_move_to_module_domain(Message *msg_ker_domain)
{
  Message *msg_mod_domain;
  if  (mem_check_module_domain((uint8_t*)msg_ker_domain) == false){
	DEBUG("Message header in kernel domain ... moving to module domain\n");
	// Allocate space for the new message header
	msg_mod_domain = (Message*)module_domain_alloc(sizeof(Message), MSG_QUEUE_PID);
	if (NULL == msg_mod_domain){
	  DEBUG("Out of memory in module domain ... failed to move message\n");
	  return NULL;
	}
	// Copy the message header over
	memcpy(msg_mod_domain, msg_ker_domain, sizeof(Message));
	// Check if the original message was a short message
	// Fix the data pointer, free the old message and return new message
	if (msg_ker_domain->data == msg_ker_domain->payload){
	  DEBUG("Internal payload ... message moved\n");
	  msg_mod_domain->data = msg_mod_domain->payload;
	  DEBUG("Freeing the header located in kernel domain\n");
	  ker_free(msg_ker_domain);
	  return msg_mod_domain;
	}
  }
  else {
	DEBUG("Message header in module domain\n");
	if (msg_ker_domain->data == msg_ker_domain->payload){
	  DEBUG("Internal payload ... message moved\n");
	  return msg_ker_domain;
	}
	msg_mod_domain = msg_ker_domain;
  }
  // Check if the data lies in the module domain
  // Copy message payload over to the module domain if it is in the kernel domain
  if (mem_check_module_domain(msg_ker_domain->data) == false){
	DEBUG("Message payload in kernel domain ... moving to module domain\n");
	if (msg_ker_domain->len > 0){
	  uint8_t* newdata = (uint8_t*)ker_malloc(msg_ker_domain->len, msg_ker_domain->did);
	  if (NULL == newdata){
		DEBUG("Out of memory in module domain ...failed to move message\n");
		if (msg_mod_domain != msg_ker_domain){
		  DEBUG("Free the header that was moved into the module domain\n");
		  ker_free(msg_mod_domain);
		  return NULL;
		}
	  }
	  memcpy(newdata, msg_ker_domain->data, msg_ker_domain->len);
	  if (flag_msg_release(msg_ker_domain->flag)){
		DEBUG("Free the message payload located in kernel domain\n");
		ker_free(msg_ker_domain->data);
	  }
	  msg_mod_domain->data = newdata;
	  msg_mod_domain->flag |= SOS_MSG_RELEASE;
	}
	else{
	  msg_mod_domain->data = NULL;
	  ker_free(msg_ker_domain->data); // Just to be sure. This should be NULL anyway
	}
  }
  if (msg_mod_domain != msg_ker_domain){
	DEBUG("Free the header located in kernel domain\n");
	ker_free(msg_ker_domain);
  }
  return msg_mod_domain;
}
#endif //FAULT_TOLERANT_SOS

/**
 * @brief dispose message
 * return message header back to message repostitary
 */
void msg_dispose(Message *m)
{
	HAS_CRITICAL_SECTION;
	msg_pool_item *prev = NULL;
	msg_pool_item *itr;


	if(flag_msg_release(m->flag)) { 
		ker_free(m->data); 
	}

	ENTER_CRITICAL_SECTION();
	itr = &msg_pool;
	while( itr != NULL ) {
		if( m >= itr->pool && m < (itr->pool + MSG_QUEUE_NUM_ITEMS) ) {
			uint8_t mask = 1 << (m - itr->pool);
			itr->alloc &= ~mask;	
			DEBUG("MQ: free %x of item %d\n", (unsigned int)itr, m - itr->pool);
			if( (itr->alloc == 0) && (itr != (&msg_pool))) {
				DEBUG("MQ: free one pool\n");
				ker_free(itr);
				prev->next = NULL;
			}
			LEAVE_CRITICAL_SECTION();
			return;
		}
		prev = itr;
		itr = itr->next;
	}
	if( itr == NULL ) {
		// 
		// Cannot find the item in the pool, call ker_free().  
		// this should never happen.
		ker_free(m);
	}
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


