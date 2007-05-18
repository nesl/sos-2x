/**
 *  Routines for managing code space
 * 
 *  In most embedded system, code space is different from data space
 *  Code space has to be physically contineous 
 *  Writing to code space is typically constrainted by underlying Flash 
 *  Layout
 *
 *  This component keeps track of the status of underlying code space
 *  It also talks to sos_linker to fix the address
 *  It provides the storage for code
 *  It keeps track of the offset of code
 *  
 *  To use this component, protocol must first request a block
 *  
 */ 
#include <hardware.h>
#include <sos_inttypes.h>
#include <pid.h>
#include <flash.h>
#include <codemem.h>
#include <malloc.h>
#include <fntable.h>
#include <string.h>
#include <sos_sched.h>
#include <random.h>
#include <sos_logging.h>
#ifdef SOS_HAS_EXFLASH
#include <exflash.h>
#endif

#ifdef SOS_SIM
#include <sim_interface.h>
#endif

#ifndef SOS_DEBUG_CODEMEM
#undef DEBUG
#define DEBUG(...)
#endif

#include <melfloader.h>

#define CODEMEM_EXECUTABLE_FLAG  0x01

/**
 * Internal data structure for opened flash space
 */
typedef struct codemem_hdr_t {
	uint32_t start_addr;          //!< starting address in the flash
	uint16_t size;                //!< size of the allocation
	uint8_t  salt;                //!< salt number to prevent 
	uint8_t  flag;  
} codemem_hdr_t;

//
// flash_start_page_addr is used to compute the address 
//
static uint32_t flash_start_page_addr;
static uint8_t* flash_alloc_bitmap;
static uint8_t  flash_bitmap_length;
static uint16_t flash_num_pages;
static codemem_hdr_t* codemem_handle_list[CODEMEM_MAX_LOADABLE_MODULES];
static uint8_t  codemem_salt;
static mod_header_ptr compiled_modules[NUM_COMPILED_MODULES] = { 0 };
static uint8_t compiled_header_ptr = 0;
//
// Cache to reduce number of flash writes
//
static uint8_t* flash_cache_page;      // A FLASHMEM_PAGE_SIZE worth of data
static uint32_t flash_cache_addr;      // the starting address of the page

// ================================================================================
// Internal Helper Routines
//
static bool check_codemem_t( codemem_t cm )
{
	uint8_t real_h = (uint8_t)( cm & 0x00ff );
	uint8_t salt   = (uint8_t)( cm >> 8 );
	
	if( real_h > CODEMEM_MAX_LOADABLE_MODULES ) {
		DEBUG("check_codemem_t: CODEMEM_MAX_LOADABLE_MODULES\n");
		return false;
	}
	
	if( codemem_handle_list[ real_h ] == NULL ) {
		DEBUG("check_codemem_t: codemem_handle_list[ real_h ] == NULL\n");
		return false;
	}
	
	if( (codemem_handle_list[ real_h ]->salt) != salt ) {
		DEBUG("codemem_handle_list[ real_h ]->salt) != salt\n");
		return false;
	}
	return true;
}

static void codemem_do_killall( codemem_t h )
{
  mod_header_ptr p;
  sos_code_id_t  cid;
	
  p = ker_codemem_get_header_address( h );
  if( p == 0 ) return;
  cid = sos_read_header_word(p, offsetof(mod_header_t, code_id));
  cid = entohs(cid);
  ker_killall(cid);

#ifdef SOS_SIM
  //
  // Close the file in simulation
  //
  delete_module_image( cid );
#endif
}


static mod_header_ptr match_cid(uint32_t addr, sos_code_id_t cid)
{
  mod_header_ptr modptr;
  sos_code_id_t  mod_cid;

  modptr = (mod_header_ptr)FlashGetProgmem( addr );
  mod_cid = sos_read_header_word(modptr, offsetof(mod_header_t, code_id));
  mod_cid = entohs(mod_cid);

  if(mod_cid == cid) {
    return modptr;
  }
  return 0;

}

static void flash_setbitmap(uint16_t start, uint16_t length, bool val)
{
	uint8_t i;
	uint8_t j;
	register uint8_t shift = 1;
	
	i = start / 8;
	j = start % 8;
	
	shift = 1 << j;
	
	for( ; j < 8 && length > 0; j++, shift <<= 1, length-- ) {
		if( val ) {
			flash_alloc_bitmap[i] |= shift;
		} else {
			flash_alloc_bitmap[i] &= ~shift;
		}
	}
	i++;
	
	for( ; i < flash_bitmap_length && length > 0; i++ ) {
		shift = 1;
		for( j = 0; j < 8 && length > 0; j++, shift <<= 1, length-- ) {
			if( val ) {
				flash_alloc_bitmap[i] |= shift;
			} else {
				flash_alloc_bitmap[i] &= ~shift;
			}
		}
	}
}

