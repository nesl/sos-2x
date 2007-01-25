/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4 shiftwidth=4 softtabstop=4 cindent: */
#include <module.h>
#include <sos.h>
#include <sos_sched.h>
#include <sos_module_fetcher.h>

// for low level memory i/o
#include <hardware_proc.h>

#ifdef LOADER_NET_EXPERIMENT
#include <led.h>
#endif

#include "loader.h"

#ifdef SOS_SIM
#include <sim_interface.h>
#endif

#ifdef MINIELF_LOADER
#include <melfloader.h>
#endif

#ifdef SOS_SFI
#include <sos_mod_verify.h>
#include <sfi_jumptable.h>
#include <sfi_exception.h>
#endif

#define LED_DEBUG
#include <led_dbg.h>

/**
 * Configurable parameters
 */
#ifndef SOS_DEBUG_DFT_LOADER
#undef DEBUG
#define DEBUG(...)
#undef DEBUG_PID
#define DEBUG_PID(...)
#endif

enum {
	TOU_INTERVAL     =   1L * 1024L,    // 1 second
	TOU_MAX          =   10,            
   	OVERHEARD_K      =   1,             	
};

enum {
	MAINTAIN     = 0,
	SEND_DATA    = 1,        //!< sending data
	SEND_ADV     = 2,        //!< sending old adv
};

enum {
	NEW_OP_OK            = 0,
	NEW_OP_BLOCKED,
	NEW_OP_FAILED,
};

static int8_t loader_handler(void *state, Message *msg);

static mod_header_t mod_header SOS_MODULE_HEADER =
{
	.mod_id             = KER_DFT_LOADER_PID,
	.state_size         = 0,
	.num_timers         = 0,
	.num_sub_func       = 0,
	.num_prov_func      = 0,
	.module_handler     = loader_handler,
};

/**
 * Timer TIDs
 */
enum {
	TOU_TID   = 0,
	TRAN_TID  = 1,
	DATA_TID  = 2,
};

/**
 * State
 */
typedef struct {
	sos_timer_t tou_timer;
	sos_timer_t tran_timer;
	sos_timer_t data_timer;
	uint8_t     tou;	
	uint8_t     net_state;
	uint8_t     tran_count;
	uint8_t     data_count;
	uint16_t    recent_neighbor;
	uint8_t     blocked;

	msg_version_data_t *version_data;
} dft_loader_t;

/*
 * Forward declaration
 */
static void process_version_data(msg_version_data_t *v, uint16_t saddr );

/*
 * State data
 */
static dft_loader_t  st;

static void block_protocol()
{
	st.blocked = 1;
}

static void restartInterval( uint8_t new_tou )
{
	int32_t tou_interval = 0;
	int32_t rand_val = 0;
	ker_timer_stop( KER_DFT_LOADER_PID, TOU_TID  );	
	ker_timer_stop( KER_DFT_LOADER_PID, TRAN_TID );	
	ker_timer_stop( KER_DFT_LOADER_PID, DATA_TID );	

	if ( new_tou < TOU_MAX ) {
		st.tou = new_tou;
	} else {
		st.tou = TOU_MAX;
	}
	st.tran_count = 0;
	st.data_count = 0;

	tou_interval = TOU_INTERVAL * (1L << st.tou);
	
	ker_timer_start( KER_DFT_LOADER_PID, TOU_TID, tou_interval );
	if ( st.net_state & SEND_DATA ) {
		//! pick random timeout between tou_interval / 2 and tou_interval
		rand_val = ker_rand() % (tou_interval / 2);
		rand_val += (tou_interval / 2);
		ker_timer_start( KER_DFT_LOADER_PID, DATA_TID, rand_val );
	} else {
		//! pick random timeout between tou_interval / 2 and tou_interval
		rand_val = ker_rand() % (tou_interval / 2);
		rand_val += (tou_interval / 2);
		ker_timer_start( KER_DFT_LOADER_PID, TRAN_TID, rand_val );
	}
}	

