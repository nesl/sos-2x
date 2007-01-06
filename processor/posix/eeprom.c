#include <sos.h>
#include <sos_types.h>

static uint8_t estore[4096];

uint8_t eeprom_read_byte (const uint8_t *addr)
{
	int a = (int) addr;
	if(a >= 4096) return 0;
	return estore[a];
}

uint16_t eeprom_read_word (const uint16_t *addr)
{
	int a = (int) addr;
	uint16_t r;
	if(a >= 4096) return 0;
	r = estore[a] | estore[a+1] << 8;
	return r;
}

void eeprom_read_block (void *buf, const void *addr, size_t n)
{
	int a = (int) addr;
	int i;
	uint8_t *c = (uint8_t*)buf;
	if((a + n) >= 4096) return;
	for(i = 0; i < n; i++) {
		c[i] = estore[a+i];
	}
}

void eeprom_write_byte (uint8_t *addr, uint8_t val)
{
	int a = (int) addr;
	if(a >= 4096) return;
	estore[a] = val;
}

void eeprom_write_word (uint16_t *addr, uint16_t val)
{
	int a = (int) addr;
	if((a + 1) >= 4096) return;
	estore[a] = (uint8_t)val;
	estore[a+1] = (uint8_t) (val >> 8);
}

void eeprom_write_block (const void *buf, void *addr, size_t n)
{
	int a = (int) addr;
	int i;
	uint8_t *c = (uint8_t*)buf;
	if((a + n) >= 4096) return;
	for(i = 0; i < n; i++) {
		estore[a+i] = c[i];
	}
}

