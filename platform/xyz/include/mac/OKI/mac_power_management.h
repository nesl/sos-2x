/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                             CC2420 Power Management                            *
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
 * This module contains functions to be used by the higher layer to power down the CC2420.             *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACPOWERMANAGEMENT_H
#define MACPOWERMANAGEMENT_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// The number of microseconds used to power up the CC2420 voltage regulator (taken from the data sheet)
#define MPM_VREG_TURN_ON_TIME 600
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Available power modes
// Note: A transition from MPM_CC2420_XOSC_OFF to MPM_CC2420_XOSC_AND_VREG_OFF is not possible
#define MPM_CC2420_ON                   0
#define MPM_CC2420_XOSC_OFF             1
#define MPM_CC2420_XOSC_AND_VREG_OFF    2

// The confirmation status codes used by mpmSetConfirm(...)
#define OK_POWER_MODE_CHANGED           0
#define OK_POWER_MODE_UNCHANGED         1
#define ERR_RX_ON_WHEN_IDLE             2
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   MODULE DATA                     **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Internal module data
typedef struct {
    UBYTE currentState;
    UBYTE selectedMode;
} MPM_INFO;
extern MPM_INFO mpmInfo;
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
//  void mpmSetRequest(BYTE mode)
//
//  DESCRIPTION:
//      This function allows the higher layer to power down CC2420 to extend battery lifetime. CC2420
//      must be powered up before any MLME or MCPS primitives can be used (both beacon/non-beacon modes).
//      Power-down is currently only supported for non-beacon PANs.
//
//      The change is not likely to happen instantaneously (under normal conditions the delay can be up 
//      to 320 us). Use either the mpmSetConfirm callback, or poll the current power state by using
//      mpmGetState() (returns the selected power mode when it has become effective).
//
//  ARGUMENTS:
//      BYTE mode
//          MPM_CC2420_ON:                The CC2420 crystal oscillator is on, ready to receive/transmit
//          MPM_CC2420_XOSC_OFF:          The CC2420 crystal oscillator is off (startup time ~1 ms)
//          MPM_CC2420_XOSC_AND_VREG_OFF: The CC2420 voltage regulator is off (startup time ~1.6 ms)
//
//          Note: Nothing will happen if the current state is MPM_CC2420_XOSC_AND_VREG_OFF, and the new
//          mode is MPM_CC2420_XOSC_OFF.
//-------------------------------------------------------------------------------------------------------
void mpmSetRequest(UBYTE mode);


//-------------------------------------------------------------------------------------------------------
//  void mpmSetRequest(BYTE mode)
//
//  DESCRIPTION:
//      Confirms that the CC2420 power mode change initiated by mpmSetRequest has become effective
//
//  ARGUMENTS:
//      BYTE status
//          OK_POWER_MODE_CHANGED:   The power mode was changed
//          OK_POWER_MODE_UNCHANGED: No change was required
//          ERR_RX_ON_WHEN_IDLE:     Could not proceed because "RX on when idle" was enabled
//-------------------------------------------------------------------------------------------------------
void mpmSetConfirm(UBYTE status);


//-------------------------------------------------------------------------------------------------------
//  BYTE mpmGetState(void)
//
//  DESCRIPTION:
//      Returns the current power state when it has become effective (after a call to mpmSetRequest)
//
//  RETURN VALUE:
//      BYTE
//          MPM_CC2420_ON:                The CC2420 crystal oscillator is on, ready to receive/transmit
//          MPM_CC2420_XOSC_OFF:          The CC2420 crystal oscillator is off
//          MPM_CC2420_XOSC_AND_VREG_OFF: The CC2420 voltage regulator is off
//-------------------------------------------------------------------------------------------------------
UBYTE mpmGetState(void);


//-------------------------------------------------------------------------------------------------------
// Internal functions

// Power up/down
void mpmTurnOnVregAndReset(void);
void mpmTurnOffReset(void);
void mpmTurnOnXosc(void);
void mpmTurnOffVreg(void);
void mpmTurnOffXosc(void);

// Restores all CC2420 registers and RAM, assuming that there was no activity in the MAC layer at power-
// down
void mpmRestoreRegsAndRam(void);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_power_management.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:49  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:28  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:12:00  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.3  2004/10/27 03:49:25  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.5  2004/08/13 13:04:43  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
