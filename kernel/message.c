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
/**
 * @brief message handling routines
 * @author Simon Han (simonhan@ee.ucla.edu)
 *
 * The routines here are meant to simplify message
 * posting.  Application writer can always
 * fill in message and use post() or call()
 *
 * Simplified interfaces to these functions can be found in message_net.c
 *
 */
#include <string.h>
#include <sos_sched.h>
#include <malloc.h>
#include <message_types.h>
#include <message_queue.h>
#include <sos_info.h>
#include <message.h>
#include <sos_logging.h>

//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Global data declarations
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Funcation declarations
//----------------------------------------------------------------------------
// Post a message with no payload
int8_t post_short(sos_pid_t did, sos_pid_t sid,
				  uint8_t type, uint8_t byte,
				  uint16_t word, uint16_t flag)
{
  Message *m = msg_create();
  MsgParam *p;
  if(m == NULL){
	return -ENOMEM;
  }
  m->daddr = node_address;
  m->did = did;
  m->type = type;
  m->saddr = node_address;
  m->sid = sid;
  m->len = 3;
  p = (MsgParam*)(m->payload);
  p->byte = byte;
  p->word = word;
  m->flag = flag & ((sos_ker_flag_t)(~SOS_MSG_RELEASE));
  sched_msg_alloc(m);
  ker_log( SOS_LOG_POST_SHORT, sid, did ); 
  return SOS_OK;
}

// Post a message with payload and source address
int8_t post_longer(sos_pid_t did, sos_pid_t sid,
				   uint8_t type, uint8_t len,
				   void *data, uint16_t flag,
				   uint16_t saddr)
{
  Message *m = msg_create();
  if(m == NULL){
	if(flag_msg_release(flag)){
	  ker_free(data);
	}
	return -ENOMEM;
  }
  m->daddr = node_address;
  m->did = did;
  m->type = type;
  m->saddr = saddr;
  m->sid = sid;
  m->len = len;
  m->data = (uint8_t*)data;
  m->flag = flag;
  sched_msg_alloc(m);
  ker_log( SOS_LOG_POST_LONG, sid, did ); 
  return SOS_OK;
}

// Post a message with payload
int8_t post_long(sos_pid_t did, sos_pid_t sid,
				 uint8_t type, uint8_t len,
				 void *data, uint16_t flag)
{
    return post_longer(did, sid,
                      type, len,
                      data, flag,
                      node_address);
}

int8_t ker_sys_post(sos_pid_t did, uint8_t type, uint8_t size, void *data, 
		uint16_t flag)
{
	sos_pid_t my_id = ker_get_current_pid();
	if(post_long(did, my_id, type, size, data, flag) != SOS_OK )
	{
		return ker_mod_panic(my_id);
	}
	return SOS_OK;
}


uint8_t *ker_msg_take_data(sos_pid_t pid, Message *msg_in)
{
  uint8_t *ret;
  Message *msg;   //!< message we will be taking data out

  if(msg_in->type == MSG_PKT_SENDDONE) {
	msg = (Message*) (msg_in->data);
  } else {
	msg = msg_in;
  }
  if(flag_msg_release(msg->flag)) {
#ifdef FAULT_TOLERANT_SOS
	if (pid >= APP_MOD_MIN_PID){
	  if (mem_check_module_domain(msg->data) == false){
		DEBUG("Copying payload of message to the module domain\n");
		ret = (uint8_t*)module_domain_alloc(msg->len, pid);
		if (NULL == ret) return NULL;
		memcpy(ret, msg->data, msg->len);
		msg->len = 0;
		ker_free(msg->data);
		msg->data = NULL;
		msg->flag &= ~(SOS_MSG_RELEASE);
		return ret;
	  }
	}
#endif // FAULT_TOLERANT_SOS

	ker_change_own((void*)msg->data, pid);
	ret = msg->data;
	msg->len = 0;
	msg->data = NULL;
	msg->flag &= ~(SOS_MSG_RELEASE);
	return ret;
  } else {
	ret = (uint8_t*)ker_malloc(msg->len, pid);
	if(ret == NULL) return NULL;
	memcpy(ret, msg->data, msg->len);
	return ret;
  }
}

void* ker_sys_msg_take_data(Message *msg)
{
	sos_pid_t my_id = ker_get_current_pid();
	void *ret = ker_msg_take_data(my_id, msg);
	if( ret != NULL ) {
		return ret;
	}
	ker_mod_panic(my_id);
	return NULL;
}

int8_t ker_sys_post_value(sos_pid_t dst_mod_id,
		        uint8_t type, uint32_t data, uint16_t flag)
{
	Message *m = msg_create();
	sos_pid_t my_id = ker_get_current_pid();
	if(m == NULL){ 
		return ker_mod_panic(my_id);
	}
	m->daddr = node_address;
	m->did = dst_mod_id;
	m->type = type;
	m->saddr = node_address;
	m->sid = my_id;
	m->len = 4;
	*((uint32_t*)(m->data)) = data; 	
	m->flag = flag & ((sos_ker_flag_t)(~SOS_MSG_RELEASE));
	sched_msg_alloc(m);

	return SOS_OK;
}


#ifdef PC_PLATFORM
char ker_msg_name[][256] = {
  "init      ",           //! 0
  "debug     ",           //! 1
  "timeout   ",           //! 2
  "senddone  ",           //! 3
  "data ready",           //! 4
  "timer3 to ",           //! 5
  "final     ",           //! 6
  "from user ",           //! 7
  "get data  ",           //! 8
  "send pkt  ",           //! 9
  "dfunc removed",        //! 10
  "func user removed",    //! 11
  "fetcher done",         //! 12
  "module op",            //! 13
  "calibrated data ready" //! 14
  "data failed",          //! 15
};
#endif