static int8_t codemem_cache_alloc( void )
{	
	if( flash_cache_page == NULL ) {
		flash_cache_page = ker_malloc( FLASHMEM_PAGE_SIZE, KER_CODEMEM_PID );
		if( flash_cache_page == NULL ) {
			return -ENOMEM;
		}
		flash_cache_addr = 0;
	}
	return SOS_OK;
}

static inline void codemem_cache_flush( )
{
	if( flash_cache_page != NULL ) {
		flash_erase( flash_cache_addr, FLASHMEM_PAGE_SIZE );
		flash_write( flash_cache_addr, flash_cache_page, FLASHMEM_PAGE_SIZE );
		ker_free( flash_cache_page );
		flash_cache_page = NULL;
		flash_cache_addr = 0;
	}
}


//
// Get the starting page address.  If the cache does not exist, allocate it.  
// If the page address is the same as 
// flash_cache_addr, write to the cache.  If the page address is different 
// from flash_cache_addr, perform the following.
// 1. store current page
// 2. load new page
// 3. save the content
// WARNING: this rountine does not handle writes across page boundary.
// WARNING: this rountine assumes the cache is already allocated.
//
static void codemem_cache_write( uint32_t addr, uint8_t* buf, uint16_t nbytes )
{
	uint16_t i;
	uint32_t start_addr = addr & ~((uint32_t)(FLASHMEM_PAGE_SIZE - 1));
	uint16_t offset = addr % FLASHMEM_PAGE_SIZE;
	
	if( flash_cache_addr != start_addr ) {
		if( flash_cache_addr != 0 ) {
			flash_erase( flash_cache_addr, FLASHMEM_PAGE_SIZE );
			flash_write( flash_cache_addr, flash_cache_page, FLASHMEM_PAGE_SIZE );
		}
		flash_cache_addr = start_addr;
		flash_read( start_addr, flash_cache_page, FLASHMEM_PAGE_SIZE );
	}
	
	for( i = 0; i < nbytes; i++, offset++ ) {
		flash_cache_page[offset] = buf[i];
	}
}

static void codemem_cache_read( uint32_t addr, uint8_t* buf, uint16_t nbytes )
{
	uint16_t i = 0;
	uint32_t start_addr = addr & ~((uint32_t)(FLASHMEM_PAGE_SIZE - 1));
	uint16_t offset = addr % FLASHMEM_PAGE_SIZE;
	
	if( flash_cache_addr == start_addr ) {
		uint16_t tmp = (FLASHMEM_PAGE_SIZE - offset);
		for( i = 0; (i < tmp) && (nbytes != 0); i++, offset++ ) {
			*buf = flash_cache_page[offset];
			buf++;
			nbytes--;
			addr++;
		}
	}
	if( nbytes == 0 ) {
		return;
	}
	flash_read( addr, buf, nbytes );
	return;
}

//
// Allocate flash memory according to the size
// \return the address to the flash
// \return zero for failure
//
static uint32_t flash_alloc( uint16_t size )
{
	uint8_t i, j;
	uint8_t num_blocks;
	uint8_t free_blocks = 0; 
	uint32_t addr;
	
	if( codemem_cache_alloc() != SOS_OK ) {
		return -ENOMEM;
	}
	
	num_blocks = (uint8_t)((size + (FLASHMEM_PAGE_SIZE - 1)) / FLASHMEM_PAGE_SIZE);
	
	//
	// Address-ordered first fit
	// Search from the beginning and find 
	//
	for( i = 0; i < flash_bitmap_length; i++ ) {
		register uint8_t shift = 1;
		register uint8_t tmp = flash_alloc_bitmap[i];
		for( j = 0; j < 8; j++, shift <<= 1 ) {
			if( tmp & shift ) {
				free_blocks = 0;
			} else {
				free_blocks++;
			}
			if( free_blocks == num_blocks ) {
				//
				// set the bit map
				//
				flash_setbitmap(i*8 + j + 1 - num_blocks, num_blocks, true);
				addr = flash_start_page_addr + ((i*8 + j + 1 - num_blocks) * FLASHMEM_PAGE_SIZE);
				return addr;
			}
		}
	}
	return 0;
}

static void flash_free( uint32_t addr, uint16_t size )
{
	uint16_t b = (addr - flash_start_page_addr) / FLASHMEM_PAGE_SIZE;
	uint8_t num_blocks = (uint8_t)((size + FLASHMEM_PAGE_SIZE - 1) / FLASHMEM_PAGE_SIZE);
	 
	//
	// Unset the bit map
	//
	flash_setbitmap(b, num_blocks, false);
	flash_erase( addr, size );
}


