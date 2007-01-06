
#ifndef _EEPROM_H_
#define _EEPROM_H_

extern uint8_t eeprom_read_byte (const uint8_t *addr);

extern uint16_t eeprom_read_word (const uint16_t *addr);

extern void eeprom_read_block (void *buf, const void *addr, size_t n);

extern void eeprom_write_byte (uint8_t *addr, uint8_t val);

extern void eeprom_write_word (uint16_t *addr, uint16_t val);

extern void eeprom_write_block (const void *buf, void *addr, size_t n);


#endif
