#ifndef _FLASH_H
#define _FLASH_H

#include <sos.h>
#include <sos_types.h>

uint16_t flash_init( void );
void flash_erase( uint32_t address, uint16_t len );
void flash_write( uint32_t addr, uint8_t* buf, uint16_t len );
void flash_read( uint32_t addr, void* buf, uint16_t size );

#define FlashGetProgmem(x) x


#endif

