/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
#include <hardware.h>
#include <codemem.h>
#include <sos_module_fetcher.h>
#include <sos_timer.h>
#include <message.h>
#include <sos_info.h>
#include <malloc.h>
#include <random.h>
#include <led_dbg.h>
#ifdef SOS_HAS_EXFLASH
#include <exflash.h>
#endif
#ifdef SOS_SIM
#include <sim_interface.h>
#endif
#include <led_dbg.h>

#ifndef SOS_DEBUG_FETCHER
#undef DEBUG
#define DEBUG(...)
#endif

/**
 * @brief A reliable transport service for modules
 *
 * The protocol will fetch data from one hop communication (i.e. no routing)
 * base on given module version, ID, and size of module.
 * When the protocol fetch is complete, it will inform the requester
 *
 * We do not queue any request at this moment.  The reason is that
 * the probability of getting more than one request is rather low.
 * This means that the components that use fetcher will have to handle the
 * failure
 */
static sos_module_t fetcher_module;

static inline void handle_overheard_fragment(Message *msg);
static int8_t handle_request(Message *msg);
static int8_t handle_data(Message *msg);
static void free_send_state_map(void);
static int8_t fetcher_handler(void *state, Message *msg);
static inline void handle_request_timeout(void);
static inline void send_fragment(void);
static inline void restart_request_timer(void);
static void check_map_and_post(void);
static void start_new_fetch(void);
static void send_fetcher_done(void);
static bool check_map(fetcher_bitmap_t *m);

#ifndef SOS_DEBUG_FETCHER
#define print_bitmap(m)
#else
static void print_bitmap(fetcher_bitmap_t *m);
#endif

static const mod_header_t mod_header SOS_MODULE_HEADER ={
  .mod_id = KER_FETCHER_PID,
  .state_size = 0,
  .num_timers = 1,
  .num_sub_func = 0,
  .num_prov_func = 0,
  .module_handler = fetcher_handler,
};

//! the status of sending fragment
enum {
    FETCHER_SENDING_FRAGMENT_INTERVAL  = 512L,
};

typedef struct {
    uint16_t dest;
    fetcher_fragment_t *fragr;  //!< the code fragment that will be stored
    fetcher_fragment_t *frag;  //!< the code fragment that will be sent
    fetcher_bitmap_t   *map;
    sos_timer_t        timer;
	uint8_t            num_funcs;
} fetcher_sending_state_t;

/**
 * @brief state for sending fragments
 * The reason we merge sending and receiving is because
 * we can send the fragment we have just received.
 */
static fetcher_sending_state_t send_state;

/**
 * @brief state for fetching fragments
 */
static fetcher_state_t *fst = NULL;

/**
 * @brief variable used for resending MSG_FETCHER_DONE
 * We use FETCHER_REQUEST_TID as retry timer
 */
static bool no_mem_retry = false;

int8_t fetcher_request(sos_pid_t req_id, sos_cam_t key, uint16_t size, uint16_t src)
{
    uint8_t bitmap_size;   //! size of the bitmap in bytes
    uint16_t num_fragments;
    uint8_t i;
    fetcher_state_t *f;
	fetcher_cam_t *cam;
	
	cam = (fetcher_cam_t *) ker_cam_lookup(key);
	if( cam == NULL ) return -EINVAL;

    //if(fst != NULL) return -EBUSY;
    DEBUG_PID(KER_FETCHER_PID, "fetcher_request, req_id = %d, size = %d, src = %d\n", req_id, size, src);

    num_fragments = ((size + (FETCHER_FRAGMENT_SIZE - 1))/ FETCHER_FRAGMENT_SIZE);
    bitmap_size = (uint8_t)((num_fragments + 7)/ 8);

    //DEBUG("size = %d\n", sizeof(fetcher_state_t) + bitmap_size);
    f = ker_malloc(sizeof(fetcher_state_t) + bitmap_size, KER_FETCHER_PID);
    if(f == NULL) {
        return -ENOMEM;
    }
    //DEBUG("num_fragments = %d, bitmap_zie = %d\n", num_fragments, bitmap_size);
    f->requester       = req_id;
    f->map.key         = key;
    f->map.bitmap_size = bitmap_size;
    f->src_addr        = src;
	f->num_funcs       = 0;
    f->next            = NULL;
	f->cm              = cam->cm;

    for(i = 0; i < bitmap_size; i++) {
        f->map.bitmap[i] = 0xff;
    }
    if((num_fragments) % 8) {
        f->map.bitmap[bitmap_size - 1] =
            (1 << (num_fragments % 8)) - 1;
    }
    print_bitmap(&f->map);

    //! backoff first!!!
    f->retx = 0;
    if(fst != NULL) {
        fetcher_state_t *tmp = fst;
		cam->status = FETCHING_QUEUED;

        while(tmp->next != NULL) { tmp = tmp->next; }
        tmp->next = f;
        return SOS_OK;
    }
	cam->status = FETCHING_STARTED;
    fst = f;
    //! setup timer
    ker_timer_start(KER_FETCHER_PID,
            FETCHER_REQUEST_TID,
            FETCHER_REQUEST_BACKOFF_SLOT * ((ker_rand() % FETCHER_REQUEST_MAX_SLOT) + 1));
    //DEBUG("request ret = %d\n", ret);
    return SOS_OK;
}

