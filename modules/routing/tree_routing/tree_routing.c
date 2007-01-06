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
 * @brief Port to sos-1.x
 */
#include <sys_module.h>
#include <module.h>
#include "tree_routing.h"
#ifdef VM_EXTENSION_ENABLE
#include <VM/Dvm.h>
#endif
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
 * @note Size is 15 bytes 
 */
typedef struct {                                                           
  uint16_t est_ticks;                                                      
  int16_t gCurrentSeqNo;                                                   
  uint8_t gbCurrentHopCount;                                               
  neighbor_entry_t *gpCurrentParent;                                       
  neighbor_entry_t *nb_list;                                               
  uint8_t nb_cnt;                                                          
} tree_route_state_t;   

//-------------------------------------------------------------
// MODULE TIMERS
//-------------------------------------------------------------
enum 
{
   TREE_NEIGHBOR_TIMER = 0,
   TREE_BACKOFF_TIMER  = 1,
};



//-------------------------------------------------------------
// MODULE STATIC FUNCTIONS
//-------------------------------------------------------------
static int8_t tree_routing_module(void *state, Message *msg);
static void init_nb(neighbor_entry_t *nb, uint16_t id) ;
static uint8_t tr_get_hdr_size(func_cb_ptr p) ; 
static neighbor_entry_t* update_neighbor(tree_route_state_t *s, uint16_t saddr, int16_t seqno, bool *duplicate) ;
static void update_est(neighbor_entry_t *nb) ;
static void update_table(tree_route_state_t *s) ;
static uint32_t evaluateCost(uint8_t sendEst, uint8_t receiveEst) ;
static void choose_parent(tree_route_state_t *s) ;
static neighbor_entry_t* get_neighbor(tree_route_state_t *s, uint16_t saddr) ;
static void send_beacon(tree_route_state_t *s) ;
static void recv_beacon(tree_route_state_t *s, Message *msg) ;
//static int8_t tr_send_data(tree_route_state_t *s, Message *msg) ;
static int8_t tr_send_data(tree_route_state_t *s, uint8_t msg_len, uint16_t saddr, tr_hdr_t* hdr);
#ifdef VM_EXTENSION_ENABLE
static uint8_t vm_unmarshall_msg_len(Message* msg);
static tr_hdr_t* vm_unmarshall_tr_hdr(Message* msg);
#endif//VM_EXTENSION_ENABLE
#ifdef PC_PLATFORM
#ifdef SOS_TREE_ROUTING_DEBUG
static void tr_debug(tree_route_state_t *s);
#endif //TR_DEBUG
#endif //PC_PLATFORM


