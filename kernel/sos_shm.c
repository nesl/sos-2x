
#include <sos.h>
#include <slab.h>
#include <sos_shm.h>

enum {
	SHM_NUM_POOL_ITEMS = 4,     // number of items in one pool (MUST be less than 8)
	SHM_NUM_BINS       = 4,
	SHM_NUM_WAITER     = 2,
};

typedef struct _shm_str {
	sos_shm_t name;
	void *mem;
	sos_pid_t owner;
	sos_pid_t waiter[SHM_NUM_WAITER];  // the tasks that are waiting on the memory
	struct _shm_str *next;
} shm_cb;

typedef struct shm_pool_t {
	struct shm_pool_t *next;
	shm_cb pool[SHM_NUM_POOL_ITEMS];
	uint8_t alloc;
} shm_pool_t;

//
// Simple hash table
//
static shm_cb *shm_bin[SHM_NUM_BINS] = {NULL};

static slab_t shm_slab;

static inline uint8_t hash_bin(sos_shm_t name)
{
	return name % SHM_NUM_BINS;
}

static inline shm_cb *name_to_bin(sos_shm_t name)
{
	return shm_bin[hash_bin(name)];
}

static shm_cb *shm_lookup(sos_shm_t name) 
{
	shm_cb *shm = name_to_bin(name);
	
	while(shm != NULL) {
		if(shm->name == name) {
			return shm;
		}
		shm = shm->next;
	}
	return NULL;
}

//
// TODO: send message to the waiter
//
static void shm_send_event( shm_cb *cb, uint8_t type )
{
	uint8_t i;
	for( i = 0; i < SHM_NUM_WAITER; i++ ) {
		if( cb->waiter[i] != NULL_PID ) {
			post_short( cb->waiter[i], KER_SHM_PID, MSG_SHM, type, cb->name, 0 );
		}
	}
}

int8_t ker_shm_open(sos_pid_t pid, sos_shm_t name, void *shm)
{
	shm_cb *cb;	
	uint8_t i;
	
	if(shm_lookup(name) != NULL) {
		return -EEXIST;
	}

	cb = ker_slab_alloc( &shm_slab, KER_SHM_PID );

	if( cb == NULL ) { return -ENOMEM; }
	
	cb->name = name;
	cb->mem = shm;
	cb->owner = pid;
	for( i = 0; i < SHM_NUM_WAITER; i++ ) {
		cb->waiter[i] = NULL_PID;
	}

	cb->next = name_to_bin( name );
	shm_bin[ hash_bin( name ) ] = cb;
	return SOS_OK;	
}

int8_t ker_shm_update(sos_pid_t pid, sos_shm_t name, void *shm)
{
	shm_cb *cb = shm_lookup( name );
	
	if( cb == NULL ) {
		return -EBADF;
	}
	
	cb->mem = shm;
	
	shm_send_event( cb, SHM_UPDATED );
	return SOS_OK;
}

int8_t ker_shm_close(sos_pid_t pid, sos_shm_t name)
{
	shm_cb *head = name_to_bin(name);
	shm_cb *prev;
	shm_cb *curr;

	if( head == NULL ) {
		return -EBADF;
	}
	
	prev = head;
	curr = head;
	while(curr != NULL) {
		if( curr->name == name ) {
			if( pid != curr->owner ) {
				return -EPERM;
			}
			if( curr == head ) {
				//! remove head
				shm_bin[ hash_bin( name ) ] = head->next;
			} else {
				prev->next = curr->next;
			}
			shm_send_event( curr, SHM_CLOSED );
			ker_slab_free( &shm_slab, curr);
			return SOS_OK;
		}
		prev = curr;
		curr = curr->next;
	}
	return -EBADF;
}

void* ker_shm_get(sos_pid_t pid, sos_shm_t name)
{
	shm_cb *cb = shm_lookup( name );

	if(cb != NULL) return cb->mem;
	return NULL;
}

int8_t ker_shm_wait( sos_pid_t pid, sos_shm_t name )
{
	shm_cb *cb = shm_lookup( name );
	uint8_t i;
	
	if( cb == NULL ) {
		return -EBADF;
	}
	
	for( i = 0; i < SHM_NUM_WAITER; i++ ) {
		if( cb->waiter[i] == NULL_PID ) {
			cb->waiter[i] = pid;
			return SOS_OK;
		}
	}
	return -ENOMEM;
}

int8_t ker_shm_stopwait( sos_pid_t pid, sos_shm_t name )
{
	shm_cb *cb = shm_lookup( name );
	uint8_t i;
	
	if( cb == NULL ) {
		return -EBADF;
	}
	
	for( i = 0; i < SHM_NUM_WAITER; i++ ) {
		if( cb->waiter[i] == pid ) {
			cb->waiter[i] = NULL_PID;
			return SOS_OK;
		}
	}
	return -EINVAL;
}

#ifdef SOS_USE_PREEMPTION
int8_t ker_sys_shm_open( sos_shm_t name, void *shm )
{
  HAS_ATOMIC_PREEMPTION_SECTION;
  sos_pid_t my_id = ker_get_current_pid();
  ATOMIC_DISABLE_PREEMPTION();
  if( ker_shm_open( my_id, name, shm ) != SOS_OK ) {
    ATOMIC_ENABLE_PREEMPTION();
    return ker_mod_panic( my_id );
  }
  ATOMIC_ENABLE_PREEMPTION();
  return SOS_OK;
}

int8_t ker_sys_shm_update( sos_shm_t name, void *shm )
{
  HAS_ATOMIC_PREEMPTION_SECTION;
  sos_pid_t my_id = ker_get_current_pid();
  ATOMIC_DISABLE_PREEMPTION();
  if( ker_shm_update( my_id, name, shm ) != SOS_OK ) {
    ATOMIC_ENABLE_PREEMPTION();
    return ker_mod_panic( my_id );
  }
  ATOMIC_ENABLE_PREEMPTION();
  return SOS_OK;
}

