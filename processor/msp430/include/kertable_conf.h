
#ifndef _KERTABLE_CONF_H
#define _KERTABLE_CONF_H
#include <hardware_proc.h>

#define get_kertable_entry(k)  (*(uint16_t*)(0x4000 + 2 * (k)))
//pgm_read_word(0x8c+2*(k))

#endif