//-------------------------------------------------------------
// MODULE HEADER
//-------------------------------------------------------------
static mod_header_t mod_header SOS_MODULE_HEADER = {
  .mod_id         =  TREE_ROUTING_PID,
  .state_size     =  sizeof(tree_route_state_t),
  .num_timers     =  2,
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
	  // Initialize and start the one shot timer
	  ker_timer_init(TREE_ROUTING_PID,
					 TREE_BACKOFF_TIMER,
					 TIMER_ONE_SHOT);
	  ker_timer_start(TREE_ROUTING_PID,
					  TREE_BACKOFF_TIMER,
					  ker_rand() % 1024L);


	  // get promiscuous packet from our neighbor
	  ker_msg_change_rules(TREE_ROUTING_PID, SOS_MSG_RULES_PROMISCUOUS);

	  // Initialize the state
	  s->nb_list = NULL;
	  s->nb_cnt = 0;
	  s->est_ticks = 0;
	  s->gCurrentSeqNo = 0;
	  s->gpCurrentParent = NULL;
	  s->gbCurrentHopCount = ROUTE_INVALID;

	  if(ker_id() == BASE_STATION_ADDRESS) {
		// basestation node
		neighbor_entry_t *nb = (neighbor_entry_t*) 
		  sys_malloc(sizeof(neighbor_entry_t));
		if(nb != NULL) {
		  init_nb(nb, ker_id());
		  nb->parent = ker_id();
		  nb->flags = NBRFLAG_VALID;
		  nb->hop = 0;
		  s->gpCurrentParent = nb;      
		  s->gbCurrentHopCount = 0;	
		} else {
		  // PANIC!!!
		  DEBUG("<TR> NO Memory for tree routing!!!\n");
		}
	  }
	  return SOS_OK;
	}


  case MSG_TIMER_TIMEOUT:
	{
	  MsgParam *param = (MsgParam*) (msg->data);
	  switch (param->byte) {

		// Random backoff timer
	  case TREE_BACKOFF_TIMER:
		{
		  // Release the pre-allocated timer
		  ker_timer_release(TREE_ROUTING_PID, TREE_BACKOFF_TIMER);
		  // timer for computing neighbor
		  ker_timer_init(TREE_ROUTING_PID,
						 TREE_NEIGHBOR_TIMER,
						 TIMER_REPEAT);
		  ker_timer_start(TREE_ROUTING_PID, 
						  TREE_NEIGHBOR_TIMER, 
						  DATA_TO_ROUTE_RATIO * DATA_FREQ);
                
		  return SOS_OK;
		}


		// Neighborhood timer
	  case TREE_NEIGHBOR_TIMER:
		{
		  // update table
		  update_table(s);
		  // choose new parent
		  choose_parent(s);
#if (defined PC_PLATFORM || defined QUALNET_PLATFORM)
#ifdef SOS_TREE_ROUTING_DEBUG
		  tr_debug(s);
#endif
#endif
		  // send beacon packet
		  post_short(TREE_ROUTING_PID, TREE_ROUTING_PID, 
					 MSG_BEACON_SEND, 0, 0, 0);
		  return SOS_OK;
		}
	  }
	}

  case MSG_TR_DATA_PKT:
	{
	  uint16_t my_id = ker_id();
	  DEBUG("<TR> RECV DATA from %d to %d\n", msg->saddr, msg->daddr);
	  if(msg->daddr == my_id){
		// Packet was addressed to us
		uint8_t msg_len = msg->len;
		tr_hdr_t *hdr = (tr_hdr_t*) sys_msg_take_data(msg);
		if(my_id == BASE_STATION_ADDRESS) {
		  // At base station, send pkt to tree routing client
		  bool dup;
		  if(hdr == NULL) return -ENOMEM;
		  if(msg->saddr != my_id){
			update_neighbor(s, msg->saddr, entohs(hdr->seqno), &dup);
		  }
		  DEBUG("<TR> src = %d, hop = %d\n", 
				entohs(hdr->originaddr),
				hdr->originhopcount);
#ifdef VM_EXTENSION_ENABLE
		  // Ram - Hack for IPSN Paper ... VM should register an event handler
		  sys_post_uart(DFLT_APP_ID1, msg->type, msg_len, hdr, SOS_MSG_RELEASE, BCAST_ADDRESS);
#else
		  post_long(hdr->dst_pid, TREE_ROUTING_PID, 
					MSG_TR_DATA_PKT, msg_len, hdr, 	
					SOS_MSG_RELEASE);
#endif
		  return SOS_OK;
		} else {
		  // Forward the packet
		  return tr_send_data(s, msg_len, msg->saddr, hdr);
		  //		  return tr_send_data(s, msg);
		}
	  } else {
		bool dup;
		tr_hdr_t *hdr = (tr_hdr_t*) msg->data;
		update_neighbor(s, msg->saddr, entohs(hdr->seqno), &dup);
		return SOS_OK;
	  }
	  break;
	}


