

#include <sos.h>
#include <malloc.h>
#include <slab.h>

#ifndef SOS_DEBUG_SLAB
#undef DEBUG
#define DEBUG(...)
#undef DEBUG_SHORT
#define DEBUG_SHORT(...)
#endif

#ifdef SOS_DEBUG_GC
#define DEBUG_GC(arg...)  printf(arg)
#else
#define DEBUG_GC(...)
#endif


int8_t ker_slab_init( sos_pid_t pid, slab_t *slab, uint8_t item_size, uint8_t items_per_pool, uint8_t flag )
{
	uint8_t i;
	if( items_per_pool > 8 ) {
		return -EINVAL;
	}
	
	//
	// build empty vector
	//
	slab->head = NULL;
	slab->num_items_per_pool = items_per_pool;
	slab->empty_vector = 0;
	slab->item_size = item_size;
	slab->flag = flag;
	for( i = 0; i < items_per_pool; i++ ) {
		slab->empty_vector <<= 1;
		slab->empty_vector |= 0x01;
	}
	
	if( slab->flag & SLAB_LONGTERM ) {
		slab->head = malloc_longterm( sizeof( slab_item_t ) + items_per_pool * item_size, pid );
	} else {
		slab->head = ker_malloc( sizeof( slab_item_t ) + items_per_pool * item_size, pid );
	}
	
	if( slab->head == NULL ) {
		return -ENOMEM;
	}
	slab->head->next = NULL;
	slab->head->alloc = 0;
	slab->head->gc_mark = 0;
	return SOS_OK;
}

void* ker_slab_alloc( slab_t *slab, sos_pid_t pid )
{
	slab_item_t *itr = slab->head;
	slab_item_t *prev = slab->head;
	
	
	while( itr != NULL ) {
		DEBUG(" itr->alloc = %x\n", itr->alloc);
		if( itr->alloc != slab->empty_vector ) {
			break;
		}
		prev = itr;
		itr = itr->next;
	}
	
	if( itr == NULL ) {
		//
		// The pool is exhausted, create a new one
		//
		DEBUG("pool exhausted\n");
		if( slab->flag & SLAB_LONGTERM ) {
			prev->next = malloc_longterm( sizeof( slab_item_t ) + slab->num_items_per_pool * slab->item_size, pid );
		} else {
			prev->next = ker_malloc( sizeof( slab_item_t ) + slab->num_items_per_pool * slab->item_size, pid );
		}
		if( prev->next == NULL ) {
			DEBUG("alloc NULL\n");
			return NULL;
		}
		itr = prev->next;
		itr->next = NULL;
		itr->alloc = 0x01;
		itr->gc_mark = 0;
		return itr->mem;
	} else {
		uint8_t i;
		uint8_t mask = 0x01;
		
		DEBUG("find free slot in pool\n");
		for( i = 0; i < slab->num_items_per_pool; i++, mask<<=1 ) {
			if( (itr->alloc & mask)  == 0 ) {
				itr->alloc |= mask;
				return itr->mem + (i * slab->item_size);
			}
		}
	}
	return NULL;
}

void ker_slab_free( slab_t *slab, void* mem ) 
{
	slab_item_t *itr = slab->head;
	slab_item_t *prev = NULL;
	
	while( itr != NULL ) {
		if( ((uint8_t*)mem) >= itr->mem && ((uint8_t*)mem) < (itr->mem + slab->num_items_per_pool * slab->item_size) ) {
			uint8_t mask = 1 << ( ( ((uint8_t*)mem) - (itr->mem) ) / slab->item_size );
			
			itr->alloc &= ~mask;
			
			if( itr->alloc == 0 && itr != slab->head ) {
				prev->next = itr->next;
				ker_free( itr );
			}
			return;
		}
		prev = itr;
		itr = itr->next;
	}
	ker_panic();
}

void slab_gc_mark( slab_t *slab, void *mem )
{
	slab_item_t *itr = slab->head;
	
	DEBUG_GC("slab: gc_mark\n");
	while( itr != NULL ) {
		if( ((uint8_t*)mem) >= itr->mem && ((uint8_t*)mem) < (itr->mem + slab->num_items_per_pool * slab->item_size) ) {
			if( (((uint8_t*)mem) - (itr->mem)) % slab->item_size == 0 ) {
				uint8_t mask = 1 << ( ( ((uint8_t*)mem) - (itr->mem) ) / slab->item_size );
				if( itr->alloc & mask ) {
					DEBUG_GC("slab mark: %d\n", (int) mem );
					itr->gc_mark |= mask;
				} else {
					DEBUG_GC("slab: used already freed memory %d\n", (int) mem );
					// use already freed memory?
					//itr->alloc |= mask;
					//itr->gc_mark |= mask;
				}
			} else {
				DEBUG_GC("slab: memory misalignment %d\n", (int) mem );
				// TODO: misalignment?  
			}
			return;
		}
		itr = itr->next;
	}
	DEBUG_GC("cannot find memory %d\n", (int) mem);
	//exit(1);
}
#include <led.h>
void slab_gc( slab_t *slab, sos_pid_t pid )
{
	slab_item_t *itr = slab->head;
	slab_item_t *prev = NULL;
	//
	// Detect memory leak while marking the memory to malloc
	//
	while( itr != NULL ) {
		if( itr->alloc != itr->gc_mark ) {
			// leak!
			DEBUG_GC("leak in slab %d %d\n", itr->alloc, itr->gc_mark);
			led_red_toggle();
			itr->alloc = itr->gc_mark;
			
			if( itr->alloc == 0 && itr != slab->head ) {
				prev->next = itr->next;
				ker_free( itr );
				itr = prev;
			} else {
				// mark it used to malloc
				itr->gc_mark = 0;
				ker_gc_mark( pid, itr );
			}
		} else {
			itr->gc_mark = 0;
			ker_gc_mark( pid, itr );
		}
		prev = itr;
		itr = itr->next;
	}
}

#if defined(PC_PLATFORM) && defined(SOS_DEBUG_SLAB)
void slab_debug( slab_t *slab )
{
	uint8_t cnt = 0;
	slab_item_t *itr = slab->head;
	DEBUG("slab size = %d, num item per pool = %d, empty vector = %x\n", slab->item_size, slab->num_items_per_pool, slab->empty_vector);
	
	while( itr != NULL ) {
		uint8_t i;
		uint8_t mask = 0x01;
		
		for( i = 0; i < slab->num_items_per_pool; i++, mask<<=1 ) {
			if( itr->alloc & mask ) {
				uint8_t *m = itr->mem;
				uint8_t j;
				
				m += (i * slab->item_size);
				DEBUG_SHORT("addr = %d: ", (int)m);
				
				for( j = 0; j < slab->item_size; j++ ) {
					DEBUG_SHORT("%d ", m[j]);
				}
				DEBUG_SHORT("\n");
			}
		}
		cnt++;
		itr = itr->next;
	}
	DEBUG("number of pools = %d\n", cnt);
}
#endif


