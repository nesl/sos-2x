/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                               Master Header File                               *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * CONFIDENTIAL                                                                                        *
 * The use of this file is restricted by the signed MAC software license agreement.                    *
 *                                                                                                     *
 * Copyright Chipcon AS, 2004                                                                          *
 *******************************************************************************************************
 * This file shall be included in all source files (*.c). No other header inclusions are required.     *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACHEADERS_H
#define MACHEADERS_H



// header files for the OKI
#include <hardware_types.h>
//#include "ML674001.h"
//#include "irq.h"
#include <string.h>			// for memcpy()


/*******************************************************************************************************
 *******************************************************************************************************
 **************************               GENERAL DEFINITIONS                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Common data types
typedef unsigned char		BOOL;

typedef char    BYTE;   /* byte */
typedef short   HWORD;  /* half word */
typedef long    WORD;   /* word */

typedef unsigned char   UBYTE;  /* unsigned byte */
typedef unsigned short  UHWORD; /* unsigned half word */
typedef unsigned long   UWORD;  /* unsigned word */

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




/*******************************************************************************************************
 *******************************************************************************************************
 **************************           HAL/MAC LIBRARY HEADER FILES            **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Hardware abstraction library for the ATMEGA128(L)
#include "hal/OKI/hal.h"
#include "hal/hal_cc2420.h"
#include "hal/OKI/hal_cc2420db.h"
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Standard GCC include files for AVR
//#include <string.h>

// Development board access (used by app/higher layer)
#ifdef STK501
    #include "hal/OKI/hal_stk501.h"
#endif
#ifdef CC2420DB
    #include "hal/OKI/hal_cc2420db.h"
#endif

// MAC setup (parameters, sizes, options)
#include "mac/OKI/mac_setup.h"

// MAC interface (used by the higher layer)
#include "mac/OKI/mac.h"

// CC2420 power-down
#include "mac/OKI/mac_power_management.h"
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// INTERNAL

// MAC header files (for internal use only!)
#include "mac/OKI/mac_general.h"
#include "mac/OKI/mac_timer.h"
#include "mac/OKI/mac_scheduler.h"
#include "mac/OKI/mac_security.h"
#include "mac/OKI/mac_tx_pool.h"
#include "mac/OKI/mac_tx_engine.h"
#include "mac/OKI/mac_indirect_queue.h"
#include "mac/OKI/mac_indirect_polling.h"
#include "mac/OKI/mac_support.h"
#include "mac/OKI/mac_beacon_handler.h"
#include "mac/OKI/mac_rx_pool.h"
#include "mac/OKI/mac_rx_engine.h"
#include "mac/OKI/mac_scan.h"
//-------------------------------------------------------------------------------------------------------



#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_headers.h,v $
 * Revision 1.5  2006/03/10 06:18:59  ndbusek
 * updated posix and oki platforms to standard hw_proc naming
 * include obsolete header file for older avr-libc
 *
 * Revision 1.4  2005/11/07 21:55:30  abs
 * add include string.h to the mac_headers to declare the memcpy routine.
 * clean up the definitions for the interrupt control macros.
 * increase the size of the onCounter to avoid the counter overflow that was effectively disabling the receiver.
 *
 * Revision 1.3  2005/07/16 00:22:28  abs
 * removed extra unecessary clutter files from processor/oki and moved important stuff into hardware_oki and mac_headers.h files.
 * updated other files appropriately.
 *
 * Revision 1.2  2005/07/14 23:09:49  abs
 * create new processor/oki for the platform/xyz
 * add the appropriate files
 *
 * Revision 1.1.1.1  2005/06/23 05:11:46  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:25  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:11:57  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.4  2004/10/27 03:49:25  simonhan
 * update xyz
 *
 * Revision 1.3  2004/10/27 03:00:52  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.7  2004/08/13 13:04:41  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