#ifdef VM_EXTENSION_ENABLE
  case POST_EXECUTE:
	{
	  uint8_t msg_len = vm_unmarshall_msg_len(msg);
	  tr_hdr_t *hdr = vm_unmarshall_tr_hdr(msg);
	  if (NULL == hdr) return SOS_OK;
	  hdr->originaddr = ehtons(ker_id()); 
 	  hdr->hopcount = ROUTE_INVALID; 
 	  hdr->dst_pid = msg->sid; 
 	  hdr->originhopcount = s->gbCurrentHopCount;
      if (s->gpCurrentParent != NULL)
         hdr->parentaddr = s->gpCurrentParent->id;
      else
         hdr->parentaddr = BCAST_ADDRESS;
      DEBUG("<TR> Request to send data\n");
	  return tr_send_data(s, msg_len, msg->saddr, hdr);
	}
#endif

  case MSG_SEND_PACKET:
	{
	  // Send out packet
	  uint8_t msg_len = msg->len;
 	  tr_hdr_t *hdr = (tr_hdr_t*)sys_msg_take_data(msg);
 	  hdr->originaddr = ehtons(ker_id()); 
 	  hdr->hopcount = ROUTE_INVALID; 
 	  hdr->dst_pid = msg->sid; 
 	  hdr->originhopcount = s->gbCurrentHopCount;
      if (s->gpCurrentParent != NULL)
         hdr->parentaddr = s->gpCurrentParent->id;
      else
         hdr->parentaddr = BCAST_ADDRESS;
      DEBUG("<TR> Request to send data\n");
	  return tr_send_data(s, msg_len, msg->saddr, hdr);
	  //	  return tr_send_data(s, msg);
	}


  case MSG_BEACON_SEND:
	{
	  send_beacon(s);
	  return SOS_OK;
	}


  case MSG_BEACON_PKT:
	{
	  recv_beacon(s, msg);
	  return SOS_OK;
	}


  case MSG_FINAL:
	{
	  ker_timer_release(TREE_ROUTING_PID, TREE_NEIGHBOR_TIMER);
	  return SOS_OK;
	}

  default: return -EINVAL;
  }
  return SOS_OK;
}

//-------------------------------------------------------------
// MODULE STATIC FUNCTION IMPLEMENTATIONS
//-------------------------------------------------------------
static void update_est(neighbor_entry_t *nb)
{
  uint16_t usExpTotal, usActTotal, newAve;
  if(nb->flags & NBRFLAG_NEW)	return;

  usExpTotal = ESTIMATE_TO_ROUTE_RATIO;

  usActTotal = nb->received + nb->missed;

  if (usActTotal < ESTIMATE_TO_ROUTE_RATIO) {
	usActTotal = ESTIMATE_TO_ROUTE_RATIO;
  }

  newAve = ((uint16_t) 255 * (uint16_t)nb->received) / (uint16_t)usActTotal;
  nb->missed = 0;
  nb->received = 0;

  // If we haven't seen a recieveEst for us from our neighbor, decay our sendEst
  // exponentially
  if (nb->liveliness < MIN_LIVELINESS) {
	nb->sendEst <<= 1;
  }
  nb->liveliness = 0;

  if (nb->flags & NBRFLAG_EST_INIT) {
	uint16_t tmp;
	tmp = ((2 * ((uint16_t)nb->receiveEst) + (uint16_t)newAve * 6) / 8);
	nb->receiveEst = (uint8_t)tmp;
  }
  else {
	nb->receiveEst = (uint8_t) newAve;
	nb->flags ^= NBRFLAG_EST_INIT;
  }
}