static int8_t handle_timeout( Message *msg )
{
	MsgParam* params = ( MsgParam * )( msg->data );

	switch( params->byte ) {
		case TOU_TID:
		{
			if( st.net_state != MAINTAIN ) {
				restartInterval( 0 );
			} else {
				restartInterval( st.tou + 1 );
			}
			break;
		}
		case TRAN_TID:
		{
			/*
			 * Transmit Version number
			 */
			if( st.blocked == 0 ) {
			version_t *v;
			v = ker_malloc( sizeof(version_t), KER_DFT_LOADER_PID );
			if( v == NULL ) return -ENOMEM;
			*v = version_hton(st.version_data->version);
			
			post_link(KER_DFT_LOADER_PID, KER_DFT_LOADER_PID, 
					MSG_VERSION_ADV, sizeof(version_t), v, 
					SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO, 
					BCAST_ADDRESS);
			}
			break;
		}
		case DATA_TID:
		{
			msg_version_data_t *d;
			
			d = (msg_version_data_t *) ker_malloc(
				sizeof( msg_version_data_t ), 
				KER_DFT_LOADER_PID);

			if( d != NULL ) {
				memcpy(d, st.version_data, sizeof( msg_version_data_t ));

				d->version = version_hton( st.version_data->version );
				
				post_link(KER_DFT_LOADER_PID, KER_DFT_LOADER_PID, 
						MSG_VERSION_DATA, 
						sizeof( msg_version_data_t ),
						d, 
						SOS_MSG_RELEASE | SOS_MSG_ALL_LINK_IO, 
						BCAST_ADDRESS);
			}
			st.net_state &= ~SEND_DATA;
			break;
		}
	}
	return SOS_OK;
}

static int8_t handle_version_adv( Message *msg )
{
	msg_version_adv_t *pkt = (msg_version_adv_t *) msg->data;

	if( pkt->version > st.version_data->version ) {
		// Someone has new version, tell them we don't have quickly...
		restartInterval( 0 );
	} else if( pkt->version < st.version_data->version ) {
		if( ( st.net_state & SEND_DATA ) == 0 ) {
			st.net_state |= SEND_DATA;
			restartInterval( 0 );
		} 
	} else {
		st.tran_count += 1;
		if( st.tran_count >= OVERHEARD_K ) {
			ker_timer_stop( KER_DFT_LOADER_PID, TRAN_TID ); 
		}
	}
	return SOS_OK;
}

static int8_t handle_version_data( Message *msg ) 
{
	msg_version_data_t *pkt = (msg_version_data_t *) msg->data;

	if( st.net_state & SEND_DATA ) {
		/*
		 * We have data to send
		 */
		if( pkt->version == st.version_data->version) {
			st.data_count ++;
			if( st.data_count >= OVERHEARD_K ) {
				st.net_state &= ~SEND_DATA;
				ker_timer_stop( KER_DFT_LOADER_PID, DATA_TID );
			}
			return SOS_OK;
		}
	}
	if( pkt->version > st.version_data->version ) {
		restartInterval( 0 );
		ker_free(st.version_data);
		st.version_data = (msg_version_data_t*) ker_msg_take_data(KER_DFT_LOADER_PID, msg);
		process_version_data( st.version_data, msg->saddr );
	}
	return SOS_OK;
}

static int8_t request_new_module(sos_cam_t key, loader_cam_t *cam, uint8_t size, uint16_t saddr, uint8_t type)
{
  cam->code_size = size; 
	cam->fetcher.fetchtype = type;
	cam->fetcher.cm = ker_codemem_alloc(
			size * LOADER_SIZE_MULTIPLIER, 
			CODEMEM_TYPE_EXECUTABLE);
	DEBUG_PID( KER_DFT_LOADER_PID, "request new module with size = %d\n", 
			size * LOADER_SIZE_MULTIPLIER);
	if(cam->fetcher.cm == CODEMEM_INVALID) {
		return -ENOMEM;
	}
	if( fetcher_request( KER_DFT_LOADER_PID, key, (uint16_t)size * LOADER_SIZE_MULTIPLIER, saddr ) 
			!= SOS_OK ) {
		ker_codemem_free( cam->fetcher.cm );
		return -ENOMEM;
	}
	block_protocol();
	return SOS_OK;

}

