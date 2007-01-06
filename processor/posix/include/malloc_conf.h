#ifndef _MALLOC_CONF_H_ 
#define _MALLOC_CONF_H_


/**
 * General Purpose Malloc Configuration
 */
//-----------------------------------------------------------------------------
// User parameters to set.
// NOTE: minimal block size is 8
#define BLOCK_SIZE          16               // A block size (must be power of 2) (Platform specific)
#define SHIFT_VALUE          4               // Must equal log(BLOCK_SIZE)/log(2)
#define MALLOC_HEAP_SIZE  3072               // 3 KB

//-----------------------------------------------------------------------------
// HEAP LAYOUT
//-----------------------------------------------------------------------------
#ifdef SOS_SFI
#define SOS_HEAP_SECTION __attribute__((aligned(16))); // Heap section needs to be aligned at the block boundary for SFI to work
#else
#define SOS_HEAP_SECTION
#endif



/**
 * Define the maximum domain boundaries for fault tolerant mode
 */
#ifdef FAULT_TOLERANT_SOS
// Block handle IDs uptill which a domain can extend
#define TI_KER_DOMAIN_BOUND 10 // Tiny Kernel domain extends from block 31 till block 10
#define TI_MOD_DOMAIN_BOUND 21 // Tiny Module domain extends from block 0 till block 21
#define SM_KER_DOMAIN_BOUND 5  // Small Kernel domain extends from block 15 till block 5
#define SM_MOD_DOMAIN_BOUND 10 // Small module domain extents from block 0 till block 10
#define LG_KER_DOMAIN_BOUND 1  // Large Kernel domain extends from block 3 till block 1 
#define LG_MOD_DOMAIN_BOUND 3  // Large Kernel domain extends from block 0 till block 3
#endif//FAULT_TOLERANT_SOS

#endif