static void update_table(tree_route_state_t *s)
{
  neighbor_entry_t *nb;
  s->est_ticks++;
  s->est_ticks %= ESTIMATE_TO_ROUTE_RATIO;
  if(s->est_ticks != 0) return;

  // update table
  nb = s->nb_list;
  while(nb != NULL) {
	update_est(nb);
	nb = nb->next;
  }
}

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
  neighbor_entry_t *nb;
  //TableEntry *pNbr;
  uint32_t ulNbrLinkCost = (uint32_t) -1;
  uint32_t ulMinLinkCost = (uint32_t) -1;
  neighbor_entry_t * pNewParent = NULL;
  uint8_t bNewHopCount = ROUTE_INVALID;

  if (ker_id() == BASE_STATION_ADDRESS) return;

  // Choose the parent based on minimal hopcount and cost.  
  // There is a special case for choosing a base-station as it's 
  // receiveEst may be zero (it's not sending any packets)

  nb = s->nb_list;
  while(nb != NULL) {
	if (nb->parent == ker_id()) {nb = nb->next; continue;}
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
	  if(s->gpCurrentParent != NULL) {
		  if(s->gpCurrentParent->id != pNewParent->id) {
			  DEBUG_PID(TREE_ROUTING_PID, "parent now = %d\n", pNewParent->id);
		  }
	  } else {
		  DEBUG_PID(TREE_ROUTING_PID, "parent now = %d\n", pNewParent->id);
	  }
#endif
	s->gpCurrentParent = pNewParent;
	s->gbCurrentHopCount = bNewHopCount + 1;
  }
#ifdef PC_PLATFORM
  else {
	DEBUG_PID(TREE_ROUTING_PID, "there is no parent\n");
	if(s->gpCurrentParent != NULL) {
		DEBUG_PID(TREE_ROUTING_PID, "but parent ID is set to %d\n", s->gpCurrentParent->id);
	}
  }
#endif

}

/**
 * This routine is probably bad for inline
 */
static void init_nb(neighbor_entry_t *nb, uint16_t id)
{
  nb->id = id;
  nb->flags = (NBRFLAG_VALID | NBRFLAG_NEW);
  nb->liveliness = 0;
  nb->parent = BCAST_ADDRESS;
  nb->hop = ROUTE_INVALID;
  nb->missed = 0;
  nb->received = 0;
  nb->receiveEst = 0;
  nb->sendEst = 0;
}

static neighbor_entry_t* get_neighbor(tree_route_state_t *s, uint16_t saddr)
{
  neighbor_entry_t *nb = s->nb_list;

  // search node
  while (nb != NULL) {
	if(nb->id == saddr) return nb;
	nb = nb->next;
  }
  // node not found, create one
  if(s->nb_cnt < MAX_NB_CNT) {
	nb = (neighbor_entry_t*) sys_malloc(sizeof(neighbor_entry_t));
	if(nb == NULL) return NULL;
	init_nb(nb, saddr);
	nb->next = s->nb_list;
	s->nb_list = nb;
	s->nb_cnt++;
	return nb;
  } 
  // node not found, but already reach max neighbor cnt
  // find the lowest number 
  {
	neighbor_entry_t *new_nb = s->nb_list;
	uint8_t minSendEst = s->nb_list->sendEst;

	nb = s->nb_list;
	while(nb != NULL) {
	  if(nb->sendEst < minSendEst) {
		new_nb = nb;
		minSendEst = nb->sendEst;
	  }
	  nb = nb->next;
	}
	init_nb(new_nb, saddr);
	return new_nb;
  }
}

/**
 * This routine is probably bad for inline
 */