static void process_version_data( msg_version_data_t *v, uint16_t saddr ) 
{
	uint8_t i;
	for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES + NUM_LOADER_MODULE_ENTRIES; i++ ) {
		sos_cam_t key = ker_cam_key( KER_DFT_LOADER_PID, i);
		loader_cam_t *cam;
		uint8_t type;
		uint8_t size;
		uint8_t ver;

		if( i < NUM_LOADER_PARAMS_ENTRIES) {
			size = (v->pam_size[i]);
			ver = (v->pam_ver[i]);
			type = FETCHTYPE_DATA;
		} else {
			size = (v->mod_size[i - NUM_LOADER_PARAMS_ENTRIES]);
			ver = (v->mod_ver[i - NUM_LOADER_PARAMS_ENTRIES]);
			type = FETCHTYPE_MODULE;
		}

		cam = ker_cam_lookup( key );

		if( cam == NULL && size == 0 ) {
			// We cannot find entry and the size is zero
			// skip this slot
			continue;
		}


		if( cam == NULL ) {
			// we need to add a new module
			cam = (loader_cam_t*) ker_malloc(sizeof(loader_cam_t), KER_DFT_LOADER_PID);
			if( cam == NULL) {
				return;
			}
			if( ker_cam_add( key, cam ) != SOS_OK ) {
				ker_free( cam );
				return;
			}
		} else {
			// we need to replace a module
			if( cam->version == ver ) {
				continue;
			}
			//! new version of module found...
			if( cam->fetcher.status != FETCHING_DONE ) {
				if( cam->fetcher.status == FETCHING_STARTED ) {
					st.blocked = 0;
					restartInterval( 0 );
				}
				fetcher_cancel( KER_DFT_LOADER_PID, key );	
				ker_codemem_free(cam->fetcher.cm);
			} else /* if( cam->fetcher.status == FETCHING_DONE ) */ {
				ker_codemem_free(cam->fetcher.cm);
			}
			if( size == 0 ) {
				//! an rmmod case
				ker_cam_remove( key );
				ker_free( cam );
				continue;
			} 
			//! an insmod case with cam
		} 
		if( request_new_module( key, cam, size, saddr, type) != SOS_OK ) {
			ker_cam_remove( key );
			ker_free( cam );
		} else {
			// another insmod case
			cam->version = ver;
			// only do one fetching
			return;
		}
	}
}

static int8_t handle_fetcher_done( Message *msg ) 
{
	fetcher_state_t *f = (fetcher_state_t*) msg->data;
	if( is_fetcher_succeed( f ) == true ) {
		//mod_header_ptr p;
		loader_cam_t *cam;
		fetcher_commit(f, true);

		st.blocked = 0;
		restartInterval( 0 );

		cam = ker_cam_lookup( f->map.key );
		if( cam->fetcher.fetchtype == FETCHTYPE_DATA) {
			uint8_t buf[2];
			ker_codemem_read( cam->fetcher.cm, KER_DFT_LOADER_PID, buf, 2, 0);
			post_short(buf[0], KER_DFT_LOADER_PID, MSG_LOADER_DATA_AVAILABLE,
					buf[1], cam->fetcher.cm, 0);
			DEBUG_PID(KER_DFT_LOADER_PID, "Data Ready\n" );
#ifdef LOADER_NET_EXPERIMENT
			ker_led(LED_GREEN_TOGGLE);
#endif
		} else {
		  uint8_t mcu_type;
#ifndef SOS_SIM
		  uint8_t plat_type;
#endif
		  mod_header_ptr p;
#ifdef MINIELF_LOADER
		  // Link and load the module here
		  melf_load_module(cam->fetcher.cm);
#endif//MINIELF_LOADER		  
		  // Get the address of the module header
		  p = ker_codemem_get_header_address( cam->fetcher.cm ); 

		  // get processor type and platform type
		  mcu_type = sos_read_header_byte(p, 
										  offsetof( mod_header_t, processor_type )); 
#ifdef SOS_SIM
		  if( (mcu_type == MCU_TYPE) )
			// In simulation, we don't check for platform
#else
			plat_type = sos_read_header_byte(p, 
											 offsetof( mod_header_t, platform_type )); 
		  if( (mcu_type == MCU_TYPE) && 
			  ( plat_type == HW_TYPE || plat_type == PLATFORM_ANY) )
#endif
			{
			  
			  /*
			   * If MCU is matched, this means we are using the same 
			   * instruction set.
			   * And if this module is for this *specific* platform or 
			   * simply for all platform with the same MCU
			   */
			  // mark module executable
			  ker_codemem_mark_executable( cam->fetcher.cm );
			  if (cam->version & 0x80) {
#ifdef SOS_SFI
#ifdef MINIELF_LOADER
				sfi_modtable_register(cam->fetcher.cm);
				if (SOS_OK == ker_verify_module(cam->fetcher.cm)){
				  sfi_modtable_flash(p);
				  ker_register_module(p);
				}
				else
				  sfi_exception(KER_VERIFY_FAIL_EXCEPTION);
#else				
				uint16_t init_offset, code_size, handler_addr;
				uint32_t handler_byte_addr, module_start_byte_addr;
				uint16_t handler_sfi_addr;
				handler_sfi_addr = sos_read_header_ptr(p, offsetof(mod_header_t, module_handler));
				handler_addr = sfi_modtable_get_real_addr(handler_sfi_addr);				
				handler_byte_addr = (handler_addr * 2);
				module_start_byte_addr = (p * 2);
				init_offset = (uint16_t)(handler_byte_addr - module_start_byte_addr);
				code_size = cam->code_size * LOADER_SIZE_MULTIPLIER;
				if (SOS_OK == ker_verify_module(cam->fetcher.cm, init_offset, code_size)){
				  sfi_modtable_flash(p);				  
				  ker_register_module(p);
				}
				else
				  sfi_exception(KER_VERIFY_FAIL_EXCEPTION);
#endif //MINIELF_LOADER
#else
				ker_register_module(p);
#endif //SOS_SFI
			  }
			}

		}
		process_version_data( st.version_data, st.recent_neighbor);
	} else {
	  DEBUG_PID( KER_DFT_LOADER_PID, "Fetch failed!, request %d\n", st.recent_neighbor);
	  f = (fetcher_state_t*) ker_msg_take_data(KER_DFT_LOADER_PID, msg);
	  fetcher_restart( f, st.recent_neighbor );
	}
	return SOS_OK;
}

