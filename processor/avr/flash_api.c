/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */

#include <avr/io.h>
#include <avr/eeprom.h>
#include <sos_types.h>
#include <hardware.h>
#include <codemem_conf.h>

#include "flash.h"

extern void* __text_end;

uint16_t flash_init( void )
{
	uint16_t tmp = (uint16_t)&(__text_end);
	
	if( tmp > (65536L - FLASHMEM_PAGE_SIZE) ) {
		return 256L;
	}
	if( tmp < (20L * 1024L) ) {
		return (uint16_t)( (tmp +
        (FLASHMEM_PAGE_SIZE - 1)) / FLASHMEM_PAGE_SIZE) + 256L;
	}
	return (uint16_t)( (tmp +
        (FLASHMEM_PAGE_SIZE - 1)) / FLASHMEM_PAGE_SIZE);
}

void flash_erase( uint32_t address, uint16_t len )
{
	//
	// No need to do anything as we always rewrite the entire page
	//
} 
/*
{
	uint16_t page;
	HAS_CRITICAL_SECTION;
	
	ENTER_CRITICAL_SECTION();
	//! making sure that EEPROM is not busy
	eeprom_busy_wait ();
	
	if( address >= 65536L ) {
		RAMPZ = 1;
	} else {
		RAMPZ = 0;
	}
		
	SpmCommand((uint16_t)address, (1 << PGERS) | (1 << SPMEN));
	SpmCommand(0, (1 << RWWSRE) | (1 << SPMEN));
	
	RAMPZ = 0;
	LEAVE_CRITICAL_SECTION();
}
*/
#include <led.h>

void flash_write( uint32_t address, uint8_t *data, uint16_t len )
{
	uint16_t page;
	uint16_t offset;
	uint16_t i;
	uint32_t page_address;
	HAS_CRITICAL_SECTION;
	
	//if( address < ((uint32_t)(&__text_end)) || address > FLASHMEM_SIZE) {
	//	led_red_toggle();
		// Disallow writing to kernel or bootloader
	//	return;
	//}
	ENTER_CRITICAL_SECTION();
	//! making sure that EEPROM is not busy
	eeprom_busy_wait ();
	
	//
	// For each different page
	//
	while( len > 0 ) {
		page   = (uint16_t)(address >> 8);
		offset = (uint16_t)(address & 0x000000ff);
		//
		// Load data from the page into temperary buffer
		//
		page_address = address & 0xffffff00;
		
		for( i = 0; i < offset; i+=2 ) {
			uint16_t w = pgm_read_word_far( page_address + i );
			SpmBufferFill( i, w );
		}
		//
		// Modify the content
		//
		for( ; len > 0 && i < 256L; i+=2 ) {
			uint16_t w = *data++;
			w += (*data++) << 8;
			len -= 2;
			SpmBufferFill( i, w ); 
		} 
	
		//
		// load the rest of the page
		//	
		for( ; i < 256L; i+=2 ) {
			uint16_t w = pgm_read_word_far( page_address + i );
			SpmBufferFill( i, w );	
		}
	
		//
		// Write it back
		//
		if( page >= 256L ) {
			RAMPZ = 1;
		} else {
			RAMPZ = 0;
		}
		
		//
		// Prepare the page address
		//
		page <<= 8;
		
		SpmCommand(page, (1 << PGERS) | (1 << SPMEN));
		SpmCommand(page, (1 << PGWRT) | (1 << SPMEN)); 
		SpmCommand(0, (1 << RWWSRE) | (1 << SPMEN));
		
		address += (256L - offset);
	}
	RAMPZ = 0;
	LEAVE_CRITICAL_SECTION();
}

void flash_read( uint32_t address, void *buf, uint16_t size )
{
	uint16_t i = 0;
	if( (address % 2) == 0 && (size % 2) == 0 ) {
		uint16_t *wp = (uint16_t*)buf;
		for(i = 0; i < size; i+=2){
			wp[i/2] = pgm_read_word_far(address + i);
		}
	} else {
		uint8_t *data = (uint8_t*)buf;
		for(i = 0; i < size; i+=1){
			data[i] = pgm_read_byte_far(address + i);
		}
	}
}



