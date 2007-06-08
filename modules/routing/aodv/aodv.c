#include <sys_module.h>
#include <module.h>

#define LED_DEBUG
#include <led_dbg.h>

#include "aodv.h"
#include <routing/neighbor/neighbor.h>
#include <string.h>

#ifndef AODV_DEBUG
#undef DEBUG
#define DEBUG(...)
#endif

static int8_t routing_msg_alloc(func_cb_ptr p, Message *msg);

// function declarations
static uint8_t check_cache(AODV_state_t *s, AODV_rreq_pkt_t *hdr);
static void update_cache(AODV_state_t *s, AODV_rreq_pkt_t *hdr, uint16_t saddr);
static void remove_cache_entry(AODV_state_t *s, uint16_t source_addr, uint16_t dest_addr);
static uint8_t check_route(AODV_state_t *s, uint16_t dest_addr, uint16_t dest_seq_no, uint8_t hop_count);
static void update_route(AODV_state_t *s, AODV_rrep_pkt_t *hdr, uint16_t saddr);
static void use_route(AODV_state_t *s, uint16_t dest_addr);
static uint16_t get_reverse_address(AODV_state_t *s, AODV_rrep_pkt_t *hdr);
static uint16_t get_next_hop(AODV_state_t *s, uint16_t dest_addr);
static void remove_inactive_routes(AODV_state_t *s);
static void remove_inactive_cache_entries(AODV_state_t *s);
static uint16_t get_dest_addr(AODV_state_t *s, uint16_t next_hop);
static uint8_t check_dest_addr(AODV_state_t *s, AODV_rerr_pkt_t *hdr);

// 
static uint16_t get_seq_no(AODV_state_t *s, uint16_t addr);
static void update_seq_no(AODV_state_t *s, uint16_t addr, uint16_t seq_no);
static uint8_t add_pending_rreq(AODV_state_t *s, uint16_t addr);

static void del_pending_rreq(AODV_state_t *s, uint16_t addr);
static uint8_t check_pending_rreq(AODV_state_t *s, uint16_t addr);

static void add_to_buffer(AODV_state_t *s, AODV_pkt_t *pkt);
static uint8_t get_from_buffer(AODV_state_t *s, uint16_t dest_addr, AODV_pkt_t ** data_pkt);
static void remove_expired_buffer_entries(AODV_state_t *s);

static uint8_t check_neighbors( AODV_state_t *s, uint16_t addr );

static void aodv_fix_rreq_endian( AODV_rreq_pkt_t *p );
static void aodv_fix_rrep_endian( AODV_rrep_pkt_t *p );
static void aodv_fix_rerr_endian( AODV_rerr_pkt_t *p );
static void aodv_fix_hdr_endian( AODV_hdr_t *p );

static int8_t aodv_module_handler(void *state, Message *msg);

static int8_t aodv_module_handler(void *state, Message *msg);
static const mod_header_t mod_header SOS_MODULE_HEADER = {
    .mod_id         = AODV_PID,
    .state_size     = sizeof(AODV_state_t),
    .num_sub_func   = 0,
    .num_prov_func  = 1,
    .platform_type  = HW_TYPE /* or PLATFORM_ANY */,
    .processor_type = MCU_TYPE,
    .code_id        = ehtons(DFLT_APP_ID0),
    .module_handler = aodv_module_handler,
	.funct          = {
		{routing_msg_alloc,"czv1", AODV_PID, 0},
	},
};