static int8_t handle_loader_ls_on_node( Message *msg )
{
	msg_ls_reply_t *reply;
	uint8_t i;

	reply = (msg_ls_reply_t *) ker_malloc( sizeof( msg_ls_reply_t ),
			KER_DFT_LOADER_PID );
	if( reply == NULL ) return -ENOMEM;

	for( i = 0; i < NUM_LOADER_PARAMS_ENTRIES; i ++ ) {
		sos_cam_t key = ker_cam_key( KER_DFT_LOADER_PID, i );
		loader_cam_t *cam;
		uint8_t buf[2];

		cam = ker_cam_lookup( key );
		if( cam != NULL && cam->fetcher.status == FETCHING_DONE 
				&& (ker_codemem_read( cam->fetcher.cm, KER_DFT_LOADER_PID, 
						buf, 2, 0) == SOS_OK)) {
			DEBUG_PID( KER_DFT_LOADER_PID, "Data(%d) = %d %d\n", i, buf[0], buf[1]);
			reply->pam_dst_pid[ i ] = buf[0];
			reply->pam_dst_subpid[ i ] = buf[1];
		} else {
			DEBUG_PID( KER_DFT_LOADER_PID, "Data(%d) = NULL\n", i);
			/*
			if( cam != NULL && cam->fetcher.status == FETCHING_DONE) {
				DEBUG_PID( KER_DFT_LOADER_PID, "ker_codemem_read failed...\n");
			}
			*/
			reply->pam_dst_pid[ i ] = NULL_PID;
			reply->pam_dst_subpid[ i ] = 0;
		}
	}
	for( i = 0; i < NUM_LOADER_MODULE_ENTRIES; i ++ ) {
		sos_cam_t key = ker_cam_key( KER_DFT_LOADER_PID, 
				NUM_LOADER_PARAMS_ENTRIES + i );
		loader_cam_t *cam;

		cam = ker_cam_lookup( key );

		if( cam != NULL && cam->fetcher.status == FETCHING_DONE ) {
			mod_header_ptr p;
			sos_code_id_t cid;
			// Get the address of the module header
			p = ker_codemem_get_header_address( cam->fetcher.cm );
			cid = sos_read_header_word( p, offsetof(mod_header_t, code_id));
			// warning: already netowrk order...
			reply->code_id[ i ] = cid;
		} else {
			reply->code_id[ i ] = 0;
		}
	}
	post_auto( KER_DFT_LOADER_PID,
			KER_DFT_LOADER_PID,
			MSG_LOADER_LS_REPLY,
			sizeof( msg_ls_reply_t ),
			reply,
			SOS_MSG_RELEASE,
			msg->saddr);
	return SOS_OK;
}