// ================================================================================
// Public Routines
//
codemem_t ker_codemem_alloc(uint16_t size, codemem_type_t type)
{
	uint8_t i;
	codemem_hdr_t *hdr;
	codemem_t      ret;
	//
	// Allocate meta data  
	//
	
	for( i = 0; i < CODEMEM_MAX_LOADABLE_MODULES; i++ ) {
		if( codemem_handle_list[i] == NULL ) {
			break;
		}
	}
	
	if( i == CODEMEM_MAX_LOADABLE_MODULES ) {
		//
		// Maximum file reached...
		//
		return CODEMEM_INVALID;
	}
	
	hdr = malloc_longterm( sizeof(codemem_hdr_t), KER_CODEMEM_PID );
	
	if( hdr == NULL ) {
		return CODEMEM_INVALID;
	}
	
	hdr->start_addr = flash_alloc( size );

	//post_uart(KER_CODEMEM_PID, KER_CODEMEM_PID, 100, 4, &(hdr->start_addr), 0, BCAST_ADDRESS);
	if( hdr->start_addr == 0 ) {
		ker_free( hdr );
		return CODEMEM_INVALID;
	}
	hdr->size = size;
	codemem_handle_list[i] = hdr;
	
	codemem_salt++;
	hdr->salt = codemem_salt;
	hdr->flag = 0;
	ret = ((uint16_t)i) | (((uint16_t)codemem_salt) << 8);
	//flash_erase( hdr->start_addr , size );

	ker_log( SOS_LOG_CMEM_ALLOC, ker_get_current_pid(), size );	
	return ret;
}

int8_t ker_codemem_write(codemem_t h, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset)
{
	uint8_t cmt = (uint8_t)(h & 0x00ff);
	codemem_hdr_t *hdr;
	
	DEBUG("ker_codemem_write: h = 0x%x nbytes = %d, offset = %d\n", h, nbytes, offset);
	if( check_codemem_t( h ) == false ) {
		return -ENOENT;
	}
	
	hdr = codemem_handle_list[cmt];
	DEBUG("ker_codemem_write: hdr = 0x%p \n", hdr);

	return ker_codemem_direct_write(hdr->start_addr, pid, buf, nbytes, offset);

}	

int8_t ker_codemem_direct_write(uint32_t start_addr, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset)
{
	uint8_t *b = buf;
	uint16_t size_written;
	uint16_t remaining_size;
	
	if( codemem_cache_alloc() != SOS_OK ) {
		return -ENOMEM;
	}
	
	DEBUG("ker_codemem_write: start_addr = 0x%x nbytes = %d, offset = %d\n", start_addr, nbytes, offset);

	start_addr += offset;
	size_written = 0;
	//
	// Remaining size in the page
	// FLASHMEM_PAGE_SIZE - (start_addr % FLASHMEM_PAGE_SIZE) 
	//
	remaining_size = FLASHMEM_PAGE_SIZE - (start_addr % FLASHMEM_PAGE_SIZE);
	if( remaining_size < nbytes ) {
		codemem_cache_write( start_addr, b, remaining_size );
		size_written = remaining_size;
		start_addr += remaining_size;
		b += remaining_size;
	}  
	
	while( 1 ) {
		if( (nbytes - size_written) > FLASHMEM_PAGE_SIZE ) {
			codemem_cache_write( start_addr, b, FLASHMEM_PAGE_SIZE );
			size_written +=  FLASHMEM_PAGE_SIZE;
			start_addr += FLASHMEM_PAGE_SIZE;
			b += FLASHMEM_PAGE_SIZE;
		} else {
			if( (nbytes - size_written) != 0 ) {
				codemem_cache_write( start_addr, b, nbytes - size_written );
			}
			break;
		}
	}

	ker_log( SOS_LOG_CMEM_WRITE, pid, nbytes );	
	return SOS_OK;
}	

int8_t ker_codemem_read(codemem_t h, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset) {
	uint8_t cmt = (uint8_t)(h & 0x00ff);
	codemem_hdr_t *hdr; 

	if( check_codemem_t( h ) == false ) {
			return -ENOENT;
	}

	hdr = codemem_handle_list[cmt];

	return ker_codemem_direct_read(hdr->start_addr, pid, buf, nbytes, offset);
}


int8_t ker_codemem_direct_read(uint32_t start_addr, sos_pid_t pid, void *buf, uint16_t nbytes, uint16_t offset)
{
	codemem_cache_read( start_addr + offset, buf, nbytes );

	ker_log( SOS_LOG_CMEM_READ, pid, nbytes );	
	return SOS_OK;
}


int8_t ker_codemem_free(codemem_t h)
{
	uint8_t cmt = (uint8_t)(h & 0x00ff);
	codemem_hdr_t *hdr;
	
	if( check_codemem_t( h ) == false ) {
		return -ENOENT;
	}
	
	hdr = codemem_handle_list[cmt];
	
	if( hdr->flag & CODEMEM_EXECUTABLE_FLAG ) {
		codemem_do_killall(h);
	}
	
	flash_free( hdr->start_addr, hdr->size );
	
	codemem_handle_list[cmt] = NULL;
	
	ker_log( SOS_LOG_CMEM_FREE, ker_get_current_pid(), hdr->size );	
	ker_free( hdr );
	return SOS_OK;
}

//
// 
//
int8_t ker_codemem_flush(codemem_t h, sos_pid_t pid)
{
	codemem_cache_flush();
	return SOS_OK;
}


int8_t codemem_register_module( mod_header_ptr h )
{
  if (compiled_header_ptr >= NUM_COMPILED_MODULES) return -EINVAL;

  compiled_modules[compiled_header_ptr++] = h;

  return SOS_OK;
}

mod_header_ptr ker_codemem_get_header_from_code_id( sos_code_id_t cid )
{
  uint8_t i;
  mod_header_ptr ret;
  
  for( i = 0; i < CODEMEM_MAX_LOADABLE_MODULES; i++ ) {
	if( (codemem_handle_list[i] != NULL) && 
		(codemem_handle_list[i]->flag & CODEMEM_EXECUTABLE_FLAG) ) {
		ret = match_cid( codemem_handle_list[i]->start_addr, cid ); 
		if( ret != 0 ) {
#ifndef SOS_SIM
			return ret;
#else
			return get_header_from_sim( cid );
#endif
      }
	}
  }
  	
  for (i = 0; i < compiled_header_ptr; i++) {
    sos_code_id_t code_id;
    ret = compiled_modules[i];
    code_id = sos_read_header_word(ret, offsetof(mod_header_t, code_id));
    code_id = entohs( code_id );
    if(cid == code_id){
      return ret;
    }
  }
	
  return 0;
}

int8_t ker_codemem_mark_executable(codemem_t h)
{
	uint8_t cmt = (uint8_t)(h & 0x00ff);
	codemem_hdr_t *hdr;
	
	if( check_codemem_t( h ) == false ) {
		return -ENOENT;
	}
	
	hdr = codemem_handle_list[cmt];
	hdr->flag |= CODEMEM_EXECUTABLE_FLAG;
	return SOS_OK;
}

uint32_t ker_codemem_get_start_address( codemem_t h )
{
	uint8_t cmt = (uint8_t)(h & 0x00ff);
	codemem_hdr_t *hdr;
	
	if( check_codemem_t( h ) == false ) {
		return (uint32_t)0;
	}

	hdr = codemem_handle_list[cmt];
	return hdr->start_addr;	
}

mod_header_ptr ker_codemem_get_header_address( codemem_t h)
{
#ifndef SOS_SIM
	return melf_get_header_address( h );
#else
	mod_header_ptr p = melf_get_header_address( h );

	sos_code_id_t cid =                                         
		      sos_read_header_word( p, offsetof(mod_header_t, code_id) );
	cid = entohs(cid);                                          
	return get_header_from_sim( cid );        
#endif
}

void codemem_init(void)
{
	uint8_t i;
	
	//
	// Compute the starting page for programming
	//
	flash_start_page_addr = flash_init();

#ifndef PC_PLATFORM
	flash_num_pages = (FLASHMEM_SIZE - flash_start_page_addr) / FLASHMEM_PAGE_SIZE;
#else
	flash_num_pages = FLASHMEM_SIZE / FLASHMEM_PAGE_SIZE;
#endif

	flash_bitmap_length = (uint8_t)((flash_num_pages + 7) / 8);
	//
	// Allocate memory for bitmap
	//
	flash_alloc_bitmap = malloc_longterm( flash_bitmap_length, KER_CODEMEM_PID );
	
	//
	// Initialize bitmap
	//
	for( i = 0; i < flash_bitmap_length; i++ ) {
		flash_alloc_bitmap[i] = 0;
	}
	
	for( i = 0; i < CODEMEM_MAX_LOADABLE_MODULES; i++ ) {
		codemem_handle_list[i] = NULL;
	}
	
	codemem_salt = 0;
	
	flash_cache_page = NULL;
	flash_cache_addr = 0;
}


int8_t ker_sys_codemem_read(codemem_t h, void *buf, uint16_t nbytes, uint16_t offset)
{
  sos_pid_t my_pid = ker_get_current_pid();
  return ker_codemem_read(h, my_pid, buf, nbytes, offset);
}
