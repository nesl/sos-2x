
#ifndef _KERTABLE_CONF_H
#define _KERTABLE_CONF_H
#include <hardware_proc.h>

#define KERTABLE_OFFSET 0x10C
#define KERTABLE_ENTRY_SIZE 2

#define get_kertable_entry(k)  pgm_read_word(KERTABLE_OFFSET+KERTABLE_ENTRY_SIZE*(k))

#endif

