/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 smartindent: */
/*                                  tab:4
 * "Copyright (c) 2000-2003 The Regents of the University  of California.
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 *
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Copyright (c) 2002-2003 Intel Corporation
 * All rights reserved.
 *
 * This file is distributed under the terms in the attached INTEL-LICENSE
 * file. If you do not find these files, copies can be found by writing to
 * Intel Research Berkeley, 2150 Shattuck Avenue, Suite 1300, Berkeley, CA,
 * 94704.  Attention:  Intel License Inquiry.
 */
/**
 * @author Ram Kumar
 * @author Simon Han
 * @brief Port to sos-1.x
 */
#include <sys_module.h>
//#include <module.h>
#include "tree_routing.h"
#include <string.h>
#include <routing/neighbor/neighbor.h>
#define LED_DEBUG
#include <led_dbg.h>

#ifndef SOS_TREE_ROUTING_DEBUG
#undef DEBUG
#define DEBUG(...)
#endif

//-------------------------------------------------------------
// MODULE STATE DECLARATION
//-------------------------------------------------------------
/**
 * @struct tree_route_state_t
 * @brief State of tree routing module
 */
typedef struct {                                                           
	tr_shared_t sr;   // shared state
	int16_t seq_no;	
	uint16_t children[10];
	uint8_t curr_child;
} tree_route_state_t;   

//-------------------------------------------------------------
// MODULE TIMERS
//-------------------------------------------------------------



//-------------------------------------------------------------
// MODULE STATIC FUNCTIONS
//-------------------------------------------------------------
static int8_t tree_routing_module(void *state, Message *msg);
static uint8_t tr_get_hdr_size(func_cb_ptr p) ; 
static uint32_t evaluateCost(uint8_t sendEst, uint8_t receiveEst) ;
static void choose_parent(tree_route_state_t *s) ;
static int8_t tr_send_data(tree_route_state_t *s, uint8_t msg_len, uint16_t saddr, tr_hdr_t* hdr);
#ifdef PC_PLATFORM
#endif //PC_PLATFORM


//-------------------------------------------------------------
// MODULE HEADER
//-------------------------------------------------------------
static const mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         =  TREE_ROUTING_PID,
  .state_size     =  sizeof(tree_route_state_t),
  .num_timers     =  0,
  .num_sub_func   =  0,
  .num_prov_func  =  1,
  .code_id        =  ehtons(TREE_ROUTING_PID),
  .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
  .processor_type = MCU_TYPE,
  .module_handler =  tree_routing_module,
  .funct = {
	[0] = {tr_get_hdr_size, "Cvv0", TREE_ROUTING_PID, MOD_GET_HDR_SIZE_FID},
  },
};

