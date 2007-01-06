
#ifndef _SOS_MODULE_FETCHER_H
#define _SOS_MODULE_FETCHER_H

#ifndef _MODULE_
#include <sos_inttypes.h>
#include <sos_sched.h>
#include <malloc.h>
#include <codemem.h>
#include <sos_timer.h>
#include <fntable.h>
#endif
#include <sos_cam.h>

typedef struct {
	codemem_t cm;
	uint8_t   fetchtype;
	uint8_t   status;
} fetcher_cam_t;

// use in fetcher_cam_t.fetchtype for patching SOS modules
enum {
	FETCHTYPE_MODULE    = 0,
	FETCHTYPE_DATA      = 1,
};

// use in fetcher_cam_t.status for fetching status
enum {
	FETCHING_STARTED    = 0,
	FETCHING_DONE       = 1,
	FETCHING_QUEUED     = 2,
};


enum {
	//! the size of fragment fetcher will fetch in at 1 time
	FETCHER_FRAGMENT_SIZE   =  64L,
};

//! timers
enum {
	FETCHER_REQUEST_TID    = 1,
	FETCHER_TRANSMIT_TID      = 2,
	FETCHER_REQUEST_BACKOFF_SLOT  = 256L,  //!< 0.25 seconds
	FETCHER_REQUEST_MAX_SLOT  = 8,
	FETCHER_REQUEST_WATCHDOG  = 5 * 1024L, //!< 5 seconds.
	FETCHER_REQUEST_MAX_RETX  = 3,
};

//! message types
enum {
	MSG_FETCHER_REQUEST        =   (MOD_MSG_START + 0),
	MSG_FETCHER_FRAGMENT       =   (MOD_MSG_START + 1),
};

typedef struct {
	sos_cam_t key;
	uint8_t bitmap_size;  //!< the size of the bitmap
	uint8_t bitmap[];  //!< the bitmap that stores fetched fragments,
	                   //! 0 means the fragment is fetched and 1 means the
	                   //! fragment is missing.
} PACK_STRUCT
fetcher_bitmap_t;

//! the packet that carries the fragment
typedef struct {
	uint16_t     frag_id;   //! fragment number
	sos_cam_t    key;
	uint8_t      fragment[FETCHER_FRAGMENT_SIZE];
} PACK_STRUCT
fetcher_fragment_t;

typedef struct fetcher_state_str {
	//! the location of fragment to be fetched
	uint16_t                    src_addr;
   	//! the module which requests the fetching
	sos_pid_t                   requester;
	//! number of retransmission of request
	uint8_t                     retx;         
	//! number of function poiinters in the module
	//! this will be discovered during fetching
	uint8_t                     num_funcs;
	codemem_t                   cm;   
	//! link list to store restart request
	struct fetcher_state_str    *next;
	//! bitmap for wanted fragments (Has to be the last element in the struct)
	fetcher_bitmap_t            map;
}
fetcher_state_t;

int8_t fetcher_init(void);

/**
 * @brief request a particular module to be fetchered
 *
 * @param requester the id that is requesting the fetch
 * @param mod_id the id to be fetched
 * @param version the version of the module to be fetched
 * @param size the size of module to be fetched
 * @param src  the node id to be fetched from
 *
 */
int8_t fetcher_request(sos_pid_t req_id, sos_cam_t key, uint16_t size, uint16_t src);

int8_t fetcher_cancel(sos_pid_t req_id, sos_cam_t key);

void fetcher_restart(fetcher_state_t *s, uint16_t src);

void fetcher_commit(fetcher_state_t *s, bool commit);

static inline bool is_fetcher_succeed(fetcher_state_t *s)
{
	if(s->retx <= FETCHER_REQUEST_MAX_RETX) {
		return true;
	}
	return false;
}


int8_t fetcher_init();

#endif


