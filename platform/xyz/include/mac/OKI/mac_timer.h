/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                           Low-Level Timing Functions                           *
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
 * This module contains the MAC callback timer, which is used to handle timer events, and start the    *
 * execution of tasks. handles. The timer generates a T1.COMPA interrupt at every                      *
 * 320 usecs = 20 symbols = 1 backoff slot.                                                            *
 *                                                                                                     *
 * Command strobes that need to be aligned with backoff slot boundary must use the WAIT_FOR_BOUNADRY() *
 * macro.                                                                                              *
 *                                                                                                     *
 * NOTE: These functions are meant to be used with an 8 MHz crystal oscillator!                        *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_TIMER_H
#define MAC_TIMER_H

#include <sos_inttypes.h>

/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The timer overflow value
#define CLOCK_SPEED_MHZ                 58  // MHz
#define MAC_SYMBOL_DURATION             16 // us
#define MAC_TIMER_OVERFLOW_VALUE        (CLOCK_SPEED_MHZ * MAC_SYMBOL_DURATION * aUnitBackoffPeriod - 1)

// Backoff slot boundary offset (1/4 into the timer tick interval)
#define MAC_TIMER_BACKOFF_SLOT_OFFSET   ((MAC_TIMER_OVERFLOW_VALUE + 1) / 4)

// Waits for the next backoff slot boandary (global interrupts should be turned off!)
#define WAIT_FOR_BOUNDARY()             while(!(get_hvalue(TIMESTAT2) | 0xFFFE))

// The number of callbacks available in the pool
#define MAC_CALLBACK_COUNT 12

// Callback queue terminator
#define NO_CALLBACK 0xFF
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Internal module data

// Callback table
typedef struct {
    VFPTR pFunc;
    INT32 timeout;
    UINT8 nextCallback;
    UINT8 prevCallback;
    BOOL  occupied;
} MAC_CALLBACK_INFO;

// Backoff slot counter
typedef struct {
    INT32 volatile bosCounter;
    UINT32 captureTime;
    UINT32 captureBosCounter;
    UINT16 captureTcnt;
    UINT32 bosCounterAdjustTime;
    UINT8 beaconDuration;
    UINT8 volatile nextCallback;
    uint32_t systime;
} MAC_TIMER_INFO;
extern MAC_TIMER_INFO mtimInfo;
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Callback timer setup (timeout in backoff slots (rounded downwards)
void mtimInit(void);
BOOL mtimSetCallback(VFPTR pFunc, INT32 timeout);
BOOL mtimCancelCallback(void *pFunc);

// Callback timer adjustments (align this node's timer with the last received beacon)
void mtimAlignWithBeacon(void);
//-------------------------------------------------------------------------------------------------------

void SfdHandler(void);
void nullHandler(void); // The handler for TIMER1B
void timerHandler(void); // The handler for the basic timer
void timerHandlerC(void); // The handler for TIMER1C


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_timer.h,v $
 * Revision 1.2  2005/11/07 22:13:21  abs
 * add timestamping to the mac layer.
 * add systime.c and call systime_init in hardware.c/hardware_init.
 * add timestamps to the radio.c.
 *
 * Revision 1.1.1.1  2005/06/23 05:11:51  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:29  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:01  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.3  2004/10/27 03:49:26  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.5  2004/08/13 13:04:44  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