static int8_t aodv_module_handler(void *state, Message *msg)
{
	// Cast state into correct structure
	AODV_state_t *s = (AODV_state_t *) state;

	switch (msg->type) 
	{
		case MSG_INIT:
		{	
			#ifndef SOS_SIM
			//cc1k_cnt_SetRFPower(0x5);
			#endif
			sys_routing_register( 0 );	

			s->seq_no = 0;
			s->broadcast_id = 0;
			
			s->AODV_route_ptr = NULL;
			s->AODV_cache_ptr = NULL;

    		s->num_of_route_entries = 0;
    		s->num_of_cache_entries = 0;
			
	    	s->max_route_entries = AODV_MAX_ROUTE_ENTRIES;
    		s->max_cache_entries = AODV_MAX_CACHE_ENTRIES;
  
			s->AODV_buf_ptr = NULL;
			s->AODV_rreq_ptr = NULL;
			s->AODV_node_ptr = NULL;

			s->num_of_buf_packets = 0;
			s->num_of_rreq = 0;
			s->num_of_nodes = 0;
			
			s->max_buf_packets = AODV_MAX_BUFFER_ENTRIES;
			s->max_rreq = AODV_MAX_RREQ;
			s->max_nodes = AODV_MAX_NODE_ENTRIES;
				
//			DEBUG("[AODV] Initialized node %d at x=%d y=%d\n", sys_id(), (uint32_t)sys_loc_x(), (uint32_t)sys_loc_y());
//			DEBUG("[AODV] node %d: sending hello packet\n", sys_id());

			sys_timer_start(AODV_TIMER, TIMER_INTERVAL, TIMER_REPEAT);
			
			return SOS_OK;
		}
		case MSG_AODV_SEND_RREQ:
		{
			uint32_t *p  = (uint32_t *)msg->data;
			AODV_rreq_pkt_t *hdr = (AODV_rreq_pkt_t *)sys_malloc(sizeof(AODV_rreq_pkt_t));
			
			s->seq_no++;
			s->broadcast_id++;

			DEBUG("[AODV] node %d SEND_RREQ: dest=%d source_seq_no=%d dest_seq_no=%d bcast_id=%d\n",
				sys_id(), *p, s->seq_no, get_seq_no(s, *p), s->broadcast_id);
			
			hdr->source_addr = sys_id();
			hdr->source_seq_no = s->seq_no;
			hdr->broadcast_id = s->broadcast_id;
			hdr->dest_addr = (uint16_t)*p;
			hdr->dest_seq_no = get_seq_no(s, (uint16_t)*p);
			hdr->hop_count = 1;

			aodv_fix_rreq_endian(hdr);
			sys_post_net(AODV_PID, MSG_AODV_RECV_RREQ,
				sizeof(AODV_rreq_pkt_t), hdr, SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO | SOS_MSG_RAW, BCAST_ADDRESS);

			return SOS_OK;
		}		

		case MSG_AODV_RECV_RREQ:
		{
			AODV_rreq_pkt_t *hdr = (AODV_rreq_pkt_t *)msg->data;
			
			aodv_fix_rreq_endian(hdr);
			//if I am the sender, then discard
			if(hdr->source_addr == sys_id())
			{
				return SOS_OK;
			}

			if(check_cache(s, hdr) == FOUND) // RREQ already received
			{
				return SOS_OK;
			}

			DEBUG("[AODV] node %d RECV_RREQ: src=%d src_seq_no=%d dest=%d dest_seq_no=%d bcast_id=%d previous_hop=%d hop_count=%d\n",
				sys_id(), hdr->source_addr, hdr->source_seq_no, hdr->dest_addr, hdr->dest_seq_no, hdr->broadcast_id, msg->saddr, 
				hdr->hop_count);

			//sys_led(LED_YELLOW_TOGGLE);
				
			update_seq_no(s, hdr->source_addr, hdr->source_seq_no);
			
			if(hdr->dest_addr == sys_id())
			{
				if(hdr->dest_seq_no > s->seq_no) 
				{
					// not going to happen -- just a sanity check
					DEBUG("[AODV] node %d ERROR: msg->dest_seq_no(%d) > seq(%d)\n", sys_id(), hdr->dest_seq_no, s->seq_no);
					// HACK fix:
					s->seq_no = hdr->dest_seq_no;
					//return SOS_OK;
				}
	    	
				update_cache(s, hdr, msg->saddr);
				sys_post_value(AODV_PID, MSG_AODV_SEND_RREP, hdr->source_addr, 0); 
				return SOS_OK;
			}
			
			switch(check_route(s, hdr->dest_addr, hdr->dest_seq_no, hdr->hop_count))
			{
				case FOUND_OLDER:  //RREQ seq_no > cache seq_no
				{
					uint8_t l = msg->len;
					void* d = sys_msg_take_data(msg);
					update_cache(s, hdr, msg->saddr);
					sys_post(AODV_PID, MSG_AODV_FWD_RREQ,
						l, d, SOS_MSG_RELEASE);
					return SOS_OK;
				}
				case FOUND_BETTER: //(RREP seq_no,RREP hopcount) < (cache seq_no || cache hopcount)
				case FOUND_LONGER:  //RREQ seq_no < cache seq_no
				{
					update_cache(s, hdr, msg->saddr);
					sys_post_value(AODV_PID, MSG_AODV_SEND_RREP, hdr->source_addr, 0);
					return SOS_OK;
				}			
				case NOT_FOUND:  //entry not in cache
				{
					AODV_rreq_pkt_t *hdr = (AODV_rreq_pkt_t *)msg->data;
					uint8_t l = msg->len;
					void* d = sys_msg_take_data(msg);

					update_cache(s, hdr, msg->saddr);
					post_long(AODV_PID, AODV_PID, MSG_AODV_FWD_RREQ,
						l, d, SOS_MSG_RELEASE);
					
					return SOS_OK;
				}
				default:
					return SOS_OK;
			}
			
			return SOS_OK;
		}

		case MSG_AODV_FWD_RREQ:
		{
			AODV_rreq_pkt_t *hdr = (AODV_rreq_pkt_t *)msg->data;
			uint8_t l = msg->len;
			void *d = sys_msg_take_data(msg);
			
			DEBUG("[AODV] node %d FWD_RREQ: src=%d dest=%d bcast_id=%d hop_count=%d\n",
				sys_id(), hdr->source_addr, hdr->dest_addr, hdr->broadcast_id, hdr->hop_count);
				
			hdr->hop_count++;
			
			aodv_fix_rreq_endian(hdr);
			sys_post_net(AODV_PID, MSG_AODV_RECV_RREQ,
				l, d, SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO | SOS_MSG_RAW, BCAST_ADDRESS);
				
			return SOS_OK;
		}

		case MSG_AODV_SEND_RREP:
		{
			uint16_t p  = (uint16_t)(*(uint32_t *)msg->data);
			
			AODV_rrep_pkt_t *hdr_rrep = (AODV_rrep_pkt_t *)sys_malloc(sizeof(AODV_rrep_pkt_t));

			hdr_rrep->dest_addr = p;
			hdr_rrep->source_addr = sys_id();
			hdr_rrep->dest_seq_no = ++(s->seq_no);
			hdr_rrep->hop_count = 1;

			uint16_t next_hop = get_reverse_address(s, hdr_rrep);
			
			remove_cache_entry(s, hdr_rrep->source_addr, hdr_rrep->dest_addr);
			
			if(next_hop != INVALID_NODE_ID)
			{
				DEBUG("[AODV] node %d SEND_RREP: dest=%d next_hop=%d\n",
					sys_id(), p, next_hop);
				
				aodv_fix_rrep_endian(hdr_rrep);
				sys_post_net(AODV_PID, MSG_AODV_RECV_RREP,
					sizeof(AODV_rrep_pkt_t), hdr_rrep, SOS_MSG_RELEASE | SOS_MSG_RAW | SOS_MSG_LINK_AUTO, next_hop);			
			}
			else
			{
				DEBUG("[AODV] node %d SEND_RREP: dest=%d next_hop=INVALID\n",
					sys_id(), p);
				sys_free(hdr_rrep);
			}
				
			return SOS_OK;
		}
				
		case MSG_AODV_RECV_RREP:
		{
			AODV_pkt_t * data_pkt = NULL;
			AODV_rrep_pkt_t *hdr = (AODV_rrep_pkt_t *)msg->data;
			
			aodv_fix_rrep_endian( hdr );
			DEBUG("[AODV] node %d RECV_RREP: src=%d dest=%d previous_hop=%d hop_count=%d\n",
				sys_id(), hdr->source_addr, hdr->dest_addr, msg->saddr, hdr->hop_count);
			
			update_seq_no(s, hdr->source_addr, hdr->dest_seq_no);
			
			if(hdr->dest_addr == sys_id())
			{
				del_pending_rreq(s, hdr->source_addr);
				update_route(s, hdr, msg->saddr);
				
				while( get_from_buffer(s, hdr->source_addr, &data_pkt) != NOT_FOUND) // AODV2
				{
					s->seq_no++;
					data_pkt->hdr.seq_no = s->seq_no;
	
					DEBUG("[AODV] node %d TRANSMITTING packet %x: dest=%d next_hop=%d\n", sys_id(), (int)data_pkt, data_pkt->hdr.dest_addr, msg->saddr);

					DEBUG("[AODV] Unbuffered packet from %d to %d with length %d: %i %i %i %i %i!\n",
						data_pkt->hdr.source_addr,
						data_pkt->hdr.dest_addr,
						data_pkt->hdr.length,
						data_pkt->data[0], 
						data_pkt->data[1],
						data_pkt->data[2],
						data_pkt->data[3],
						data_pkt->data[4]); 
			
					aodv_fix_hdr_endian(&(data_pkt->hdr));
					sys_post_net(AODV_PID, MSG_AODV_RECV_DATA,
						sizeof(AODV_hdr_t) + data_pkt->hdr.length, data_pkt, 
						SOS_MSG_RELEASE | SOS_MSG_LINK_AUTO | SOS_MSG_RAW, msg->saddr);		
				}
				return SOS_OK;
			}
			else
			{
				void* d;
				uint8_t l;
				switch(check_route(s, hdr->dest_addr, hdr->dest_seq_no, hdr->hop_count))
				{
					case NOT_FOUND:  //entry not in cache
					case FOUND_OLDER:  //RREP seq_no > cache seq_no
					case FOUND_LONGER:  //(RREP seq_no = cache seq_no) + (RREP hopcount < cache hopcount)
						l = msg->len;
						d = sys_msg_take_data(msg);
					
						update_route(s, hdr, msg->saddr);
						sys_post(AODV_PID, MSG_AODV_FWD_RREP,
							l, d, SOS_MSG_RELEASE);					
						break;
				}	
				return SOS_OK;
			}
			
			return SOS_OK;
		}

		case MSG_AODV_FWD_RREP:
		{
			AODV_rrep_pkt_t *hdr = (AODV_rrep_pkt_t *)msg->data;
			uint16_t next_hop;
			
			DEBUG("[AODV] node %d FWD_RREP: src=%d dest=%d next_hop=%d hop_count=%d\n",
				sys_id(), hdr->source_addr, hdr->dest_addr, get_reverse_address(s, hdr), hdr->hop_count);

			if((next_hop = get_reverse_address(s, hdr)) != INVALID_NODE_ID)
			{
				void* d;
				uint8_t l;
				
				l = msg->len;
				d = sys_msg_take_data(msg);
					
				hdr->hop_count++;
				remove_cache_entry(s, hdr->source_addr, hdr->dest_addr);
				aodv_fix_rrep_endian( hdr );
				sys_post_net(AODV_PID, MSG_AODV_RECV_RREP,
					l, d, SOS_MSG_RELEASE | SOS_MSG_LINK_AUTO | SOS_MSG_RAW, next_hop);			
					
			}
			return SOS_OK;
		}

		case MSG_AODV_RECV_DATA:
		{
			AODV_pkt_t *data_pkt = (AODV_pkt_t *)msg->data;
			//cmn_packet_t *cmn_pkt;
			
			aodv_fix_hdr_endian( &( data_pkt->hdr ) );
			DEBUG("[AODV] node %d RECV_DATA: src=%d dest=%d previous_hop=%d seq_no=%d\n",
				sys_id(), data_pkt->hdr.source_addr, data_pkt->hdr.dest_addr, msg->saddr, data_pkt->hdr.seq_no);

/*				DEBUG("[AODV] Received packet from %d to %d with length %d: %i %i %i %i %i!\n",
					data_pkt->hdr.source_addr,
					data_pkt->hdr.dest_addr,
					data_pkt->hdr.length,
					data_pkt->data[0], 
					data_pkt->data[1],
					data_pkt->data[2],
					data_pkt->data[3],
					data_pkt->data[4]); */
			
			update_seq_no(s, data_pkt->hdr.source_addr, data_pkt->hdr.seq_no);
			

			//If I am the destination node, then send to application
			if(data_pkt->hdr.dest_addr == sys_id())
			{
				uint8_t *user_data;
				//sys_led(LED_GREEN_TOGGLE);
				
				user_data = sys_malloc(data_pkt->hdr.length);
				if( user_data || data_pkt->hdr.length == 0) {
					memcpy(user_data, data_pkt->data, data_pkt->hdr.length);
				
					post_longer(data_pkt->hdr.dst_pid, data_pkt->hdr.src_pid, data_pkt->hdr.msg_type,
						data_pkt->hdr.length, user_data, SOS_MSG_RELEASE, data_pkt->hdr.source_addr);
				}
				return SOS_OK;
			} else {
				//
				// Check for loop
				//
				uint16_t next_hop;
				next_hop = get_next_hop(s, data_pkt->hdr.dest_addr);
				DEBUG("[AODV] next hop is %d\n", next_hop);
				if( next_hop != msg->saddr ) { 	
					uint8_t l = msg->len;
					void* d   = sys_msg_take_data(msg);
					//forward to destination
					sys_post(AODV_PID, MSG_AODV_FWD_DATA,
							l, d, SOS_MSG_RELEASE);
				} else {
					DEBUG("Drop packet to prevent loop!\n");
				}
				return SOS_OK;
			}
		}

		case MSG_AODV_FWD_DATA:
		{
			uint8_t l = msg->len;
			AODV_pkt_t *data_pkt = (AODV_pkt_t *)sys_msg_take_data(msg);
			uint16_t next_hop;
			
			//if destination is a neighbor, then send directly
			if(check_neighbors(s, data_pkt->hdr.dest_addr) == FOUND)
			{
				uint16_t dest_addr = data_pkt->hdr.dest_addr;
				DEBUG("[AODV] node %d FWD_DATA: src:%d dest:%d next_hop=%d seq_no=%d\n",
					sys_id(), data_pkt->hdr.source_addr, data_pkt->hdr.dest_addr, data_pkt->hdr.dest_addr,
					data_pkt->hdr.seq_no);
				
				aodv_fix_hdr_endian( &(data_pkt->hdr) );
				sys_post_net(AODV_PID, MSG_AODV_RECV_DATA,
					l, data_pkt, SOS_MSG_RELEASE | SOS_MSG_LINK_AUTO | SOS_MSG_RAW, dest_addr);		
				return SOS_OK;	
			}
			else 
			{
				//forward to next hop
				next_hop = get_next_hop(s, data_pkt->hdr.dest_addr);
				if(next_hop != INVALID_NODE_ID)
				{
					use_route(s, data_pkt->hdr.dest_addr);
					//sys_led(LED_YELLOW_TOGGLE);
					DEBUG("[AODV] node %d FWD_DATA: src:%d dest:%d next_hop=%d seq_no=%d\n",
						sys_id(), data_pkt->hdr.source_addr, data_pkt->hdr.dest_addr, next_hop,
						data_pkt->hdr.seq_no);
					aodv_fix_hdr_endian( &(data_pkt->hdr) );
					sys_post_net(AODV_PID, MSG_AODV_RECV_DATA,
						l, data_pkt, SOS_MSG_RELEASE | SOS_MSG_LINK_AUTO | SOS_MSG_RAW, next_hop);			
					return SOS_OK;
				} else {
					DEBUG("[AODV] node %d: buffering packet to node %d\n", sys_id(), data_pkt->hdr.dest_addr);                    
					add_to_buffer(s, data_pkt);
					
					if( check_pending_rreq(s, data_pkt->hdr.dest_addr) == NOT_FOUND) {
						if(add_pending_rreq(s, data_pkt->hdr.dest_addr) == SUCCESS) {
							sys_post_value(AODV_PID, MSG_AODV_SEND_RREQ, 
									data_pkt->hdr.dest_addr, 0);                        
						}

					}
					//DEBUG("[AODV] node %d FWD_DATA unable to find next hop\n", sys_id());
					return SOS_OK;
				}
			}				
				
			return SOS_OK;
		}				
		
		case MSG_AODV_SEND_RERR:
		{
			MsgParam *p  = (MsgParam *)msg->data;
			AODV_rerr_pkt_t *hdr;
			uint16_t dest_addr;

			DEBUG("received send_rerr for node=%d\n", p->word);

			while((dest_addr = get_dest_addr(s, p->word)) != 0)
			{
				DEBUG("[AODV] node %d SEND_RERR: dest_addr=%d seq_no=%d\n",
					sys_id(), dest_addr, get_seq_no(s, dest_addr));	
		
				hdr = (AODV_rerr_pkt_t *)sys_malloc(sizeof(AODV_rerr_pkt_t));
				
				hdr->addr = dest_addr;
				hdr->seq_no = get_seq_no(s, dest_addr);
			
				aodv_fix_rerr_endian( hdr );
				sys_post_net(AODV_PID, MSG_AODV_RECV_RERR,
					sizeof(AODV_rerr_pkt_t), hdr, SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO | SOS_MSG_RAW, BCAST_ADDRESS);			

			} 

			return SOS_OK;
		}

		case MSG_AODV_RECV_RERR:
		{
			AODV_rerr_pkt_t *hdr = (AODV_rerr_pkt_t *)msg->data;
			
			aodv_fix_rerr_endian( hdr );
			DEBUG("[AODV] node %d RECV_RRER: addr=%d seq_no=%d\n",
				sys_id(), hdr->addr, hdr->seq_no);			

			if(check_dest_addr(s, hdr) == FOUND)
			{
				void* d = sys_msg_take_data(msg);
				aodv_fix_rerr_endian( hdr );
				sys_post_net(AODV_PID, MSG_AODV_RECV_RERR,
						sizeof(AODV_rerr_pkt_t), d, SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO | SOS_MSG_RAW, BCAST_ADDRESS);			
			} 
			
			return SOS_OK;
			
		}		
	
		case MSG_TIMER_TIMEOUT:
		{
			remove_inactive_routes(s); //check for inactive routes
			remove_inactive_cache_entries(s); //check for inactive cache entries
			
			remove_expired_buffer_entries(s);
						
			// Check my own leak
			if( malloc_gc_module(sys_pid() ) ) {
				led_red_toggle();
				led_red_toggle();
				led_red_toggle();
				led_red_toggle();
				led_red_toggle();
			}
			return SOS_OK;
		}
		
		case MSG_FINAL:
		{
			sys_timer_stop(AODV_TIMER);
			return SOS_OK;
		}

		default: return -EINVAL;
	}
	return SOS_OK;
}

