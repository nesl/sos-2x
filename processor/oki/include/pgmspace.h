/* Copyright (c) 2002, 2003, 2004  Marek Michalkiewicz
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */
/*
   pgmspace.h

   Contributors:
     Created by Marek Michalkiewicz <marekm@linux.org.pl>
     Eric B. Weddington <eric@ecentral.com>
     Wolfgang Haidinger <wh@vmars.tuwien.ac.at> (pgm_read_dword())
 */

#ifndef __PGMSPACE_H_
#define __PGMSPACE_H_ 1

#define __need_size_t
#include <inttypes.h>
#include <stddef.h>

//#ifndef __ATTR_CONST__
//#define __ATTR_CONST__ __attribute__((__const__))
//#endif

#ifndef __ATTR_PROGMEM__
#define __ATTR_PROGMEM__ __attribute__((section(".progmem")))
#endif

//#ifndef __ATTR_PURE__
//#define __ATTR_PURE__ __attribute__((__pure__))
//#endif

#define PROGMEM __ATTR_PROGMEM__

#ifdef __cplusplus
extern "C" {
#endif

typedef void prog_void;	// PROGMEM;
typedef char prog_char;	// PROGMEM;
typedef unsigned char prog_uchar;	// PROGMEM;

/*
typedef void prog_void PROGMEM;
typedef char prog_char PROGMEM;
typedef unsigned char prog_uchar PROGMEM;

typedef int8_t    prog_int8_t   PROGMEM;
typedef uint8_t   prog_uint8_t  PROGMEM;
typedef int16_t   prog_int16_t  PROGMEM;
typedef uint16_t  prog_uint16_t PROGMEM;
#if defined(__HAS_INT32_T__)
typedef int32_t   prog_int32_t  PROGMEM;
typedef uint32_t  prog_uint32_t PROGMEM;
#endif
#if defined(__HAS_INT64_T__)
typedef int64_t   prog_int64_t  PROGMEM;
typedef uint64_t  prog_uint64_t PROGMEM;
#endif
*/

/* Although in C, we can get away with just using __c, it does not work in
   C++. We need to use &__c[0] to avoid the compiler puking. Dave Hylands
   explaned it thusly,

     Let's suppose that we use PSTR("Test"). In this case, the type returned
     by __c is a prog_char[5] and not a prog_char *. While these are
     compatible, they aren't the same thing (especially in C++). The type
     returned by &__c[0] is a prog_char *, which explains why it works
     fine. */

/** \ingroup avr_pgmspace
    \def PSTR(s)
    Used to declare a static pointer to a string in program space. */
#define PSTR(s) ({static char __c[] PROGMEM = (s); &__c[0];})

/** \ingroup avr_pgmspace
    \def pgm_read_byte_near(address_short)
    Read a byte from the program space with a 16-bit (near) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_byte_near(address_short) //__LPM((uint16_t)(address_short))

/** \ingroup avr_pgmspace
    \def pgm_read_word_near(address_short)
    Read a word from the program space with a 16-bit (near) address.
    \note The address is a byte address.
    The address is in the program space. */
#define pgm_read_word_near(address_short) (*((volatile unsigned long *)(address_short)))          /* word input */

/** \ingroup avr_pgmspace
    \def pgm_read_dword_near(address_short)
    Read a double word from the program space with a 16-bit (near) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_dword_near(address_short) //__LPM_dword((uint16_t)(address_short))

/** \ingroup avr_pgmspace
    \def pgm_read_byte_far(address_long)
    Read a byte from the program space with a 32-bit (far) address.
    \note The address is a byte address.
    The address is in the program space. */

//#define pgm_read_byte_far(address_long)  //__ELPM((uint32_t)(address_long))

/** \ingroup avr_pgmspace
    \def pgm_read_word_far(address_long)
    Read a word from the program space with a 32-bit (far) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_word_far(address_long)  (*((volatile unsigned long *)(address_long)))          /* word input */

/** \ingroup avr_pgmspace
    \def pgm_read_dword_far(address_long)
    Read a double word from the program space with a 32-bit (far) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_dword_far(address_long) //__ELPM_dword((uint32_t)(address_long))

/** \ingroup avr_pgmspace
    \def pgm_read_byte(address_short)
    Read a byte from the program space with a 16-bit (near) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_byte(address_short)    pgm_read_byte_near(address_short)

/** \ingroup avr_pgmspace
    \def pgm_read_word(address_short)
    Read a word from the program space with a 16-bit (near) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_word(address_short)    pgm_read_word_near(address_short)
#define pgm_read_word(address_short) (*((volatile unsigned long *)(address_short)))          /* word input */

/** \ingroup avr_pgmspace
    \def pgm_read_dword(address_short)
    Read a double word from the program space with a 16-bit (near) address.
    \note The address is a byte address.
    The address is in the program space. */
//#define pgm_read_dword(address_short)   pgm_read_dword_near(address_short)

// Used to declare a variable that is a pointer to a string in program space.
#ifndef PGM_P
#define PGM_P const prog_char *
#endif

// Used to declare a generic pointer to an object in program space.
#ifndef PGM_VOID_P
#define PGM_VOID_P const prog_void *
#endif

#ifdef __cplusplus
}
#endif

#endif /* __PGMSPACE_H_ */