int8_t ker_sys_shm_close( sos_shm_t name )
{
  HAS_ATOMIC_PREEMPTION_SECTION;
  sos_pid_t my_id = ker_get_current_pid();
  ATOMIC_DISABLE_PREEMPTION();
  if( ker_shm_close( my_id, name ) != SOS_OK ) {
    ATOMIC_ENABLE_PREEMPTION();
    return ker_mod_panic( my_id );
  }
  ATOMIC_ENABLE_PREEMPTION();
  return SOS_OK;
}

void* ker_sys_shm_get( sos_shm_t name )
{
  HAS_ATOMIC_PREEMPTION_SECTION;
  sos_pid_t my_id = ker_get_current_pid();
  void* ret;
  ATOMIC_DISABLE_PREEMPTION();
  ret = ker_shm_get( my_id, name );
  ATOMIC_ENABLE_PREEMPTION();
  return ret;
}

int8_t ker_sys_shm_wait( sos_shm_t name )
{
  HAS_ATOMIC_PREEMPTION_SECTION;
  sos_pid_t my_id = ker_get_current_pid();
  ATOMIC_DISABLE_PREEMPTION();
  if( ker_shm_wait( my_id, name ) != SOS_OK ) {
    ATOMIC_ENABLE_PREEMPTION();
    return ker_mod_panic( my_id );
  }
  ATOMIC_ENABLE_PREEMPTION();
  return SOS_OK;
}

int8_t ker_sys_shm_stopwait( sos_shm_t name )
{
  HAS_ATOMIC_PREEMPTION_SECTION;
  sos_pid_t my_id = ker_get_current_pid();
  ATOMIC_DISABLE_PREEMPTION();
  if( ker_shm_stopwait( my_id, name ) != SOS_OK ) {
    ATOMIC_ENABLE_PREEMPTION();
    return ker_mod_panic( my_id );
  }
  ATOMIC_ENABLE_PREEMPTION();
  return SOS_OK;
}

#else
int8_t ker_sys_shm_open( sos_shm_t name, void *shm )
{
	sos_pid_t my_id = ker_get_current_pid();
	
	if( ker_shm_open( my_id, name, shm ) != SOS_OK ) {
		return ker_mod_panic( my_id );
	}
	return SOS_OK;
}

int8_t ker_sys_shm_update( sos_shm_t name, void *shm )
{
	sos_pid_t my_id = ker_get_current_pid();
	
	if( ker_shm_update( my_id, name, shm ) != SOS_OK ) {
		return ker_mod_panic( my_id );
	}
	return SOS_OK;
}

int8_t ker_sys_shm_close( sos_shm_t name )
{
	sos_pid_t my_id = ker_get_current_pid();
	
	if( ker_shm_close( my_id, name ) != SOS_OK ) {
		return ker_mod_panic( my_id );
	}
	return SOS_OK;
}

void* ker_sys_shm_get( sos_shm_t name )
{
	sos_pid_t my_id = ker_get_current_pid();
	
	return ker_shm_get( my_id, name );
}

int8_t ker_sys_shm_wait( sos_shm_t name )
{
	sos_pid_t my_id = ker_get_current_pid();
	
	if( ker_shm_wait( my_id, name ) != SOS_OK ) {
		return ker_mod_panic( my_id );
	}
	return SOS_OK;
}

int8_t ker_sys_shm_stopwait( sos_shm_t name )
{
	sos_pid_t my_id = ker_get_current_pid();
	
	if( ker_shm_stopwait( my_id, name ) != SOS_OK ) {
		return ker_mod_panic( my_id );
	}
	return SOS_OK;
}
#endif
//
// Remove all data associated with the pid
//
int8_t shm_remove_all( sos_pid_t pid )
{
	uint8_t i;
	
	for( i = 0; i < SHM_NUM_BINS; i++ ) {
		shm_cb *prev = NULL;
		shm_cb *curr = shm_bin[i];
		prev = curr;
		
		while( curr != NULL ) {
			if( curr->owner == pid ) {
				shm_cb *tmp = curr;
				//
				// Remove the owner
				//
				if( curr == shm_bin[i] ) {
					//! remove head
					shm_bin[ i ] = curr->next;
				} else {
					prev->next = curr->next;
				}
				
				curr = curr->next;
				shm_send_event( tmp, SHM_CLOSED );
				ker_slab_free( &shm_slab, tmp );
				
			} else {
				uint8_t j;
				for( j = 0; j < SHM_NUM_WAITER; j++ ) {
					if( curr->waiter[j] == pid ) {
						curr->waiter[j] = NULL_PID;
					}
				}
				prev = curr;
				curr = curr->next;
			}
		}
	}
	return SOS_OK;
}

void shm_gc( void )
{
	uint8_t i;
	
	//
	// Mark all slab memory
	//
	for( i = 0; i < SHM_NUM_BINS; i++ ) {
		shm_cb *itr = shm_bin[i];
		
		while( itr != NULL ) {
			slab_gc_mark( &shm_slab, itr );
			itr = itr->next;
		}
	}
	
	//
	// GC slab memory
	//
	slab_gc( &shm_slab, KER_SHM_PID );
	//
	// GC the shm
	//
	malloc_gc( KER_SHM_PID );
}

int8_t shm_init( void )
{
	uint8_t i;
	
	for( i = 0; i < SHM_NUM_BINS; i++ ) {
		shm_bin[i] = NULL;
	}
	ker_slab_init( KER_SHM_PID, &shm_slab, sizeof( shm_cb ), 4, SLAB_LONGTERM );
	return SOS_OK;
}