static int8_t routing_msg_alloc(func_cb_ptr p, Message *msg)
{
	AODV_state_t *s = sys_get_state();
	
	AODV_pkt_t *data_pkt;
	uint16_t next_hop;
	uint8_t ret;
	
	DEBUG("[AODV] node %d SEND_DATA: dest=%d length=%d\n",
		  sys_id(), msg->daddr, msg->len);
	ret = check_neighbors(s, msg->daddr);
	/*
	if( ret == NO_NEIGHBOR ) {
		return -EAGAIN;
	}
	*/
	if( ret == FOUND ) {
		// If we found the neighbor, send directly to the node
		msg->flag |= (SOS_MSG_LINK_AUTO | SOS_MSG_RAW);
		if( post(msg) == SOS_OK ) {
			// Senddone is handled in the kernel
			return (SOS_OK + 1);
		}
	}	
	data_pkt = (AODV_pkt_t *)sys_malloc(sizeof(AODV_hdr_t) +msg->len);
	
	data_pkt->hdr.source_addr = sys_id();
	data_pkt->hdr.dest_addr = msg->daddr;
	data_pkt->hdr.dst_pid = msg->did;
	data_pkt->hdr.src_pid = msg->sid;
	data_pkt->hdr.msg_type = msg->type;
	data_pkt->hdr.length = msg->len;
	
	memcpy(data_pkt->data, msg->data, msg->len);
	DEBUG("[AODV] Tx packet %x to %d: %i %i %i %i %i!\n",
		  (int)data_pkt,
		  data_pkt->hdr.dest_addr,
		  data_pkt->data[0], 
		  data_pkt->data[1],
		  data_pkt->data[2],
		  data_pkt->data[3],
		  data_pkt->data[4]); 
	
	//if the route is known, then forward to next hop
	if((next_hop = get_next_hop(s, data_pkt->hdr.dest_addr)) != INVALID_NODE_ID)
	{
		DEBUG("[AODV] node %d: forwarding packet to node %d\n", sys_id(), next_hop);
		s->seq_no++;
		data_pkt->hdr.seq_no = s->seq_no;
		//sys_led(LED_RED_TOGGLE);	
		use_route(s, data_pkt->hdr.dest_addr);			
		aodv_fix_hdr_endian( &(data_pkt->hdr) );
		sys_post_net(AODV_PID, MSG_AODV_RECV_DATA,
					 sizeof(AODV_hdr_t) + data_pkt->hdr.length, data_pkt, 
					 SOS_MSG_RELEASE | SOS_MSG_LINK_AUTO | SOS_MSG_RAW, next_hop);
	} else {
		//route is unknown
		DEBUG("[AODV] node %d: buffering packet to node %d\n", sys_id(), msg->daddr);
		add_to_buffer(s, data_pkt);
		
		//if rreq for the node hasn't been sent, then send it
		
		if(check_pending_rreq(s, data_pkt->hdr.dest_addr) == NOT_FOUND)
		{
			if(add_pending_rreq(s, data_pkt->hdr.dest_addr) == SUCCESS) {
				sys_post_value(AODV_PID, MSG_AODV_SEND_RREQ,
							   data_pkt->hdr.dest_addr, 0);
			}
		}
	}
	return SOS_OK;
}

