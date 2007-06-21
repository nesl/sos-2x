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
 */
#ifndef _MESSAGE_QUEUE_H
#define _MESSAGE_QUEUE_H
#include <sos_types.h>
#include <message_types.h>

/*
 * WARNING:
 *   These functions are meant for SOS kernel
 *   Application Module should not normally need to use this.
 */


/**
 * @brief data structure for priority message queue
 */
typedef struct {
#ifdef SOS_USE_PREEMPTION
  //! msg queue
  Message *head;
  //! number of messages in the queue.
  //! this is for use by different apps.
  uint8_t msg_cnt;  
#else
  //! number of messages in the queue.
  //! this is used to reduce dequeue overhead
  uint8_t msg_cnt;
  uint8_t lm_cnt;
  uint8_t sm_cnt;
  uint8_t hm_cnt;
  //! high priority queue
  Message *hq_head;
  Message *hq_tail;
  //! system queue
  Message *sq_head;
  Message *sq_tail;
  //! low priority queue
  Message *lq_head;
  Message *lq_tail;
#endif
} mq_t;

extern int8_t msg_queue_init(void);

/**
 * @brief initialize message queue
 * this function always succeed
 */
extern void mq_init(mq_t *q);
/**
 * @brief enqueue message
 * This function always succeed
 */
extern void mq_enqueue(mq_t *q, Message *m);
/**
 * @brief dequeue message
 * @return pointer to message, or NULL for empty queue
 */
extern Message *mq_dequeue(mq_t *q);
/**
 * @brief get message that matches the header in the queue
 *
 * NOTE it matches only daddr, saddr, did, sid, type
 * NOTE it only gets the first that matches the description
 */
extern Message *mq_get(mq_t *q, Message *m);
/**
 * @brief create message
 * @return pointer to message, or NULL for fail
 * get new message header from message repositary
 */
extern Message *msg_create(void);
/**
 * @brief dispose message
 * return message header back to message repostitary
 * this function always succeed
 */
extern void msg_dispose(Message *m);
/**
 * @brief handle the process of creating senddone message
 * @param msg_sent  the Message just sent or delivered
 * @param succ      is the delivery successful?
 * @param msg_owner the owner of the message
 * NOTE the implementation will need to improve
 */
extern void msg_send_senddone(Message *msg_sent, bool succ, sos_pid_t msg_owner);

extern void mq_gc_mark_payload( mq_t *q, sos_pid_t pid );

extern void mq_gc_mark_hdr( mq_t *q, sos_pid_t pid );

extern void mq_gc_mark_one_hdr( Message *msg );

extern void mq_gc( void );
#endif //_MESSAGE_QUEUE_H