int8_t fetcher_cancel(sos_pid_t req_id, sos_cam_t key)
{
    fetcher_state_t *tmp;
    fetcher_state_t *prev;
    if( fst == NULL ) return -EINVAL;
    if( fst->map.key == key ) {	
        tmp = fst;
        start_new_fetch();
        ker_free( tmp );
		/*
		 * Cancel sender as well
		 */
		if( (send_state.map != NULL) && (send_state.map->key == key)) {
			free_send_state_map();
		}
        return SOS_OK;
    }
    prev = fst;
    tmp = fst->next;
    while(tmp != NULL) {
        if( tmp->map.key == key ) {	
            prev->next = tmp->next;
            ker_free( tmp );
            return SOS_OK;	
        }
        prev = tmp;
        tmp = tmp->next;
    }
    return -EINVAL;
}

void fetcher_restart(fetcher_state_t *s, uint16_t src)
{
	fetcher_cam_t *cam;
	cam = ker_cam_lookup( s->map.key );
    s->src_addr = src;
    s->retx = 0;
	s->next = NULL;
    if(fst != NULL) {
        fetcher_state_t *tmp = fst;
		cam->status = FETCHING_QUEUED;

        while(tmp->next != NULL) { tmp = tmp->next; }
        tmp->next = s;
        return;
    }
    fst = s;
	cam->status = FETCHING_STARTED;
    ker_timer_start(KER_FETCHER_PID,
            FETCHER_REQUEST_TID,
            FETCHER_REQUEST_BACKOFF_SLOT * ((ker_rand() % FETCHER_REQUEST_MAX_SLOT) + 1));
}

void fetcher_commit(fetcher_state_t *s, bool commit)
{
	fetcher_cam_t *cam;
	cam = (fetcher_cam_t *) ker_cam_lookup(s->map.key);
	if( cam == NULL ) return;
	if( commit == true ) {
		ker_codemem_flush( cam->cm, KER_FETCHER_PID );
	} else {
		ker_codemem_free( cam->cm );
	}
}

static void free_send_state_map()
{
	ker_free(send_state.map);
	send_state.map = NULL;
	ker_timer_stop(KER_FETCHER_PID, FETCHER_TRANSMIT_TID);
}