static uint8_t check_cache(AODV_state_t *s, AODV_rreq_pkt_t *hdr)
{
	AODV_cache_entry_t * AODV_cache_ptr = s->AODV_cache_ptr;
	
	if (AODV_cache_ptr == NULL)
		return NOT_FOUND;
		
	while(1)
	{
		if((AODV_cache_ptr->source_addr == hdr->source_addr)
		&& (AODV_cache_ptr->dest_addr == hdr->dest_addr)
		&& (AODV_cache_ptr->broadcast_id >= hdr->broadcast_id))
			return FOUND;

		if (AODV_cache_ptr->next != NULL)
			AODV_cache_ptr = AODV_cache_ptr->next;
		else
			break;
	}
	
	return NOT_FOUND;
}

static void update_cache(AODV_state_t *s, AODV_rreq_pkt_t *hdr, uint16_t saddr)
{
	AODV_cache_entry_t *AODV_cache_ptr = s->AODV_cache_ptr;
	AODV_cache_entry_t *AODV_tmp_cache_ptr;

	if(s->max_cache_entries != UNLIMITED_ENTRIES)
	{
		if(s->num_of_cache_entries == s->max_cache_entries)
		{
			DEBUG("[AODV] node %d: Cache list is full. Cannot add entry\n", sys_id());
			return;
		}
	}
	
	if (AODV_cache_ptr != NULL)
	{
		while(1)
		{
			if((AODV_cache_ptr->dest_addr == hdr->dest_addr) && (AODV_cache_ptr->dest_addr == hdr->dest_addr))
			{
				DEBUG("[AODV] node %d updating cache entry: src=%d dest=%d bcast_id=%d hop_count=%d next_hop=%d\n",
					sys_id(), hdr->source_addr, hdr->dest_addr, hdr->broadcast_id, hdr->hop_count, saddr);

				AODV_cache_ptr->broadcast_id = hdr->broadcast_id;
				AODV_cache_ptr->lifetime = REVERSE_ROUTE_LIFE;
				AODV_cache_ptr->next_hop = saddr;
				AODV_cache_ptr->source_seq_no = hdr->source_seq_no;
  			AODV_cache_ptr->hop_count = hdr->hop_count;
					
				return;
			}
	
			if (AODV_cache_ptr->next != NULL)
				AODV_cache_ptr = AODV_cache_ptr->next;
			else
				break;
		}
	}

	DEBUG("[AODV] node %d adding cache entry: src=%d source_seq_no=%d dest=%d bcast_id=%d hop_count=%d next_hop=%d\n",
		sys_id(), hdr->source_addr, hdr->source_seq_no, hdr->dest_addr, hdr->broadcast_id, hdr->hop_count, saddr);

	
	AODV_tmp_cache_ptr = (AODV_cache_entry_t *)sys_malloc(sizeof(AODV_cache_entry_t));		
	AODV_tmp_cache_ptr->dest_addr = hdr->dest_addr;
	AODV_tmp_cache_ptr->source_addr = hdr->source_addr;
	AODV_tmp_cache_ptr->broadcast_id = hdr->broadcast_id;
	AODV_tmp_cache_ptr->lifetime = REVERSE_ROUTE_LIFE;
	AODV_tmp_cache_ptr->next_hop = saddr;
	AODV_tmp_cache_ptr->source_seq_no = hdr->source_seq_no;
	AODV_tmp_cache_ptr->hop_count = hdr->hop_count;

	//the entry will always be placed at the head of the list	
	if(s->AODV_cache_ptr == NULL)
	{
		s->AODV_cache_ptr = AODV_tmp_cache_ptr;
		AODV_tmp_cache_ptr->next = NULL;
	}
	else
	{
		AODV_tmp_cache_ptr->next = s->AODV_cache_ptr->next;
		s->AODV_cache_ptr->next = AODV_tmp_cache_ptr;
	}
	
	s->num_of_cache_entries++;
}