//-------------------------------------------------------------
// MODULE 
//-------------------------------------------------------------
static int8_t tree_routing_module(void *state, Message *msg)
{
  // Cast state into correct structure
  tree_route_state_t *s = (tree_route_state_t*) state;

  switch (msg->type) {

  case MSG_INIT:
	{

	  // Initialize the state
	  s->seq_no = 0;

	  sys_led(LED_RED_OFF);
	  sys_led(LED_YELLOW_OFF);
	  sys_led(LED_GREEN_OFF);

	  uint8_t i;
	  s->children[0] = BCAST_ADDRESS;
	  for (i = 1; i < 10;i++)
			s->children[i] = sys_id();
	  s->curr_child = 0;

	  if(sys_id() == BASE_STATION_ADDRESS) {
		  s->sr.parent = sys_id();
		  s->sr.hop_count = 0;
	  } else {
		s->sr.parent = BCAST_ADDRESS;
		s->sr.hop_count = ROUTE_INVALID;
	  }
	  sys_shm_open( sys_shm_name(TREE_ROUTING_PID, SHM_TR_VALUE), &(s->sr));
	  sys_shm_wait( sys_shm_name(NBHOOD_PID, SHM_NBR_LIST) );
	  //
	  // Wait on neighbor update
	  //
	  return SOS_OK;
	}

  case MSG_NEW_CHILD:
	{
	  uint16_t new_child;
	  uint8_t new_index = 10;
	  uint8_t i;

	  new_child = msg->saddr; 
      //sys_led(LED_YELLOW_TOGGLE);
	  //sys_led(LED_RED_TOGGLE);
      for (i=0;i<10;i++){
		  if (s->children[i] == new_child)
			  return SOS_OK;
		  else if (s->children[i] == sys_id() || s->children[i] == BCAST_ADDRESS ){
			  new_index = i;
		      break;
		  }
	  }

	  if (new_index < 10)
		  s->children[new_index] = new_child;
	  // TODO: add some stuff to remove dead children?
	}

  case MSG_TR_DATA_PKT:
	{
	  uint16_t my_id = sys_id();
	  DEBUG("<TR> RECV DATA from %d to %d\n", msg->saddr, msg->daddr);
	  //sys_led(LED_YELLOW_TOGGLE);
	  if(msg->daddr == my_id){
		// Packet was addressed to us
		uint8_t msg_len = msg->len;
		tr_hdr_t *hdr = (tr_hdr_t*) sys_msg_take_data(msg);
		if(my_id == BASE_STATION_ADDRESS) {
		  // At base station, send pkt to tree routing client
		  DEBUG("<TR> src = %d, hop = %d\n", 
				entohs(hdr->originaddr),
				hdr->originhopcount);
		  sys_post(hdr->dst_pid,
					MSG_TR_DATA_PKT, msg_len, hdr, 	
					SOS_MSG_RELEASE);
		  return SOS_OK;
		} else {
		  // Forward the packet
		  return tr_send_data(s, msg_len, msg->saddr, hdr);
		}
	  } 
	  break;
	}
  case MSG_SEND_PACKET:
	{
	  // Send out packet
	  uint8_t msg_len = msg->len;
 	  tr_hdr_t *hdr = (tr_hdr_t*)sys_msg_take_data(msg);
 	  hdr->originaddr = ehtons(sys_id()); 
 	  hdr->hopcount = ROUTE_INVALID; 
 	  hdr->dst_pid = msg->sid; 
 	  hdr->originhopcount = s->sr.hop_count;
	  hdr->parentaddr = s->sr.parent;
      DEBUG("<TR> Request to send data\n");
	  return tr_send_data(s, msg_len, msg->saddr, hdr);
	}

  case MSG_SEND_TO_CHILDREN:
	{
	  uint8_t msg_len = msg->len;
	  uint8_t *payload = sys_msg_take_data(msg);

	  while (s->curr_child < 10 && s->children[s->curr_child] == sys_id())
		  s->curr_child++;

	  if (s->curr_child == 10){
		  sys_free(payload);
		  s->curr_child = 0;
		  DEBUG("<TR> Request to send data to children complete\n");
		  return SOS_OK;
	  }

	  uint8_t *hdr = sys_malloc(msg_len); 
	  if (hdr){
		  memcpy(hdr, payload, msg_len);

		  //sys_led(LED_RED_TOGGLE);
		  DEBUG("<TR> Request to send data to child %d\n", curr_child);

		  sys_post_net(
				 msg->sid, 
				 (MOD_MSG_START + 1),
				 msg_len, 
				 hdr,     
				 SOS_MSG_RELEASE, 
				 s->children[s->curr_child]);
		  s->curr_child++;
	  }
	  sys_post(TREE_ROUTING_PID, MSG_SEND_TO_CHILDREN, msg_len, payload, SOS_MSG_RELEASE);
	  return SOS_OK;
	}

  case MSG_SHM:
	{
		DEBUG("SHM update\n");
		choose_parent(s);
		return SOS_OK;
	}

  case MSG_FINAL:
	{
	  sys_shm_stopwait( sys_shm_name(NBHOOD_PID, SHM_NBR_LIST) );
	  sys_shm_close( sys_shm_name(TREE_ROUTING_PID, SHM_TR_VALUE));
	  return SOS_OK;
	}

  default: return -EINVAL;
  }
  return SOS_OK;
}

//-------------------------------------------------------------
// MODULE STATIC FUNCTION IMPLEMENTATIONS
//-------------------------------------------------------------

static uint32_t evaluateCost(uint8_t sendEst, uint8_t receiveEst)
{
  uint32_t transEst = (uint32_t) sendEst * (uint32_t) receiveEst;
  uint32_t immed = ((uint32_t) 1 << 24);

  if (transEst == 0) return ((uint32_t) 1 << (uint32_t) 16);
  // DO NOT change this LINE! mica compiler is WEIRD!
  immed = immed / transEst;
  return immed;
}

