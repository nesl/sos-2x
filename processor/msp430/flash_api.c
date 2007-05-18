/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */

#include "flash.h"
#include <hardware.h>
#include <codemem_conf.h>


extern void* __text_end;
static void __attribute__ ((section(".data"))) flash_write_block( uint32_t addr, uint8_t* buf, uint16_t len );

uint32_t flash_init( void )
{
	return (uint32_t)( (uint32_t)((((uint16_t)&__text_end) + FLASHMEM_PAGE_SIZE - 1) / FLASHMEM_PAGE_SIZE) 
				* FLASHMEM_PAGE_SIZE );
}

/**
 * Erase the segment pointing to the addr
 */
void flash_erase( uint32_t address, uint16_t len )
{
	uint16_t a     = (uint16_t) address;
	uint16_t *addr = (uint16_t*) a;
	uint16_t l      = (len + FLASHMEM_PAGE_SIZE - 1) / FLASHMEM_PAGE_SIZE;
	HAS_CRITICAL_SECTION;
	
	while( l > 0 ) {
		ENTER_CRITICAL_SECTION();
		FCTL2 = FWKEY + FSSEL1 + FN2;   // SMCLK / 5
		FCTL3 = FWKEY;                  // Clear LOCK
		FCTL1 = FWKEY + ERASE;          // Erase segment erase
		*addr = 0;                      // dummy write to trigger erase
		FCTL3 = FWKEY + LOCK;           // Done, set LOCK
	
		LEAVE_CRITICAL_SECTION();
		l--;
		a += FLASHMEM_PAGE_SIZE;
		addr = (uint16_t*) a;
	}
}

void flash_write( uint32_t addr, uint8_t* buf, uint16_t len )
{
	uint16_t i = 0;
	//
	// HACK: assume the block is already erased and write is aligned with the block address 
	//
	if( ((addr & 0x3F) != 0) || ((len % 64) != 0)) {
		return;
	}
	while( len != 0 ) {
		flash_write_block( addr + i * 64, buf + i * 64, 64 );
		len -= 64;
		i++;
	}
}

/**
 * Write a block of data into flash (block size is 64 bytes)
 * Note: this routine has to be in the RAM.
 */
static void __attribute__ ((section(".data"))) flash_write_block( uint32_t addr, uint8_t* buf, uint16_t len )
{
	HAS_CRITICAL_SECTION;
	register uint16_t i;
	register uint16_t* d = (uint16_t*) ((uint16_t)addr);
	register uint16_t* b = (uint16_t*) buf;
	
	ENTER_CRITICAL_SECTION();
	while( FCTL3 & BUSY );
	
	FCTL2 = FWKEY + FSSEL1 + FN2;   // SMCLK / 5
	FCTL3 = FWKEY;                  // Clear LOCK
	FCTL1 = FWKEY + BLKWRT + WRT;   // Enable block write
	
	for( i = 0; i < 32; i++ ) {
		*d = *b;
		d++;
		b++;
		while( (FCTL3 & WAIT) == 0);
	}

	FCTL1 = FWKEY;                  // Clear WRT and BLKWRT
	
	while( FCTL3 & BUSY );          // Test Busy
	
	FCTL3 = FWKEY + LOCK;           // Set LOCK

	LEAVE_CRITICAL_SECTION();
}

void flash_read( uint32_t addr, void* buf, uint16_t size )
{
	register uint16_t i;
	register uint16_t* d = (uint16_t*)((uint16_t)addr);
	register uint16_t* b = buf;
	bool odd_bytes;
	
	odd_bytes = size % 2;
	if( odd_bytes ) {
		size -= 1;
	}
	for( i = 0; i < size; i+=2 ) {
			*b = *d;
			b++;
			d++;
	}
	if( odd_bytes ) {
		*((uint8_t*)b) = *((uint8_t*)d);
	}
}