static void remove_cache_entry(AODV_state_t *s, uint16_t source_addr, uint16_t dest_addr)
{
	/* Select node at head of list */
	AODV_cache_entry_t * AODV_cache_ptr = s->AODV_cache_ptr;
	AODV_cache_entry_t * AODV_previous_ptr = s->AODV_cache_ptr;

	/* Loop until we've reached the end of the list */
	while( AODV_cache_ptr != NULL )
	{
		if((AODV_cache_ptr->source_addr == dest_addr)
			&& (AODV_cache_ptr->dest_addr == source_addr))
		{
			// Found the item to be deleted,
     	// re-link the list around it 
			if( AODV_cache_ptr == s->AODV_cache_ptr )
				/* We're deleting the head */
				s->AODV_cache_ptr = AODV_cache_ptr->next;
			else
				AODV_previous_ptr->next = AODV_cache_ptr->next;

			DEBUG("[AODV] node %d deleting cache entry: src=%d dest=%d\n",
				sys_id(), AODV_cache_ptr->source_addr, AODV_cache_ptr->dest_addr);
					
			/* Free the node */
			sys_free( AODV_cache_ptr );
			
			s->num_of_cache_entries--;
			break;

		}
		AODV_previous_ptr = AODV_cache_ptr;
		AODV_cache_ptr = AODV_cache_ptr->next;
	}

	return;			
		
}

static uint8_t check_route(AODV_state_t *s, uint16_t dest_addr, uint16_t dest_seq_no, uint8_t hop_count)
{
	AODV_route_entry_t *AODV_route_ptr = s->AODV_route_ptr;
	
	if (AODV_route_ptr == NULL)
		return NOT_FOUND;
		
	while(1)
	{

		if(AODV_route_ptr->dest_addr == dest_addr)
		{
			if(dest_seq_no > AODV_route_ptr->dest_seq_no)
			{
				//update seq_no
				AODV_route_ptr->dest_seq_no = dest_seq_no;
				return FOUND_OLDER;
			}
			
			if((dest_seq_no == AODV_route_ptr->dest_seq_no)
				&&(hop_count < AODV_route_ptr->hop_count))
				return FOUND_LONGER;

			return FOUND_BETTER;			
		}

		if (AODV_route_ptr->next != NULL)
			AODV_route_ptr = AODV_route_ptr->next;
		else
			break;
	}
	
	return NOT_FOUND;
}	

static void use_route(AODV_state_t *s, uint16_t dest_addr)
{
	AODV_route_entry_t *AODV_route_ptr = s->AODV_route_ptr;
	
	if (AODV_route_ptr != NULL)
	{
		while(1)
		{
			if(AODV_route_ptr->dest_addr == dest_addr)
			{
				DEBUG("[AODV] node %d using route entry : dest=%d\n",
					sys_id(), dest_addr);
 
				AODV_route_ptr->lifetime = ROUTE_EXPIRATION_TIMEOUT;
					
				return;
			}
	
			if (AODV_route_ptr->next != NULL)
				AODV_route_ptr = AODV_route_ptr->next;
			else
				break;
		}
	}
	
	return;
}
		
static void update_route(AODV_state_t *s, AODV_rrep_pkt_t *hdr, uint16_t saddr)
{
	AODV_route_entry_t *AODV_route_ptr = s->AODV_route_ptr;
	AODV_route_entry_t *AODV_tmp_route_ptr;
	
	if(s->max_route_entries != UNLIMITED_ENTRIES)
	{
		if(s->num_of_route_entries == s->max_route_entries)
		{
			DEBUG("[AODV] node %d: Route list is full. Cannot add entry\n", sys_id());
			return;
		}
	}

	if (AODV_route_ptr != NULL)
	{
		while(1)
		{
			if(AODV_route_ptr->dest_addr == hdr->source_addr)
			{
				DEBUG("[AODV] node %d updating route entry: src=%d dest=%d dest_seq_no=%d hop_count=%d next_hop=%d\n",
					sys_id(), hdr->dest_addr, hdr->source_addr, hdr->dest_seq_no, hdr->hop_count, saddr);
 
				AODV_route_ptr->next_hop = saddr;
  				AODV_route_ptr->hop_count = hdr->hop_count;
				AODV_route_ptr->dest_seq_no = hdr->dest_seq_no;
				AODV_route_ptr->lifetime = ROUTE_EXPIRATION_TIMEOUT;
					
				return;
			}
	
			if (AODV_route_ptr->next != NULL)
				AODV_route_ptr = AODV_route_ptr->next;
			else
				break;
		}
	}

	DEBUG("[AODV] node %d adding route entry: src=%d dest=%d dest_seq_no=%d hop_count=%d next_hop=%d\n",
		sys_id(), hdr->source_addr, hdr->dest_addr, hdr->dest_seq_no, hdr->hop_count, saddr);

	AODV_tmp_route_ptr = (AODV_route_entry_t *)sys_malloc(sizeof(AODV_route_entry_t));
	AODV_tmp_route_ptr->dest_addr = hdr->source_addr;
	AODV_tmp_route_ptr->next_hop = saddr;
	AODV_tmp_route_ptr->hop_count = hdr->hop_count;
	AODV_tmp_route_ptr->dest_seq_no = hdr->dest_seq_no;
	AODV_tmp_route_ptr->lifetime = ROUTE_EXPIRATION_TIMEOUT;

	//the entry will always be placed at the head of the list
	if(s->AODV_route_ptr == NULL)
	{
		s->AODV_route_ptr = AODV_tmp_route_ptr;
		AODV_tmp_route_ptr->next = NULL;
	}
	else
	{
		AODV_tmp_route_ptr->next = s->AODV_route_ptr->next;
		s->AODV_route_ptr->next = AODV_tmp_route_ptr;
	}
	
	s->num_of_route_entries++;
}

static uint16_t get_reverse_address(AODV_state_t *s, AODV_rrep_pkt_t *hdr)
{
	AODV_cache_entry_t * AODV_cache_ptr = s->AODV_cache_ptr;
	
	
	if (AODV_cache_ptr == NULL)
		return INVALID_NODE_ID; // this should never happen
	
	while(1)
	{
		if((AODV_cache_ptr->source_addr == hdr->dest_addr)
		&& (AODV_cache_ptr->dest_addr == hdr->source_addr))
			return AODV_cache_ptr->next_hop;

		if (AODV_cache_ptr->next != NULL)
			AODV_cache_ptr = AODV_cache_ptr->next;
		else
			break;
	}
	
	return INVALID_NODE_ID; // this should never happen
}