static void choose_parent(tree_route_state_t *s)
{
  uint32_t ulNbrLinkCost = (uint32_t) -1;
  uint32_t ulMinLinkCost = (uint32_t) -1;
  nbr_entry_t * pNewParent = NULL;
  uint8_t bNewHopCount = ROUTE_INVALID;
  nbr_entry_t *nb;

  if (sys_id() == BASE_STATION_ADDRESS) return;
  nb = sys_shm_get( sys_shm_name(NBHOOD_PID, SHM_NBR_LIST) );
  if( nb == NULL ) {
	return;
  }

  // Choose the parent based on minimal hopcount and cost.  
  // There is a special case for choosing a base-station as it's 
  // receiveEst may be zero (it's not sending any packets)

  while(nb != NULL) {
	if (nb->parent == sys_id()) {nb = nb->next; continue;}
	if (nb->parent == BCAST_ADDRESS) {nb = nb->next; continue;}
	if (nb->hop == ROUTE_INVALID) {nb = nb->next; continue;}
	if (nb->sendEst < 25) {nb = nb->next; continue;}
	if ((nb->hop != 0) && (nb->receiveEst < 25)) {nb = nb->next; continue;}

	ulNbrLinkCost = evaluateCost(nb->sendEst,nb->receiveEst);

	if ((nb->hop != 0) && (ulNbrLinkCost > MAX_ALLOWABLE_LINK_COST)) 
	  {nb = nb->next; continue;}

	if ((nb->hop < bNewHopCount) || 
		((nb->hop == bNewHopCount) && ulMinLinkCost > ulNbrLinkCost)) {
	  ulMinLinkCost = ulNbrLinkCost;
	  pNewParent = nb;
	  bNewHopCount = nb->hop;
	}
	nb = nb->next;
  }

  if (pNewParent) {
#ifdef PC_PLATFORM
	  if(s->sr.parent != BCAST_ADDRESS) {
		  if(s->sr.parent != pNewParent->id) {
			  DEBUG("parent now = %d\n", pNewParent->id);
		  }
	  } else {
		  DEBUG("parent now = %d\n", pNewParent->id);
	  }
#endif
	s->sr.parent = pNewParent->id;
	s->sr.hop_count = bNewHopCount + 1;
	
	// inform new parent that we are now a child
	uint16_t *my_id;
	my_id = (uint16_t *) sys_malloc(sizeof(uint16_t));
	*my_id = sys_id();

	sys_post_net(TREE_ROUTING_PID, MSG_NEW_CHILD, sizeof(uint16_t),my_id,SOS_MSG_RELEASE, s->sr.parent);
  }
#ifdef PC_PLATFORM
  else {
	DEBUG("there is no parent\n");
	if(s->sr.parent != BCAST_ADDRESS) {
		DEBUG("but parent ID is set to %d\n", s->sr.parent);
	}
  }
#endif

}

static int8_t tr_send_data(tree_route_state_t *s, uint8_t msg_len, uint16_t saddr, tr_hdr_t* hdr)
{
  //  uint8_t msg_len = msg->len;
  //  tr_hdr_t *hdr = (tr_hdr_t*)sys_msg_take_data(msg);
  //uint16_t my_id = sys_id();
  if(hdr == NULL) return -ENOMEM;
  
  if(s->sr.parent == BCAST_ADDRESS) {
	 /* 
	if(my_id == entohs(hdr->originaddr)) {
	  hdr->seqno = ehtons(s->seq_no++);                    
	  hdr->hopcount = s->sr.hop_count;
	  //DEBUG("<SEND> Data, seq = %d\n", hdr->seqno);
	  sys_post_net(TREE_ROUTING_PID, 
			   MSG_TR_DATA_PKT, msg_len, hdr,     
			   SOS_MSG_RELEASE, BCAST_ADDRESS);
	  DEBUG_PID(TREE_ROUTING_PID, "No parent - Broadcasting data from local node\n");
	  return SOS_OK;  
	} else {
	  sys_free(hdr);
	  return -EINVAL;
	}
	*/
	  sys_free(hdr);
	  return SOS_OK;
  } 
  
  if(s->sr.hop_count >= hdr->hopcount) {
	//Possible cycle??
	sys_free(hdr);
	return -EINVAL;
  }
  
	hdr->hopcount = s->sr.hop_count;

	hdr->seqno = ehtons(s->seq_no++); 
	sys_post_net(TREE_ROUTING_PID, 
			 MSG_TR_DATA_PKT, msg_len, hdr,     
			 SOS_MSG_RELEASE, s->sr.parent);
	DEBUG("Forward data: %d <-- %d, orig(%d)\n", 
		  s->sr.parent, sys_id(), entohs(hdr->originaddr));
	return SOS_OK;		
}

// Dynamic function that will actually route the message
/*
static int8_t tree_route_msg(char* proto, sos_pid_t did, uint8_t length, void* data)
{
  tr_hdr_t *hdr = (tr_hdr_t*)data;
  hdr->originaddr = ehtons(sys_id());
  hdr->hopcount = ROUTE_INVALID;
  hdr->dst_pid = did;
  return post_long(TREE_ROUTING_PID, TREE_ROUTING_PID, MSG_SEND_PACKET, length, data, SOS_MSG_DYM_MANAGED);
} 
*/



// Dynamic Function that will return the size of the routing header
static uint8_t tr_get_hdr_size(func_cb_ptr p)
{
  return sizeof(tr_hdr_t);
}


#ifndef _MODULE_
mod_header_ptr tree_routing_get_header()
{
   return sos_get_header_address(mod_header);
}
#endif


