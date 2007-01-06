/**
 * @file memmap_conf.h
 * @brief AVR Processor architecture configuration for memory map
 * @author Ram Kumar {ram@ee.ucla.edu}
 */

#ifndef _MEMMAP_CONF_H_
#define _MEMMAP_CONF_H_

//-----------------------------------------------------------------------
// PROCESSOR SPECIFIC CONSTANTS
//-----------------------------------------------------------------------

/**
 * \addtogroup memorymap
 * AVR architecture has a 16-bit address range. <BR>
 * The total internal memory on ATMEGA128 is 4 KB. <BR>
 * The word size is 1 byte. <BR>
 * Address is represented as bit vector <A11:A0> <BR>
 * We use a block size of 8 bytes. <BR>
 * Block number is given by the bit vector <A11:A3> <BR>
 * @{
 */

/**
 * Configuration for the memory map
 */
enum memmap_config_t
  {
    MEMMAP_START_ADDR  = 0,      //!< First address covered by memmap
    MEMMAP_END_ADDR    = 4096UL, //!< Last address covered by memmap
    MEMMAP_REGION_SIZE = MEMMAP_END_ADDR - MEMMAP_START_ADDR, //!< Size of region covered by memmap
    MEMMAP_BLOCK_SIZE  = 8,      //!< Block size in bytes
  };

#define SOS_MEMMAP_SECTION __attribute__((section(".sos_memmap_section")));

typedef uint16_t data_addr_t;
typedef uint16_t blk_num_t;


/* @} */

#endif//_MEMMAP_CONF_H_
