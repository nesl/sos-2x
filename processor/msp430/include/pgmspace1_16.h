/**
 * @file pgmspace1_16.h
 * @brief Macros to read the program memory
 * @author Ram Kumar {ram@ee.ucla.edu}
 * Note:
 * 1. These MACROS are implemented for the kernel to be able to
 *    access the progra memory.
 * 2. MSP430 uses the same address space for program and data memory
 *    and therefore these MACROS are trivial
 * 3. AVR uses a separate address space and therefore it needs
      special treatment
*/

#ifndef __PGMSPACE1_16_H_
#define __PGMSPACE1_16_H_ 

#include <sos_inttypes.h>

#ifndef __ATTR_PROGMEM__
// If the compiler is not placing the constant in the FLASH
// then uncomment the line below
#define __ATTR_PROGMEM__ __attribute__ ((section (".sosprogmem")))
//#define __ATTR_PROGMEM__ __attribute__ ((section (".text")))
//#define __ATTR_PROGMEM__
#endif

#define PROGMEM __ATTR_PROGMEM__

/* #define pgm_read_byte_near(address_short) *((uint8_t *)(address_short)) */

/* #define pgm_read_word_near(address_short) *((uint16_t *)(address_short)) */

/* #define pgm_read_dword_near(address_short) *((uint32_t *)(address_short)) */

/* #define pgm_read_byte_far(address_long)   *((uint8_t *)(address_long)) */
/* //#define pgm_read_byte_far(address_long)   ((uint32_t)(address_long)) */



/* #define pgm_read_word_far(address_long)  *((uint16_t *)(address_long)) */
/* //#define pgm_read_word_far(address_long)  ((uint32_t)(address_long)) */




/* #define pgm_read_dword_far(address_long) *((uint32_t *)(address_long)) */


/* #define pgm_read_byte(address_short)    *((uint8_t *)(address_short)) */


/* #define pgm_read_word(address_short)    *((uint16_t *)(address_short)) */


/* #define pgm_read_dword(address_short)   *((uint32_t *)(address_short)) */


#endif /* __PGMSPACE_H_ */
