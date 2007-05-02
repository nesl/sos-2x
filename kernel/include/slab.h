
#ifndef _SLAB_H_
#define _SLAB_H_

typedef struct slab_item_t {
	struct slab_item_t *next; //!< link list
	uint8_t alloc;    //!< allocation vector
	uint8_t gc_mark;  //!< garbage collection mark
	uint8_t mem[];    //!< real memory region
} slab_item_t;

typedef struct slab_t {
	slab_item_t *head;
	uint8_t num_items_per_pool;
	uint8_t empty_vector;
	uint8_t item_size;
	uint8_t flag;
} slab_t;

#define SLAB_LONGTERM   0x80

/**
 * The slab allocator for dynamic memory allocation of fixed size blocks
 */

extern int8_t ker_slab_init( sos_pid_t pid, slab_t *slab, 
		uint8_t item_size, uint8_t items_per_pool, uint8_t flag );

extern void* ker_slab_alloc( slab_t *slab, sos_pid_t pid );

extern void ker_slab_free( slab_t *slab, void* mem );

extern void slab_gc_mark( slab_t *slab, void *mem );

extern void slab_gc( slab_t *slab, sos_pid_t pid );

#if defined(SOS_DEBUG_SLAB) && defined(PC_PLATFORM)
extern void slab_debug( slab_t *slab );
#else
#define slab_debug(...)
#endif

#endif  // #ifndef _SLAB_H_

