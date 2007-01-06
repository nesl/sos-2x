/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */

#ifndef _SOS_HARDWARE_TYPES_H_
#define _SOS_HARDWARE_TYPES_H_
#include <hardware_proc.h>

typedef struct spi_addr {
		uint8_t cs_reg; 
		uint8_t cs_bit; 
		uint8_t cs_flags; 
} spi_addr_t;

#endif

