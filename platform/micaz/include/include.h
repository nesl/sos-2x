/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                                     CHIPCON CC2420 SOFTWARE                            *
 *      ***   + +   ***                              Master inclusion file                             *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * This file shall be included in all source files (*.c). No other library inclusions are required.    *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * Revision history:                                                                                   *
 * $Log: include.h,v $
 * Revision 1.2  2006/03/10 00:02:14  ndbusek
 * updated to avr-libc and shortened constant to 32 bit (max size handeled by compiler)
 *
 * Revision 1.1.1.1  2005/06/23 05:11:38  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:17  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:11:48  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 08:02:27  simonhan
 * check in micaz directory
 *
 * Revision 1.3  2005/03/29 00:54:58  simonhan
 * remove SOS_DEV_SECTION
 *
 * Revision 1.2  2005/03/21 03:56:28  simonhan
 * switch back to our own version of pgmspace
 *
 * Revision 1.1.1.1  2005/01/14 22:30:25  simonhan
 * sos micaz
 *
 * Revision 1.2  2004/11/15 04:06:58  ram
 * Moved the Zigbee MAC to the 
 *
 * Revision 1.1  2004/08/26 20:06:39  ram
 * MicaZ IEEE 802.15.4 MAC
 *
 * Revision 1.5  2004/04/02 12:27:32  oyj
 * Added mac_security.h
 *
 * Revision 1.4  2004/03/30 15:00:26  mbr
 * Release for web
 *  
 *
 *
 *
 *******************************************************************************************************/

#ifndef INCLUDE_H
#define INCLUDE_H


//-------------------------------------------------------------------------------------------------------
// Common data types
typedef unsigned char		BOOL;

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned long		DWORD;
typedef unsigned long long	QWORD;

typedef unsigned char		UINT8;
typedef unsigned short		UINT16;
typedef unsigned long		UINT32;
typedef unsigned long long	UINT64;

typedef signed char			INT8;
typedef signed short		INT16;
typedef signed long			INT32;
typedef signed long long	INT64;

// Common values
#ifndef FALSE
	#define FALSE 0
#endif
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef NULL
	#define NULL 0
#endif

// Useful stuff
#define BM(n) (1 << (n))
#define BF(x,b,s) (((x) & (b)) >> (s))
#define MIN(n,m) (((n) < (m)) ? (n) : (m))
#define MAX(n,m) (((n) < (m)) ? (m) : (n))
#define ABS(n) ((n < 0) ? -(n) : (n))

// Dynamic function pointer
typedef void (*VFPTR)(void);
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Standard GCC include files for AVR
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

// SOS Specific Includes
#include <sos.h>

#ifdef STK501
    #include "hal/atmega128/hal_stk501.h"
#endif
#ifdef CC2420DB
    #include "hal/atmega128/hal_cc2420db.h"
#endif

// By Ram
#include "hal/atmega128/hal_micaz.h"
// By Ram - For Debug
//#include "../apps/mac/micaz_demo/led.h"


// HAL include files
#include "hal/atmega128/hal.h"
#include "hal/hal_cc2420.h"

// BASIC RF include files
#include "basic_rf/basic_rf.h"

// MAC include files
#include "mac/mac_setup.h"
#include "mac/mac.h"
#include "mac/mac_security.h"
#include "mac/mac_csmaca.h"
#include "mac/atmega128/mac_timer.h"
//-------------------------------------------------------------------------------------------------------

#endif