static uint16_t get_next_hop(AODV_state_t *s, uint16_t dest_addr)
{
	AODV_route_entry_t * AODV_route_ptr = s->AODV_route_ptr;
	
	
	if (AODV_route_ptr == NULL)
		return INVALID_NODE_ID; // this should never happen

#ifdef AODV_DEBUG
	while(AODV_route_ptr != NULL) {
		DEBUG("[AODV] node %d next hop %d\n", AODV_route_ptr->dest_addr,
				AODV_route_ptr->next_hop);
		AODV_route_ptr = AODV_route_ptr->next;
	}
	AODV_route_ptr = s->AODV_route_ptr;
#endif
		
	while(1)
	{
		if(AODV_route_ptr->dest_addr == dest_addr)
			return AODV_route_ptr->next_hop;

		if (AODV_route_ptr->next != NULL)
			AODV_route_ptr = AODV_route_ptr->next;
		else
			break;
	}
	
	return INVALID_NODE_ID; 
}

static void remove_inactive_routes(AODV_state_t *s)
{
	/* Select node at head of list */
	AODV_route_entry_t * AODV_route_ptr = s->AODV_route_ptr;
	AODV_route_entry_t * AODV_delete_ptr;
	AODV_route_entry_t * AODV_previous_ptr = s->AODV_route_ptr;

	/* Loop until we've reached the end of the list */
	while( AODV_route_ptr != NULL )
	{
		AODV_route_ptr->lifetime--;
		if(AODV_route_ptr->lifetime == 0)
		{
			// Found the item to be deleted,
     	// re-link the list around it 
			if( AODV_route_ptr == s->AODV_route_ptr )
				/* We're deleting the head */
				s->AODV_route_ptr = AODV_route_ptr->next;
			else
				AODV_previous_ptr->next = AODV_route_ptr->next;

			DEBUG("[AODV] node %d deleting route entry: dest=%d dest_seq_no=%d hop_count=%d next_hop=%d\n",
			 	sys_id(), AODV_route_ptr->dest_addr, AODV_route_ptr->dest_seq_no, AODV_route_ptr->hop_count, AODV_route_ptr->next_hop);

			AODV_delete_ptr = AODV_route_ptr;
			AODV_route_ptr = AODV_route_ptr->next;
		
			/* Free the node */
			sys_free( AODV_delete_ptr );
			
			s->num_of_route_entries--;
			
			continue;
		}
		AODV_previous_ptr = AODV_route_ptr;
		AODV_route_ptr = AODV_route_ptr->next;
	}

	return;		
		
}

static void remove_inactive_cache_entries(AODV_state_t *s)
{
	/* Select node at head of list */
	AODV_cache_entry_t * AODV_cache_ptr = s->AODV_cache_ptr;
	AODV_cache_entry_t * AODV_delete_ptr;
	AODV_cache_entry_t * AODV_previous_ptr = s->AODV_cache_ptr;

	/* Loop until we've reached the end of the list */
	while( AODV_cache_ptr != NULL )
	{
		AODV_cache_ptr->lifetime--;
		if(AODV_cache_ptr->lifetime == 0)
		{
			// Found the item to be deleted,
     	// re-link the list around it 
			if( AODV_cache_ptr == s->AODV_cache_ptr )
				/* We're deleting the head */
				s->AODV_cache_ptr = AODV_cache_ptr->next;
			else
				AODV_previous_ptr->next = AODV_cache_ptr->next;

			DEBUG("[AODV] node %d deleting cache entry: src=%d source_seq_no=%d dest=%d  dest_seq_no=%d hop_count=%d next_hop=%d\n",
				sys_id(), AODV_cache_ptr->source_addr, AODV_cache_ptr->source_seq_no, AODV_cache_ptr->dest_addr, AODV_cache_ptr->source_seq_no, AODV_cache_ptr->hop_count, AODV_cache_ptr->next_hop);

			AODV_delete_ptr = AODV_cache_ptr;
			AODV_cache_ptr = AODV_cache_ptr->next;
					
			/* Free the node */
			sys_free( AODV_delete_ptr );
			
			s->num_of_cache_entries--;
			
			continue;
		}
		AODV_previous_ptr = AODV_cache_ptr;
		AODV_cache_ptr = AODV_cache_ptr->next;
	}

	return;			
		
}


static uint8_t check_dest_addr(AODV_state_t *s, AODV_rerr_pkt_t *hdr)
{
	/* Select node at head of list */
	AODV_route_entry_t * AODV_route_ptr = s->AODV_route_ptr;
	AODV_route_entry_t * AODV_delete_ptr;
	AODV_route_entry_t * AODV_previous_route_ptr = s->AODV_route_ptr;

	uint8_t return_value = NOT_FOUND;
	/* Loop until we've reached the end of the list */
	
	while( AODV_route_ptr != NULL )
	{
		if((AODV_route_ptr->dest_addr == hdr->addr)
		&& (get_seq_no(s, hdr->addr) <= hdr->seq_no))
		{
			return_value = FOUND;
			// Found the item to be deleted,
     	// re-link the list around it 
			if( AODV_route_ptr == s->AODV_route_ptr )
				/* We're deleting the head */
				s->AODV_route_ptr = AODV_route_ptr->next;
			else
				AODV_previous_route_ptr->next = AODV_route_ptr->next;

			DEBUG("[AODV] node %d deleting route entry after RERR: dest=%d dest_seq_no=%d hop_count=%d next_hop=%d\n",
			 	sys_id(), AODV_route_ptr->dest_addr, AODV_route_ptr->dest_seq_no, AODV_route_ptr->hop_count, AODV_route_ptr->next_hop);

			AODV_delete_ptr = AODV_route_ptr;
			AODV_route_ptr = AODV_route_ptr->next;

			/* Free the node */
			sys_free( AODV_delete_ptr );
			
			s->num_of_route_entries--;
			
			continue;
		}
		AODV_previous_route_ptr = AODV_route_ptr;
		AODV_route_ptr = AODV_route_ptr->next;
	}	

	return return_value;	

}

static uint16_t get_dest_addr(AODV_state_t *s, uint16_t next_hop)
{
	/* Select node at head of list */
	AODV_route_entry_t * AODV_route_ptr = s->AODV_route_ptr;
	AODV_route_entry_t * AODV_previous_route_ptr = s->AODV_route_ptr;

	uint16_t temp_addr = 0;

	/* Loop until we've reached the end of the list */
	while( AODV_route_ptr != NULL )
	{
		if(AODV_route_ptr->next_hop == next_hop)
		{
			// Found the item to be deleted,
     	// re-link the list around it 
			if( AODV_route_ptr == s->AODV_route_ptr )
				/* We're deleting the head */
				s->AODV_route_ptr = AODV_route_ptr->next;
			else
				AODV_previous_route_ptr->next = AODV_route_ptr->next;

			DEBUG("[AODV] node %d deleting route entry after RERR: dest=%d dest_seq_no=%d hop_count=%d next_hop=%d\n",
			 	sys_id(), AODV_route_ptr->dest_addr, AODV_route_ptr->dest_seq_no, AODV_route_ptr->hop_count, AODV_route_ptr->next_hop);

			temp_addr = AODV_route_ptr->dest_addr;
			/* Free the node */
			sys_free( AODV_route_ptr );
			
			s->num_of_route_entries--;

			return temp_addr;

		}
		AODV_previous_route_ptr = AODV_route_ptr;
		AODV_route_ptr = AODV_route_ptr->next;
	}	

	return 0;
}

