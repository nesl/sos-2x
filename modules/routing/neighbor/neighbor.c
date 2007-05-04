/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */

#include <sys_module.h>

//#define LED_DEBUG
#include <led_dbg.h>

#include "neighbor.h"

#define NEIGHBOR_TIMER_INTERVAL	   (8 * 1024L)
#define MSG_BEACON_PKT             MOD_MSG_START

//
// State
//
typedef struct {
	nbr_entry_t *nb_list;
	int16_t gCurrentSeqNo;                                                   
	uint8_t gbCurrentHopCount;      
	uint8_t nb_cnt;
	uint8_t est_ticks; 
} nbr_state_t;

//
// Function declarations
//
static int8_t nbr_msg_handler(void *start, Message *e);
static void send_beacon(nbr_state_t *s);
static void recv_beacon(nbr_state_t *s, Message *msg);
static void update_table(nbr_state_t *s);
static void init_nb(nbr_entry_t *nb, uint16_t id);
#ifdef PC_PLATFORM
static void tr_debug(nbr_state_t *s);
#else
#define tr_debug(...)
#endif

/**
* This is the only global variable one can have.
 */
static const mod_header_t mod_header SOS_MODULE_HEADER = {
	.mod_id         = NBHOOD_PID,
	.state_size     = sizeof(nbr_state_t),
	.num_sub_func   = 0,
	.num_prov_func  = 0,
	.platform_type  = HW_TYPE /* or PLATFORM_ANY */,
	.processor_type = MCU_TYPE,
	.code_id        = ehtons(DFLT_APP_ID0),
	.module_handler = nbr_msg_handler,
};


static int8_t nbr_msg_handler(void *state, Message *msg)
{
	nbr_state_t *s = (nbr_state_t*)state;
	
	/**
	* Switch to the correct message handler
	 */
	switch (msg->type){
		case MSG_INIT:
		{
		
			s->nb_list = NULL;
			s->nb_cnt = 0;
			s->est_ticks = 0;
			s->gCurrentSeqNo = 0;
			s->gbCurrentHopCount = ROUTE_INVALID;
			
			sys_shm_open( sys_shm_name(NBHOOD_PID, SHM_NBR_LIST), s->nb_list );
			
			sys_timer_start(BACKOFF_TIMER, sys_rand() % 1024L, TIMER_ONE_SHOT);
			break;
		}
			
		case MSG_FINAL:
		{
			break;
		}
			
		case MSG_TIMER_TIMEOUT:
		{
			if( timer_get_tid( msg ) == BACKOFF_TIMER ) {
				sys_timer_start(NEIGHBOR_DISCOVERY_TIMER, NEIGHBOR_TIMER_INTERVAL, TIMER_REPEAT);
			} else {
				//
				// Send beacon packets
				//
				update_table( s );
				sys_shm_update( sys_shm_name(NBHOOD_PID, SHM_NBR_LIST), s->nb_list );
				send_beacon( s );
				tr_debug(s);
			}
			break;
		}
		case MSG_BEACON_PKT:
		{
			//
			// Process beacon packets
			//
			recv_beacon( s, msg );
			break;
		}
			
		default:
			return -EINVAL;
	}
	
	/**
		* Return SOS_OK for those handlers that have successfully been handled.
	 */
	return SOS_OK;
}

static void update_est(nbr_entry_t *nb)
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

static void update_table(nbr_state_t *s)
{
  nbr_entry_t *nb;
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


static void init_nb(nbr_entry_t *nb, uint16_t id)
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

static nbr_entry_t* get_neighbor(nbr_state_t *s, uint16_t saddr)
{
	nbr_entry_t *nb = s->nb_list;
	
	// search node
	while (nb != NULL) {
		if(nb->id == saddr) return nb;
		nb = nb->next;
	}
	// node not found, create one
	if(s->nb_cnt < MAX_NB_CNT) {
		nb = (nbr_entry_t*) sys_malloc(sizeof(nbr_entry_t));
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
		nbr_entry_t *new_nb = s->nb_list;
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

static nbr_entry_t* update_neighbor(nbr_state_t *s, uint16_t saddr, int16_t seqno, bool *duplicate) 
{
	nbr_entry_t *nb = get_neighbor(s, saddr);
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

static void send_beacon(nbr_state_t *s)
{
	nbr_entry_t *nb;
	nbr_beacon_t *pkt;
	uint8_t pkt_size;
	uint8_t i = 0;
	uint16_t *tr_parent;
	// sort the table according to recv estimate, NO NEED
	
	// Count how many in the list
	if(s->nb_cnt > 0) {
		pkt_size = sizeof(est_entry_t) * (s->nb_cnt) + sizeof(nbr_beacon_t);
	} else {
		pkt_size = sizeof(nbr_beacon_t);
	}
	
	pkt = (nbr_beacon_t*)sys_malloc(pkt_size);
	
	if(pkt == NULL) return;
	
	// pack  nb list
	nb = s->nb_list;
	
	while(nb != NULL) {
		pkt->estList[i].id = ehtons(nb->id);
		pkt->estList[i].receiveEst = nb->receiveEst;
		nb = nb->next;
		i++;
	}
	tr_parent = sys_shm_get( sys_shm_name( TREE_ROUTING_PID, 0 ) );
	pkt->seqno = ehtons((s->gCurrentSeqNo)++);
	pkt->hopcount = s->gbCurrentHopCount;
	pkt->parent = (tr_parent) ? ehtons(*tr_parent) : ehtons(BCAST_ADDRESS);
	pkt->estEntries = s->nb_cnt;
	DEBUG("Send Beacon seq = %d\n", entohs(pkt->seqno));
	sys_post_net(NBHOOD_PID, MSG_BEACON_PKT, pkt_size, pkt, SOS_MSG_RELEASE, BCAST_ADDRESS);
}

static void recv_beacon(nbr_state_t *s, Message *msg)
{
	nbr_beacon_t *pkt = (nbr_beacon_t*)(msg->data);
	uint8_t i;
	uint16_t my_addr = sys_id();
	bool dup;
	nbr_entry_t *nb;
	
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

#ifdef PC_PLATFORM
static void tr_debug(nbr_state_t *s)
{
  nbr_entry_t *nb = s->nb_list;

  DEBUG("\taddr\tprnt\tmisd\trcvd\tlstS\thop\trEst\tsEst\n");
  while(nb != NULL) {
    DEBUG("\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
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
  DEBUG_SHORT("\n");
}
#endif


#ifndef _MODULE_
mod_header_ptr neighbor_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif


