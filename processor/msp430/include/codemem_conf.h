
#ifndef _CODEMEM_CONF_H
#define _CODEMEM_CONF_H
#include <sos_inttypes.h>
//! the type used to represent code space address
//! since the maximum size of a module is 4K, we only need 16 bits
typedef uint32_t code_addr_t;

//! the type used to represent the page number in the flash
typedef uint32_t code_page_t;

enum {
	//! underlying flash page size (bytes)
	FLASH_PAGE_SIZE = 512L,
	//! starting address of loadable module (bytes)
	CODEMEM_START_ADDR = FLASH_PAGE_SIZE * 192L, 
	//! starting page number that can be used for loadable module
	CODEMEM_START_PAGE = 192L,
	//! last page number that ca be used for loadable module
	CODEMEM_END_PAGE = 250L,
	//! the size of code memory that can be used for loadable module
	CODEMEM_SIZE   = FLASH_PAGE_SIZE * (CODEMEM_END_PAGE+1) - CODEMEM_START_ADDR,
	//! maximum range of relative addressing (bytes)
	MAX_RANGE_REL_ADDRESSING_PAGES = 16L,
	MAX_RANGE_REL_ADDRESSING = FLASH_PAGE_SIZE * MAX_RANGE_REL_ADDRESSING_PAGES,
	//! code space addressing moode
	CODEMEM_ADDRESSING_BYTES = 1L,
	FLASHMEM_SIZE            = (47L * 1024L),
	FLASHMEM_PAGE_SIZE       = 512L,
	NUM_COMPILED_MODULES     = 16,
	CODEMEM_MAX_LOADABLE_MODULES = 16,
};

#endif