static void add_to_buffer(AODV_state_t *s, AODV_pkt_t *pkt)
{
	AODV_buf_pkt_entry_t *AODV_buf_ptr = s->AODV_buf_ptr;
	AODV_buf_pkt_entry_t *AODV_tmp_buf_ptr;

	//DEBUG("[AODV2] add_to_buffer started\n");
	
	if(s->max_buf_packets != UNLIMITED_ENTRIES)
	{
		if(s->num_of_buf_packets == s->max_buf_packets)
		{
			DEBUG("[AODV] node %d: Buffer list is full. Cannot add entry\n", sys_id());
			return;
		}
	}
	
	DEBUG("[AODV] node %d adding to buffer: src=%d dest=%d \n",
		sys_id(), pkt->hdr.source_addr, pkt->hdr.dest_addr);

	AODV_tmp_buf_ptr = (AODV_buf_pkt_entry_t *)sys_malloc(sizeof(AODV_buf_pkt_entry_t));
	AODV_tmp_buf_ptr->buf_packet = pkt;
	AODV_tmp_buf_ptr->lifetime = ROUTE_DISCOVERY_TIMEOUT;
	
	//the entry will always be placed at the tail of the list

	if(s->AODV_buf_ptr != NULL)
	{
		while(AODV_buf_ptr->next != NULL)
		{
			AODV_buf_ptr = AODV_buf_ptr->next;
		}	
		AODV_buf_ptr->next = AODV_tmp_buf_ptr;
		AODV_tmp_buf_ptr->next = NULL;
	}
	else
	{
		s->AODV_buf_ptr = AODV_tmp_buf_ptr;
		AODV_tmp_buf_ptr->next = NULL;
	}	

	s->num_of_buf_packets++;
}


static uint8_t get_from_buffer(AODV_state_t *s, uint16_t dest_addr, AODV_pkt_t ** data_pkt)
{
	//DEBUG("[AODV2] get_from_buffer started\n");
		
	/* Select node at head of list */
	AODV_buf_pkt_entry_t * AODV_buf_ptr = s->AODV_buf_ptr;
	AODV_buf_pkt_entry_t * AODV_previous_ptr = s->AODV_buf_ptr;

	/* Loop until we've reached the end of the list */
	while( AODV_buf_ptr != NULL )
	{
		DEBUG("BUFFER: AODV_dest_addr = %d dest_addr=%d\n" , AODV_buf_ptr->buf_packet->hdr.dest_addr,dest_addr);
		if(AODV_buf_ptr->buf_packet->hdr.dest_addr == dest_addr)
		{
			/* Found the item to be returned, 
     		 re-link the list around it */
			if( AODV_buf_ptr == s->AODV_buf_ptr )
				/* We're returning the head */
				s->AODV_buf_ptr = AODV_buf_ptr->next;
			else
				AODV_previous_ptr->next = AODV_buf_ptr->next;
			
			*data_pkt = AODV_buf_ptr->buf_packet;
			sys_free(AODV_buf_ptr);

			s->num_of_buf_packets--;
			return FOUND;

		}
		AODV_previous_ptr = AODV_buf_ptr;
		AODV_buf_ptr = AODV_buf_ptr->next;
		
	}
	return NOT_FOUND;		 
}