static neighbor_entry_t* update_neighbor(tree_route_state_t *s, uint16_t saddr, int16_t seqno, bool *duplicate) 
{
  neighbor_entry_t *nb = get_neighbor(s, saddr);
  int16_t sDelta;
  if(nb == NULL) {
	*duplicate = true;
	return NULL;
  }
  *duplicate = false;

  sDelta = (seqno - nb->lastSeqno - 1);

  if(nb->flags & NBRFLAG_NEW) {
	nb->received++;
	nb->lastSeqno = seqno;
	nb->flags ^= NBRFLAG_NEW;
  } else if(sDelta >= 0) {
	nb->missed += sDelta;
	nb->received++;
	nb->lastSeqno = seqno;
  } else if(sDelta < ACCEPTABLE_MISSED) {
	// Something happend to this node.  Reinitialize it's state
	init_nb(nb, saddr);
	nb->received++;
	nb->lastSeqno = seqno;
	nb->flags ^= NBRFLAG_NEW;
  } else {
	*duplicate = true;
  }
  return nb;
}
static void send_beacon(tree_route_state_t *s)
{
  neighbor_entry_t *nb;
  tr_beacon_t *pkt;
  uint8_t pkt_size;
  uint8_t i;
  // sort the table according to recv estimate, NO NEED

  // Count how many in the list
  if(s->nb_cnt > 0) {
	pkt_size = sizeof(est_entry_t) * (s->nb_cnt - 1) + sizeof(tr_beacon_t);
  } else {
	pkt_size = sizeof(tr_beacon_t) - sizeof(est_entry_t);
  }
  pkt = (tr_beacon_t*)sys_malloc(pkt_size);
  if(pkt == NULL) return;

  // pack  nb list
  nb = s->nb_list;
  i = 0;
  while(nb != NULL) {
	pkt->estList[i].id = ehtons(nb->id);
	pkt->estList[i].receiveEst = nb->receiveEst;
	nb = nb->next;
	i++;
  }
  pkt->seqno = ehtons((s->gCurrentSeqNo)++);
  pkt->hopcount = s->gbCurrentHopCount;
  pkt->parent = (s->gpCurrentParent) ? ehtons(s->gpCurrentParent->id) : ehtons(BCAST_ADDRESS);
  pkt->estEntries = s->nb_cnt;
  DEBUG_PID(TREE_ROUTING_PID, "Send Beacon seq = %d\n", entohs(pkt->seqno));
  post_net(TREE_ROUTING_PID, TREE_ROUTING_PID, MSG_BEACON_PKT, pkt_size, pkt, SOS_MSG_RELEASE, BCAST_ADDRESS);
}

static void recv_beacon(tree_route_state_t *s, Message *msg)
{
  tr_beacon_t *pkt = (tr_beacon_t*)(msg->data);
  uint8_t i;
  uint16_t my_addr = ker_id();
  bool dup;
  neighbor_entry_t *nb;

  nb = update_neighbor(s, msg->saddr, entohs(pkt->seqno), &dup);

  if(nb == NULL) return;
  nb->parent = entohs(pkt->parent);
  nb->hop = pkt->hopcount;

  // find out my address, extract the estimation
  for (i = 0; i < pkt->estEntries; i++) {
	if (entohs(pkt->estList[i].id) == my_addr) {
	  //DEBUG("found my entry %d\n", pkt->estList[i].receiveEst);
	  nb->sendEst = pkt->estList[i].receiveEst;
	  nb->liveliness++;
	}
  }
  return;
}

static int8_t tr_send_data(tree_route_state_t *s, uint8_t msg_len, uint16_t saddr, tr_hdr_t* hdr)
{
  //  uint8_t msg_len = msg->len;
  //  tr_hdr_t *hdr = (tr_hdr_t*)sys_msg_take_data(msg);
  bool dup;
  uint16_t my_id = ker_id();
  if(hdr == NULL) return -ENOMEM;
  
  if(s->gpCurrentParent == NULL) {
	if(my_id == entohs(hdr->originaddr)) {
	  hdr->seqno = ehtons(s->gCurrentSeqNo++);                    
	  hdr->hopcount = s->gbCurrentHopCount;
	  //DEBUG("<SEND> Data, seq = %d\n", hdr->seqno);
	  post_net(TREE_ROUTING_PID, TREE_ROUTING_PID,
			   MSG_TR_DATA_PKT, msg_len, hdr,     
			   SOS_MSG_RELEASE, BCAST_ADDRESS);
	  DEBUG_PID(TREE_ROUTING_PID, "No parent - Broadcasting data from local node\n");
	  return SOS_OK;  
	} else {
	  sys_free(hdr);
	  return -EINVAL;
	}
  } 
  
  if(s->gbCurrentHopCount >= hdr->hopcount) {
	//Possible cycle??
	sys_free(hdr);
	return -EINVAL;
  }
  
  if(my_id != entohs(hdr->originaddr)) {
	//	update_neighbor(s, msg->saddr, entohs(hdr->seqno), &dup);
	update_neighbor(s, saddr, entohs(hdr->seqno), &dup);
  } else {
	dup = false;
  }
  
  if(dup == false) {
	hdr->hopcount = s->gbCurrentHopCount;
	if(my_id != entohs(hdr->originaddr)){
	  hdr->seqno = ehtons(s->gCurrentSeqNo++); 
	} else {
	  hdr->seqno = ehtons(s->gCurrentSeqNo); 
	}
	post_net(TREE_ROUTING_PID, TREE_ROUTING_PID,
			 MSG_TR_DATA_PKT, msg_len, hdr,     
			 SOS_MSG_RELEASE, s->gpCurrentParent->id);
	DEBUG_PID(TREE_ROUTING_PID, "Forward data: %d <-- %d, orig(%d)\n", 
		  s->gpCurrentParent->id, node_address, entohs(hdr->originaddr));
	return SOS_OK;		
  } else {
	sys_free(hdr);
	return -EINVAL;
  } 
}

