/**
 * \file memmap_checker_const.h
 * \brief Memory Map Checker Constant Table
 * \author Ram Kumar {ram@ee.ucla.edu}
 */
#include <hardware.h>
#ifndef _MEMMAP_CHECKER_CONST_H_
#define _MEMMAP_CHECKER_CONST_H_

/**
 * Lookup table storing the permissions bitmask
 */
extern uint8_t MEMMAP_PERMS_BM_LUT[32] PROGMEM;

/**
 * Lookup table storing the bitmasks for the start of block
 */
extern uint8_t BLK_HDR_CHECK_BM_LUT[32] PROGMEM;


#endif//_MEMMAP_CHECKER_CONST_H_