static int8_t fetcher_handler(void *state, Message *msg)
{
    switch (msg->type) {
        case MSG_FETCHER_FRAGMENT:
        {
			fetcher_fragment_t *f;
			f = (fetcher_fragment_t*)msg->data;
			f->key = entohs( f->key );
			f->frag_id = entohs(f->frag_id);

			DEBUG_PID(KER_FETCHER_PID,"MSG_FETCHER_FRAGMENT:\n");
			handle_overheard_fragment(msg);
			if(fst == NULL) {
				DEBUG_PID(KER_FETCHER_PID, "NO Request!!!\n");
				return SOS_OK;  //!< no request
			}
			//DEBUG_PID(KER_FETCHER_PID,"calling restart_request_timer()\n");
			restart_request_timer();
			fst->retx = 0;
			//DEBUG_PID(KER_FETCHER_PID,"calling handle_data()\n");
			return handle_data(msg);
		}
		case MSG_FETCHER_REQUEST:
		{
			fetcher_bitmap_t *bmap =
				(fetcher_bitmap_t *) msg->data;
			bmap->key = entohs( bmap->key );

			//! received request from neighbors
			DEBUG("handling request to %d from %d\n", msg->daddr, msg->saddr);
			if(msg->daddr == ker_id()) {
				return handle_request(msg);
			}
			if(fst == NULL) return SOS_OK;  //!< no request
			restart_request_timer();
			fst->retx = 0;
			return SOS_OK;
		}
		case MSG_TIMER_TIMEOUT:
		{
			MsgParam *params = (MsgParam*)(msg->data);
			if(params->byte == FETCHER_REQUEST_TID) {
				//DEBUG("request timeout\n");
				if( no_mem_retry ) {
					send_fetcher_done();
					return SOS_OK;
				}
				handle_request_timeout();
			} else if(params->byte == FETCHER_TRANSMIT_TID) {
				//DEBUG("send fragment timeout\n");
				send_fragment();
			}
			return SOS_OK;
		}
#ifdef SOS_HAS_EXFLASH
		case MSG_EXFLASH_WRITEDONE:
		{
			ker_free(send_state.fragr);
			send_state.fragr = NULL;
			check_map_and_post();
			return SOS_OK;
		}
		case MSG_EXFLASH_READDONE:
		{

			post_auto(KER_FETCHER_PID,
					KER_FETCHER_PID,
					MSG_FETCHER_FRAGMENT,
					sizeof(fetcher_fragment_t),
					send_state.frag,
					SOS_MSG_RELEASE,
					send_state.dest);
			send_state.frag = NULL;
			return SOS_OK;
		}
#endif
		case MSG_INIT:
		{
			send_state.map = NULL;
			send_state.frag = NULL;
			send_state.fragr = NULL;
			ker_msg_change_rules(KER_FETCHER_PID, SOS_MSG_RULES_PROMISCUOUS);
			ker_permanent_timer_init(&(send_state.timer), KER_FETCHER_PID, FETCHER_TRANSMIT_TID, TIMER_REPEAT);
			ker_timer_init(KER_FETCHER_PID, FETCHER_REQUEST_TID, TIMER_ONE_SHOT);
			return SOS_OK;
		}

	}
	return -EINVAL;
}

static inline void restart_request_timer()
{
	ker_timer_restart(KER_FETCHER_PID,
			FETCHER_REQUEST_TID,
			FETCHER_REQUEST_WATCHDOG + (FETCHER_REQUEST_BACKOFF_SLOT * ((ker_rand() % FETCHER_REQUEST_MAX_SLOT) + 1)));
}

static inline void handle_request_timeout()
{
	if(fst == NULL) {
		return;
	}
	//sos_assert(fst != NULL);
	//DEBUG("handle request timeout, retx = %d\n", fst->retx);
	fst->retx++;
	if(fst->retx <= FETCHER_REQUEST_MAX_RETX) {
		fetcher_bitmap_t *m;
		uint8_t size = sizeof(fetcher_bitmap_t) + fst->map.bitmap_size;

		DEBUG_PID(KER_FETCHER_PID,"send request to %d\n", fst->src_addr);
		print_bitmap(&fst->map);

		m = (fetcher_bitmap_t *) ker_malloc( size, KER_FETCHER_PID);
		if( m != NULL ) {
			memcpy(m, &(fst->map), size);
			m->key = ehtons( m->key );
			post_auto(KER_FETCHER_PID,
					KER_FETCHER_PID,
					MSG_FETCHER_REQUEST,
					size,
					m,
					SOS_MSG_RELEASE,
					fst->src_addr);
		}
		restart_request_timer();
	} else {
		DEBUG_PID(KER_FETCHER_PID, "request failed!!!\n");
		//codemem_close(&fst->cm, false);
		send_fetcher_done();
	}
}

