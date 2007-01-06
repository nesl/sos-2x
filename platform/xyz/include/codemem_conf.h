/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _CODEMEM_CONF_H
#define _CODEMEM_CONF_H
#include <sos_inttypes.h>
//! the type used to represent code space address
//! since the maximum size of a module is 4K, we only need 16 bits
typedef uint32_t code_addr_t;

//! the type used to represent the page number in the flash
typedef uint16_t code_page_t;

enum {
	//! underlying flash page size (bytes)
	FLASH_PAGE_SIZE = 2048L,
	//! starting page number that can be used for loadable module
	CODEMEM_START_PAGE = 60L,
	//! last page number that can be used for loadable module
	CODEMEM_END_PAGE = 96L,
	//! starting address of loadable module (bytes)
	CODEMEM_START_ADDR = FLASH_PAGE_SIZE * CODEMEM_START_PAGE,
	//! the size of code memory that can be used for loadable module
	CODEMEM_SIZE   = FLASH_PAGE_SIZE * (CODEMEM_END_PAGE+1) - CODEMEM_START_ADDR,
	//! maximum range of relative addressing (bytes)
	MAX_RANGE_REL_ADDRESSING_PAGES = 2L,
	MAX_RANGE_REL_ADDRESSING = FLASH_PAGE_SIZE * MAX_RANGE_REL_ADDRESSING_PAGES,
	//! code space addressing mode
	//! AVR uses word address instead of byte address in the code space
	//! we use 2 bytes to model this.
	CODEMEM_ADDRESSING_BYTES = 1L,
	FLASHMEM_SIZE            = (480L * 256L),
	FLASHMEM_PAGE_SIZE       = 256L,
	NUM_COMPILED_MODULES     = 16,
	CODEMEM_MAX_LOADABLE_MODULES = 16,
};

#endif
