/**
 * @brief integer types for SOS
 *
 * This is a subset of inttypes.h
 * Since Cygwin does not support inttypes natively, 
 * we provide a wrapper here
 *
 */
#ifndef _SOS_INTTYPES_H
#define _SOS_INTTYPES_H

#ifdef __CYGWIN__
#ifndef _STDINT_H
#define _STDINT_H
#ifndef _CYGWIN_INTTYPES_H
#define _CYGWIN_INTTYPES_H

#ifndef __uint8_t_defined
#define __uint8_t_defined
typedef unsigned char uint8_t;
#endif
#ifndef __int8_t_defined
#define __int8_t_defined
typedef signed char int8_t;
#endif
#ifndef __uint16_t_defined
#define __uint16_t_defined
typedef unsigned short uint16_t;
#endif
#ifndef __int16_t_defined
#define __int16_t_defined
typedef signed short int16_t;
#endif
#ifndef __uint32_t_defined
#define __uint32_t_defined
typedef unsigned int uint32_t;
#endif
#ifndef __int32_t_defined
#define __int32_t_defined
typedef signed int int32_t;
#endif
#ifndef __int64_t_defined
#define __int64_t_defined
typedef long long int64_t;
#endif
#ifndef __uint64_t_defined
#define __uint64_t_defined
typedef unsigned long long uint64_t;
#endif
#endif  // ifndef _CYGWIN_INTTYPES_H
#endif  // ifndef _STDINT_H
#else   
#include <inttypes.h>
#endif  // ifdef __CYGWIN__
#endif  // ifndef _SOS_INTTYPES_H