static inline void handle_overheard_fragment(Message *msg)
{
	fetcher_fragment_t *f;

	if ( send_state.map == NULL ) {
		return;
	}
	f = (fetcher_fragment_t*)msg->data;
	if( (send_state.map->key != f->key) ) {
		return;
	}	
	DEBUG_PID(KER_FETCHER_PID, "Surpress %d\n", (f->frag_id));
	send_state.map->bitmap[(f->frag_id) / 8] &= ~(1 << ((f->frag_id) % 8));
}

static inline void send_fragment()
{
	uint16_t frag_id;
	uint8_t i, j;
	uint8_t mask = 1;
	uint8_t ret;
	fetcher_fragment_t *out_pkt;
	fetcher_cam_t *cam;

	if ( send_state.map == NULL ) {
		ker_timer_stop(KER_FETCHER_PID, FETCHER_TRANSMIT_TID);
		return;
	}

	cam = (fetcher_cam_t *) ker_cam_lookup(send_state.map->key);

	if ( cam == NULL ) {
		// file got deleted. give up!
		free_send_state_map();
		return;
	}

	if ( send_state.frag != NULL) {
		//! timer fires faster than data reading.  Highly unlikely...
		//! but we still handle it.
		return;
	}

	//! search map and find one fragment to send
	for(i = 0; i < send_state.map->bitmap_size; i++) {
		//! for each byte
		if(send_state.map->bitmap[i] != 0) {
			break;
		}
	}
	if(i == send_state.map->bitmap_size) {
		/*
		 * Did not find any block...
		 */
		free_send_state_map();
		return;
	}

	//sos_assert(i < send_state.map->bitmap_size);

	frag_id = i * 8;
	mask = 1;
	for(j = 0; j < 8; j++, mask = mask << 1) {
		if(mask & (send_state.map->bitmap[i])) {
			send_state.map->bitmap[i] &= ~(mask);
			break;
		}
	}
	//sos_assert(j < 8);
	frag_id += j;
	print_bitmap(send_state.map);
	out_pkt = (fetcher_fragment_t*)ker_malloc(sizeof(fetcher_fragment_t), KER_FETCHER_PID);
	if(out_pkt == NULL){
		DEBUG_PID(KER_FETCHER_PID,"malloc fetcher_fragment_t failed\n");
		goto send_fragment_postproc;
	}
	out_pkt->frag_id = ehtons(frag_id);
	out_pkt->key = ehtons(send_state.map->key);

	ret = ker_codemem_read(cam->cm, KER_FETCHER_PID,
			out_pkt->fragment, FETCHER_FRAGMENT_SIZE,
			frag_id * (code_addr_t)FETCHER_FRAGMENT_SIZE);
	if(ret == SOS_SPLIT) {
		send_state.frag = out_pkt;
	} else if(ret != SOS_OK){
		DEBUG_PID(KER_FETCHER_PID, "codemem_read failed\n");
		ker_free(out_pkt);
		goto send_fragment_postproc;
	}

#ifndef MINIELF_LOADER
	if (cam->fetchtype == FETCHTYPE_MODULE) {
		fntable_unfix_address(
				ker_codemem_get_header_address( cam->cm ),
				send_state.num_funcs,
				out_pkt->fragment,
				FETCHER_FRAGMENT_SIZE,
				(func_addr_t) frag_id * FETCHER_FRAGMENT_SIZE);
	}
#endif

	//DEBUG("out_pkt has addr %x\n", (int)out_pkt);
	DEBUG_PID(KER_FETCHER_PID, "send_fragment: frag_id = %d to %d\n", frag_id, send_state.dest);
	ret = post_auto(KER_FETCHER_PID,
			KER_FETCHER_PID,
			MSG_FETCHER_FRAGMENT,
			sizeof(fetcher_fragment_t),
			out_pkt,
			SOS_MSG_RELEASE,
			send_state.dest);
send_fragment_postproc:
	if(check_map(send_state.map)) {
		//! no more fragment to send
		free_send_state_map();
	}
}

