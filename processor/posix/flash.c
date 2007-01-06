#include <hardware.h>

#define KB			1024

#ifdef EMU_MICA2
#define FLASH_SIZE	128L		// size in KB
#define PAGE_SIZE	256L		// size in bytes
//! here is the flash
static uint8_t flash_image_buf[FLASH_SIZE * KB];
uint16_t pgm_read_word_far(uint32_t addr) { return (flash_image_buf[addr + 1] << 8) | flash_image_buf[addr]; }
uint16_t pgm_read_word(uint32_t addr) { return pgm_read_word_far(addr); }
uint8_t pgm_read_byte(uint32_t addr) { return flash_image_buf[addr]; }
#endif

#ifdef EMU_XYZ
#define FLASH_SIZE	256		// size in KB
#define PAGE_SIZE	2048	// size in bytes
//! here is the flash
static uint8_t flash_image_buf[FLASH_SIZE * KB];
uint32_t pgm_read_word_far(uint32_t addr) { return (flash_image_buf[addr + 3] << 24) | (flash_image_buf[addr + 2] << 16) | (flash_image_buf[addr + 1] << 8) | flash_image_buf[addr]; }		// proper endianess ?
uint32_t pgm_read_word(uint32_t addr) { return pgm_read_word_far(addr); }
uint8_t pgm_read_byte(uint32_t addr) { return flash_image_buf[addr]; }
#endif

#ifndef SOS_DEBUG_FLASH
#undef DEBUG
#define DEBUG(...)
#endif

uint16_t flash_init( void )
{
	uint32_t i;
	DEBUG("flash_init\n");
	for(i = 0; i < FLASH_SIZE * KB; i++) {
		flash_image_buf[i] = 0xff;
	}
	return 1;
}

/*
static void dump_flash_image(int32_t start, int32_t end)
{
	uint32_t i;
	printf("image from %d to %d\n", start, end);
	for(i = start; i < end; i++){
		if(i % 16 == 0) printf("\n");
		if(flash_image_buf[i] < 16){
			printf("0%X", flash_image_buf[i]);
		} else {
			printf("%X", flash_image_buf[i]);
		}
	}
	printf("\n");
}


static void dump_flash_image()
{
	uint16_t i;
	DEBUG("dump: \n");
	for( i = 256; i < 512; i++ ) {
		DEBUG_SHORT("%x ", flash_image_buf[i]);
	}
	DEBUG_SHORT("\n");
}
*/

void flash_write( uint32_t address, uint8_t *data, uint16_t len )
{
	uint16_t i;
	
	DEBUG("***Flash buffer write addr = %u, length = %hu\n", address, len);
	//dump_flash_image();
	for( i = 0; i < len; i++ ) {
		//DEBUG_SHORT("%x ", data[i]);
		flash_image_buf[address+i] = data[i];
	}
	//DEBUG_SHORT("\n ");
	//dump_flash_image();
}

void flash_read( uint32_t address, void *buf, uint16_t size )
{
	uint16_t i;
	uint8_t *data = buf;
	
	DEBUG("***Flash buffer read addr = %u, len = %hu\n", address, size);
	//dump_flash_image();
	for( i = 0; i < size; i++ ) {
		data[i] = flash_image_buf[address+i];
		//DEBUG_SHORT("%x ", data[i]);
	}
	//DEBUG_SHORT("\n ");
}

void flash_erase( uint32_t address, uint16_t len )
{
	//
	// No need to do anything as we always rewrite the entire page
	//
} 

void *FlashGetProgmem(uint32_t addr)
{
	//DEBUG("Get Progmem %d\n", addr);
	return &(flash_image_buf[addr]);
}