static void remove_expired_buffer_entries(AODV_state_t *s)
{
	//DEBUG("[AODV2] remove_expired_buffer_entries started\n");
	
	/* Select node at head of list */
	AODV_buf_pkt_entry_t * AODV_buf_ptr = s->AODV_buf_ptr;
	AODV_buf_pkt_entry_t * AODV_delete_ptr;
	AODV_buf_pkt_entry_t * AODV_previous_ptr = s->AODV_buf_ptr;


	/* Loop until we've reached the end of the list */
	while( AODV_buf_ptr != NULL )
	{
		AODV_buf_ptr->lifetime--;
		if(AODV_buf_ptr->lifetime == 0)
		{
			/* Found the item to be deleted,
     		 re-link the list around it */
			if( AODV_buf_ptr == s->AODV_buf_ptr )
				/* We're deleting the head */
				s->AODV_buf_ptr = AODV_buf_ptr->next;
			else
				AODV_previous_ptr->next = AODV_buf_ptr->next;
			del_pending_rreq(s, AODV_buf_ptr->buf_packet->hdr.dest_addr);
			//sys_led(LED_GREEN_TOGGLE);
			
			DEBUG("[AODV] WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
			DEBUG("[AODV] node %d deleting buffer entry: dest=%d\n",
				sys_id(), AODV_buf_ptr->buf_packet->hdr.dest_addr);

			AODV_delete_ptr = AODV_buf_ptr;
			AODV_buf_ptr = AODV_buf_ptr->next;
			
			/* Free the node */
			sys_free( AODV_delete_ptr->buf_packet );
			sys_free( AODV_delete_ptr );
			
			s->num_of_buf_packets--;
			continue;
		}
		AODV_previous_ptr = AODV_buf_ptr;
		AODV_buf_ptr = AODV_buf_ptr->next;
	}
	return;			

/*		char buf_conf[4] = { 'v', 'L', 'v', '1' };
        AODV_buf_pkt_entry_t * AODV_previous_ptr = NULL;
        AODV_buf_pkt_entry_t * AODV_buf_ptr = s->AODV_buf_ptr;
      
        while(1)
        {
        
        	if(AODV_buf_ptr == NULL)
                return NULL;

        	while(AODV_buf_ptr != NULL && --AODV_buf_ptr->lifetime != 0)
        	{
                	AODV_previous_ptr = AODV_buf_ptr;
                	AODV_buf_ptr = AODV_buf_ptr->next;
        	}
	
        	if(AODV_buf_ptr == NULL)
                	return NULL;
        	else
        	{
                	if(AODV_previous_ptr == NULL)
                	{
                        	s->AODV_buf_ptr = AODV_buf_ptr->next;
                	}
                	else
                	{
                        	AODV_previous_ptr->next = AODV_buf_ptr->next;
                	}	
        			del_pending_rreq(buf_conf, AODV_buf_ptr->buf_packet->hdr.dest_addr);
			        	free(AODV_previous_ptr);
					DEBUG("[AODV] WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					DEBUG("[AODV] node %d deleting buffer entry: dest=%d\n",
						sys_id(), AODV_buf_ptr->buf_packet->hdr.dest_addr);
        			
        			s->num_of_buf_packets--;

            
        	}
    	}
        return; */

	
} 

static uint8_t add_pending_rreq(AODV_state_t *s, uint16_t addr)
{
	AODV_rreq_entry_t *AODV_rreq_ptr = s->AODV_rreq_ptr;
	AODV_rreq_entry_t *AODV_tmp_rreq_ptr;

	if(s->max_rreq != UNLIMITED_ENTRIES)
	{
		if(s->num_of_rreq == s->max_rreq)
		{
			DEBUG("[AODV] node %d: RREQ list is full. Cannot add entry\n", sys_id());
			return FAIL;
		}
	}
	
	DEBUG("[AODV] node %d adding rreq: dest=%d \n",
		sys_id(), addr);

	AODV_tmp_rreq_ptr = (AODV_rreq_entry_t *)sys_malloc(sizeof(AODV_rreq_entry_t));
	
	AODV_tmp_rreq_ptr->dest_addr = addr;
	
	//the entry will always be placed at the tail of the list

	if(s->AODV_rreq_ptr != NULL)
	{
		while(AODV_rreq_ptr->next != NULL)
		{
			AODV_rreq_ptr = AODV_rreq_ptr->next;
		}	
		AODV_rreq_ptr->next = AODV_tmp_rreq_ptr;
		AODV_tmp_rreq_ptr->next = NULL;
	}
	else
	{
		s->AODV_rreq_ptr = AODV_tmp_rreq_ptr;
		AODV_tmp_rreq_ptr->next = NULL;
	}	

	s->num_of_rreq++;	
	return SUCCESS;
	
}


static void del_pending_rreq(AODV_state_t *s, uint16_t addr)
{
	/* Select node at head of list */
	AODV_rreq_entry_t * AODV_rreq_ptr = s->AODV_rreq_ptr;
	AODV_rreq_entry_t * AODV_previous_ptr = s->AODV_rreq_ptr;

	/* Loop until we've reached the end of the list */
	while( AODV_rreq_ptr != NULL )
	{
		if(AODV_rreq_ptr->dest_addr == addr)
		{
			// Found the item to be deleted,
     	// re-link the list around it 
			if( AODV_rreq_ptr == s->AODV_rreq_ptr )
				/* We're deleting the head */
				s->AODV_rreq_ptr = AODV_rreq_ptr->next;
			else
				AODV_previous_ptr->next = AODV_rreq_ptr->next;

			DEBUG("[AODV] node %d deleting rreq: dest=%d\n",
				sys_id(), AODV_rreq_ptr->dest_addr);
					
			/* Free the node */
			sys_free( AODV_rreq_ptr );
			
			s->num_of_rreq--;
			return;

		}
		AODV_previous_ptr = AODV_rreq_ptr;
		AODV_rreq_ptr = AODV_rreq_ptr->next;
	}

	return;			
}


static uint8_t check_pending_rreq(AODV_state_t *s, uint16_t addr)
{
	AODV_rreq_entry_t * AODV_rreq_ptr = s->AODV_rreq_ptr;
	
	if (AODV_rreq_ptr == NULL)
		return NOT_FOUND;
		
	while(1)
	{
		if(AODV_rreq_ptr->dest_addr == addr)
			return FOUND;

		if (AODV_rreq_ptr->next != NULL)
			AODV_rreq_ptr = AODV_rreq_ptr->next;
		else
			break;
	}
	
	return NOT_FOUND;
}

static uint16_t get_seq_no(AODV_state_t *s, uint16_t addr)
{
	AODV_node_entry_t * AODV_node_ptr = s->AODV_node_ptr;
	
	if (AODV_node_ptr == NULL)
		return 0;
		
	while(1)
	{
		if(AODV_node_ptr->addr == addr)
			return AODV_node_ptr->seq_no;

		if (AODV_node_ptr->next != NULL)
			AODV_node_ptr = AODV_node_ptr->next;
		else
			break;
	}
	
	return 0;	
	
}

static void update_seq_no(AODV_state_t *s, uint16_t addr, uint16_t seq_no)
{
	AODV_node_entry_t *AODV_node_ptr = s->AODV_node_ptr;
	AODV_node_entry_t *AODV_tmp_node_ptr;

	//First we update the seq_no list
	if(s->max_nodes != UNLIMITED_ENTRIES)
	{
		if(s->num_of_nodes == s->max_nodes)
		{
			DEBUG("[AODV] node %d: Node list is full. Cannot add entry\n", sys_id());
			return;
		}
	}
	
	if (AODV_node_ptr != NULL)
	{
		while(1)
		{
			if(AODV_node_ptr->addr == addr)
			{
				if(seq_no > AODV_node_ptr->seq_no)
				{
					DEBUG("[AODV] node %d updating seq_no: addr=%d old_seq_no=%d new_seq_no=%d\n",
						sys_id(), addr, AODV_node_ptr->seq_no, seq_no);

					AODV_node_ptr->seq_no = seq_no;
				}
				return;
			}
	
			if (AODV_node_ptr->next != NULL)
				AODV_node_ptr = AODV_node_ptr->next;
			else
				break;
		}
	}

	DEBUG("[AODV] node %d adding seq_no: addr=%d seq_no=%d\n",
		sys_id(), addr, seq_no);

	//the entry will always be placed at the head of the list	
	AODV_tmp_node_ptr = (AODV_node_entry_t *)sys_malloc(sizeof(AODV_node_entry_t));
	
	AODV_tmp_node_ptr->addr = addr;
	AODV_tmp_node_ptr->seq_no = seq_no;

	if(s->AODV_node_ptr == NULL)
	{
		s->AODV_node_ptr = AODV_tmp_node_ptr;
		AODV_tmp_node_ptr->next = NULL;
	}
	else
	{
		AODV_tmp_node_ptr->next = s->AODV_node_ptr->next;
		s->AODV_node_ptr->next = AODV_tmp_node_ptr;
	}
	
	s->num_of_nodes++;
}

static uint8_t check_neighbors( AODV_state_t *s, uint16_t addr )
{
	nbr_entry_t *nb_list;
	
	nb_list = sys_shm_get( sys_shm_name(NBHOOD_PID, SHM_NBR_LIST) );
	
	if( nb_list == NULL ) {
		return NO_NEIGHBOR;
	}
	while( nb_list != NULL ) {
		//if( nb_list->id == addr && nb_list->receiveEst > 25) {
		if( nb_list->id == addr) {
			return FOUND;
		}
		nb_list = nb_list->next;
	}
	
	return NOT_FOUND;
}

static void aodv_fix_rreq_endian( AODV_rreq_pkt_t *p )
{
	p->source_addr = ehtons(p->source_addr);
	p->source_seq_no = ehtons(p->source_seq_no);
	p->broadcast_id = ehtons(p->broadcast_id);
	p->dest_addr = ehtons(p->dest_addr);
	p->dest_seq_no = ehtons(p->dest_seq_no);
}

static void aodv_fix_rrep_endian( AODV_rrep_pkt_t *p )
{
	p->source_addr = ehtons(p->source_addr);
	p->dest_addr = ehtons(p->dest_addr);
	p->dest_seq_no = ehtons(p->dest_seq_no);
}

static void aodv_fix_rerr_endian( AODV_rerr_pkt_t *p )
{
	p->addr = ehtons(p->addr);
	p->seq_no = ehtons(p->seq_no);
}

static void aodv_fix_hdr_endian( AODV_hdr_t *p )
{
	p->dest_addr = ehtons(p->dest_addr);
	p->source_addr = ehtons(p->source_addr);
}

#ifndef _MODULE_
mod_header_ptr aodv_get_header()
{
  return sos_get_header_address(mod_header);
}
#endif