static inline int8_t set_num_funcs_in_send_state(sos_cam_t key)
{
	fetcher_cam_t *cam;
	mod_header_t hdr;
	cam = (fetcher_cam_t *) ker_cam_lookup(key);

	if( cam == NULL ) {
		// We don't have the module, give up
		return -EINVAL;
	}

	if( cam->fetchtype != FETCHTYPE_MODULE ) {
		// not module
		return SOS_OK;
	}
	// if we are currently fetching the same module
	if( fst != NULL && (fst->map.key == key) ) {
		if( (fst->map.bitmap[0] & 0x01) != 0 ) {
			// number func is not available
			return -EINVAL;
		}
	}
	// read module header from flash and set it
	if( ker_codemem_read(cam->cm, KER_FETCHER_PID, 
				&hdr, sizeof(mod_header_t), 0) != SOS_OK ) {
		return -EINVAL;
	}
	send_state.num_funcs = hdr.num_sub_func + hdr.num_prov_func;
	return SOS_OK;
}

static int8_t handle_request(Message *msg)
{
	int8_t ret;
	//! setup a periodic timer to send fragments
	if(send_state.map == NULL) {
		fetcher_bitmap_t *b = (fetcher_bitmap_t*) msg->data;
		ret = set_num_funcs_in_send_state(b->key);
		if( ret != SOS_OK ) {
			// cannot find num funcs, give up
			return SOS_OK;
		}
		ret = ker_timer_restart(KER_FETCHER_PID,
				FETCHER_TRANSMIT_TID,
				FETCHER_SENDING_FRAGMENT_INTERVAL);
		if(ret == SOS_OK) {
			send_state.map = (fetcher_bitmap_t*)ker_msg_take_data(KER_FETCHER_PID, msg);
			send_state.dest = msg->saddr;
			DEBUG_PID(KER_FETCHER_PID,"send_state.map = 0x%x send_state.dest = 0x%x\n", (int)send_state.map, send_state.dest);
		} else {
			return -ENOMEM;
		}
	} else {
		fetcher_bitmap_t *map = (fetcher_bitmap_t*)msg->data;

		//! XXX change to broadcast
		//send_state.dest = BCAST_ADDRESS;
		DEBUG_PID(KER_FETCHER_PID,"else send_state.dest = %x\n", send_state.dest);
		//! merge wanted list
		if((send_state.map->key == map->key)) {
			uint8_t i;
			for(i = 0; i < send_state.map->bitmap_size &&
					i < map->bitmap_size; i++) {
				send_state.map->bitmap[i] |= map->bitmap[i];
			}

		}
	}

	if( fst != NULL && (fst->map.key == send_state.map->key) ) {
		//! send only those we have
		uint8_t i;
		for(i = 0; i < send_state.map->bitmap_size; i++ ) {
			uint8_t tmp;
			//! this is the fragment that has been requested but we dont have
			tmp = send_state.map->bitmap[i] & fst->map.bitmap[i];
			send_state.map->bitmap[i] &= ~tmp;
		}
	}
	return SOS_OK;
}

