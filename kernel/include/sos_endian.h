
/**
 * The code is copied and modified from 
 * http://www.netrino.com/Publications/Glossary/Endianness.html
 * 
 * Brown, Christopher and Michael Barr. "Introduction to Endianness" 
 * Embedded Systems Programming, January 2002 , pp. 55-56.
 *
 * NOTE: we choose to use little endian as the default because 
 * All of supported MCUs are little endian 
 */
#ifndef _SOS_ENDIAN_H
#define _SOS_ENDIAN_H

#if defined(LLITTLE_ENDIAN) && !defined(BBIG_ENDIAN)

#define ehtons(X)  (X)
#define ehtonl(X)  (X)
#define entohs(X)  (X)
#define entohl(X)  (X)


#elif defined(BBIG_ENDIAN) && !defined(LLITTLE_ENDIAN)

#define ehtons(X)  ((((uint16_t)(X) & 0xff00) >> 8) | \
                  (((uint16_t)(X) & 0x00ff) << 8))
#define ehtonl(X)  ((((uint32_t)(X) & 0xff000000) >> 24) | \
                  (((uint32_t)(X) & 0x00ff0000) >> 8)  | \
                  (((uint32_t)(X) & 0x0000ff00) << 8)  | \
                  (((uint32_t)(X) & 0x000000ff) << 24))
#define entohs     ehtons
#define entohl     ehtonl
#elif defined(BBIG_ENDIAN) && defined(LLITTLE_ENDIAN)
#error "Either BBIG_ENDIAN or LLITTLE_ENDIAN must be #defined, but not both."
#else
#error "neither BBIG_ENDIAN nor LLITTLE_ENDIAN is defined."
#endif

#endif // ifndef _SOS_ENDIAN_H