#if (defined PC_PLATFORM || defined QUALNET_PLATFORM)
#ifdef SOS_TREE_ROUTING_DEBUG
static void tr_debug(tree_route_state_t *s)
{
  neighbor_entry_t *nb = s->nb_list;

  DEBUG_PID(TREE_ROUTING_PID, "\taddr\tprnt\tmisd\trcvd\tlstS\thop\trEst\tsEst\n");
  while(nb != NULL) {
	DEBUG_PID(TREE_ROUTING_PID, "\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
		  nb->id,
		  nb->parent,
		  nb->missed,
		  nb->received,
		  nb->lastSeqno,
		  nb->hop,
		  nb->receiveEst,
		  nb->sendEst);
	nb = nb->next;
  }
  if(s->gpCurrentParent) {
	DEBUG_PID(TREE_ROUTING_PID, "TreeRouting: Parent = %d\n", s->gpCurrentParent->id);
  } else {
	DEBUG_PID(TREE_ROUTING_PID, "TreeRouting: Parent = NULL\n");
  }
  DEBUG_SHORT("\n");
}
#endif
#endif

// Dynamic function that will actually route the message
/*
static int8_t tree_route_msg(char* proto, sos_pid_t did, uint8_t length, void* data)
{
  tr_hdr_t *hdr = (tr_hdr_t*)data;
  hdr->originaddr = ehtons(ker_id());
  hdr->hopcount = ROUTE_INVALID;
  hdr->dst_pid = did;
  return post_long(TREE_ROUTING_PID, TREE_ROUTING_PID, MSG_SEND_PACKET, length, data, SOS_MSG_DYM_MANAGED);
} 
*/

#ifdef VM_EXTENSION_ENABLE
static uint8_t vm_unmarshall_msg_len(Message* msg)
{
  post_msg_type *pmsg = (post_msg_type*)(msg->data);
  DvmStackVariable *bufArgs = pmsg->argBuf;
  uint8_t msg_len = sizeof(tr_hdr_t) + (bufArgs->buffer.var->size * sizeof(uint8_t));
  return msg_len;
}

static tr_hdr_t* vm_unmarshall_tr_hdr(Message* msg)
{
  uint8_t i;
  post_msg_type *pmsg = (post_msg_type*)(msg->data);
  if (pmsg->fnid == MSG_SEND_PACKET){
	LED_DBG(LED_GREEN_TOGGLE);
	DvmStackVariable *bufArgs = pmsg->argBuf;
	uint8_t msg_len = sizeof(tr_hdr_t) + (bufArgs->buffer.var->size * sizeof(uint8_t));
	uint8_t* pkt = sys_malloc(msg_len);
	uint8_t *data = (pkt + sizeof(tr_hdr_t));
	for (i = 0; i < bufArgs->buffer.var->size; i++) 
	  data[i] = bufArgs->buffer.var->entries[i];
	return (tr_hdr_t*)pkt;
  }
  else
	LED_DBG(LED_RED_TOGGLE);
  return NULL;
}

#endif


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


