
#ifndef _MALLOC_CONF_H_
#define _MALLOC_CONF_H_

/**
 * General Purpose Malloc Configuration
 */
//-----------------------------------------------------------------------------
// User parameters to set.
//
#define BLOCK_SIZE           8               // A block size (must be power of 2) (Platform specific)
#define SHIFT_VALUE          3               // Must equal log(BLOCK_SIZE)/log(2)
//#define MALLOC_HEAP_SIZE   7168               // MSP430 has 7 KB of RAM
#define MALLOC_HEAP_SIZE    5120               // MSP430 has 5 KB of RAM
#define SOS_HEAP_SECTION
#endif