static int8_t handle_init()
{
	ker_permanent_timer_init((&st.tou_timer), KER_DFT_LOADER_PID, 
			TOU_TID, TIMER_ONE_SHOT);		
	ker_permanent_timer_init((&st.tran_timer), KER_DFT_LOADER_PID, 
			TRAN_TID, TIMER_ONE_SHOT);		
	ker_permanent_timer_init((&st.data_timer), KER_DFT_LOADER_PID, 
			DATA_TID, TIMER_ONE_SHOT);		
	st.net_state = MAINTAIN;
	st.tran_count = 0;
	st.data_count = 0;
	st.blocked    = 0;
	st.recent_neighbor = 0;
	st.version_data = ker_malloc(sizeof(msg_version_data_t), KER_DFT_LOADER_PID);
	if(st.version_data == NULL) return -ENOMEM;

	memset((uint8_t *) st.version_data, 0, sizeof(msg_version_data_t));
	/*
     * Use default version to be 1 so that loader_pc can probe loaded data
	 */
	st.version_data->version = 1;

	restartInterval( 0 );
	return SOS_OK;
}

#ifdef LOADER_NET_EXPERIMENT
//static bool experiment_started = false;
#ifndef EXPERIMENT_SIZE
#define EXPERIMENT_SIZE 256
#endif
static void start_experiment(uint16_t size)
{
	sos_cam_t key;
	loader_cam_t *cam;
	codemem_t cm;
	uint8_t buf[2];

	buf[0] = 128;
	buf[1] = 1;
	DEBUG_PID(KER_DFT_LOADER_PID, "start experiment: size = %d\n", size);
	cm = ker_codemem_alloc( size, CODEMEM_TYPE_EXECUTABLE);
	ker_codemem_write(cm, KER_DFT_LOADER_PID, buf, 2, 0);
	cam = ker_malloc(sizeof(loader_cam_t), KER_DFT_LOADER_PID);

	st.version_data->pam_ver[0]++;
	st.version_data->pam_size[0] = (size + (LOADER_SIZE_MULTIPLIER - 1)) / LOADER_SIZE_MULTIPLIER;
	st.version_data->version ++;

	key = ker_cam_key( KER_DFT_LOADER_PID, 0 );
	cam->fetcher.fetchtype = FETCHTYPE_DATA;
	cam->fetcher.cm = cm;
	ker_cam_add(key, cam);	
	restartInterval(0);
}
#endif

static int8_t loader_handler( void *state, Message *msg )
{
	switch (msg->type){
		case MSG_TIMER_TIMEOUT: 
		{ return handle_timeout( msg ); }

		case MSG_VERSION_ADV:
		{
			msg_version_adv_t *pkt = (msg_version_adv_t *) msg->data;
			pkt->version = version_ntoh(pkt->version);	
			if(pkt->version >= st.version_data->version) {
				st.recent_neighbor = msg->saddr;
			}
			return handle_version_adv( msg ); 
		}

		case MSG_VERSION_DATA:
		{ 
			msg_version_data_t *pkt = (msg_version_data_t *) msg->data;
			pkt->version = version_ntoh(pkt->version);
			if(pkt->version >= st.version_data->version) {
				st.recent_neighbor = msg->saddr;
			}
			return handle_version_data( msg ); 
		}

		case MSG_LOADER_LS_ON_NODE:
		{
			return handle_loader_ls_on_node( msg );
		}

		case MSG_FETCHER_DONE:
		{ 
			return handle_fetcher_done( msg ); 
		}

		case MSG_INIT:
		{ 
#ifdef LOADER_NET_EXPERIMENT
			handle_init();
			if( ker_id() == 0 ) {
				start_experiment(EXPERIMENT_SIZE);	
			}
#else
			return handle_init(); 
#endif
		}

		default:
		return -EINVAL;
	}
	return SOS_OK;
}



#ifndef _MODULE_
mod_header_ptr loader_get_header()
{
	return sos_get_header_address(mod_header);
}
#endif