static int8_t handle_data(Message *msg)
{
	fetcher_fragment_t *f;
	int8_t ret = -EINVAL;
	fetcher_cam_t *cam;

	f = (fetcher_fragment_t*)msg->data;

	DEBUG_PID(KER_FETCHER_PID, "fetcher: get data, key = %d, frag_id = %d\n", f->key, f->frag_id);
	//msg_print(msg);

	if(f->key != fst->map.key) {
		DEBUG_PID(KER_FETCHER_PID,"version mis-match\n");
		return SOS_OK;
	}

	cam = (fetcher_cam_t *) ker_cam_lookup(f->key);
	if( cam == NULL ) {
		// XXX cannot find CAM...
		// TODO: need to inform upper layer...
		return -EINVAL;
	}
	if( (fst->map.bitmap[(f->frag_id) / 8] & (1 << ((f->frag_id) % 8))) == 0 ) {
		// we already have the fragment...
		return SOS_OK;
	}

	if((f->frag_id != 0) && ((fst->map.bitmap[0] & 0x01) != 0)
		&& (cam->fetchtype == FETCHTYPE_MODULE)) {
		// if the first fragment is not available, we drop the packet
		return SOS_OK;
	}

#ifndef MINIELF_LOADER
	//DEBUG("calling codemem_write\n");
	if(f->frag_id == 0 && cam->fetchtype == FETCHTYPE_MODULE) {
		// if this is the first fragment, get number of functions
		mod_header_t * hdr = (mod_header_t*) f->fragment;
		fst->num_funcs = hdr->num_sub_func + hdr->num_prov_func;
	}

	if( cam->fetchtype == FETCHTYPE_MODULE ) {
		fntable_fix_address(
				ker_codemem_get_header_address( cam->cm ),
				fst->num_funcs,
				f->fragment,
				FETCHER_FRAGMENT_SIZE,
				(func_addr_t)FETCHER_FRAGMENT_SIZE * f->frag_id);
	}
#endif

	ret = ker_codemem_write(cam->cm, KER_FETCHER_PID,
			f->fragment, FETCHER_FRAGMENT_SIZE,
			(code_addr_t)FETCHER_FRAGMENT_SIZE * f->frag_id);
	if(ret == SOS_SPLIT) {
		send_state.fragr = (fetcher_fragment_t*) ker_msg_take_data(KER_FETCHER_PID, msg);	
		f = send_state.fragr;
	} else if(ret != SOS_OK) {
		//DEBUG("codemem_write failed\n");
		return SOS_OK;
	}

	fst->map.bitmap[(f->frag_id) / 8] &= ~(1 << ((f->frag_id) % 8));
	check_map_and_post();
	return SOS_OK;
}

static void check_map_and_post()
{
	if(fst == NULL) {
		return;
	}
	if(check_map(&fst->map)) {
		fst->retx = 0;
#ifdef SOS_SIM
		/*
		 * We update module version here
		 */
		// TODO: figure out the right place...
		//set_version_to_sim(fst->map.mod_id, fst->map.version);
#endif
		DEBUG_PID(KER_FETCHER_PID, "Request Done!!!\n");
		ker_timer_stop(KER_FETCHER_PID, FETCHER_REQUEST_TID);
		send_fetcher_done();
	}
}

static void start_new_fetch(void)
{
	fst = fst->next;
	if (fst) {
		fetcher_cam_t *cam;
		cam = ker_cam_lookup( fst->map.key );
		cam->status = FETCHING_STARTED;
		ker_timer_start(KER_FETCHER_PID,
				FETCHER_REQUEST_TID,
				FETCHER_REQUEST_BACKOFF_SLOT *
				((ker_rand() % FETCHER_REQUEST_MAX_SLOT) + 1));
	}	
}

static void send_fetcher_done()
{
	fetcher_cam_t *cam;
	DEBUG_PID(KER_FETCHER_PID,"Fetcher Done!\n");
	if( post_long(fst->requester, KER_FETCHER_PID, MSG_FETCHER_DONE, sizeof(fetcher_state_t), fst, SOS_MSG_RELEASE) != SOS_OK ) {

		no_mem_retry = true;
		ker_timer_start(KER_FETCHER_PID,
				FETCHER_REQUEST_TID,
				1024);
		return;
	}
	cam = ker_cam_lookup( fst->map.key );
	cam->status = FETCHING_DONE;
	no_mem_retry = false;
	start_new_fetch();
}

//! check whether we have completed the fragment map
static bool check_map(fetcher_bitmap_t *m)
{
	uint8_t i;
	for(i = 0; i < m->bitmap_size; i++) {
		if(m->bitmap[i] != 0) {
			return false;
		}
	}
	return true;
}

#ifdef SOS_DEBUG_FETCHER
static void print_bitmap(fetcher_bitmap_t *m)
{
	uint8_t i;
	int16_t delt = 0;
	char buf[DEBUG_BUFFER_SIZE];
	delt = sprintf(buf,"map: ");
	for(i = 0; i < m->bitmap_size; i++) {
		delt += sprintf(buf+delt,"0x%x ", m->bitmap[i]);
	}
	DEBUG_PID(KER_FETCHER_PID,"%s\n",buf);
}
#endif

int8_t fetcher_init()
{
	sched_register_kernel_module(&fetcher_module, sos_get_header_address(mod_header), NULL);
	return SOS_OK;
}

