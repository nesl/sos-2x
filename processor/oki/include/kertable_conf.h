
#ifndef _KERTABLE_CONF_H
#define _KERTABLE_CONF_H

#include "pgmspace.h"

#define get_kertable_entry(k)  pgm_read_word(0x00000140+4*(k))

#endif

