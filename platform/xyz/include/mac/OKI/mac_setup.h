/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                            MAC Parameter Setup File                            *
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
 * This file contains the MAC setup parameters which may be set differently for different applications *
 * or devices. These parameters can also be set in the make file.                                      *
 *******************************************************************************************************
 * Compiler: Any C compiler                                                                            *
 * Target platform: CC2420DB, CC2420 + any MCU                                                         *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MAC_SETUP_H
#define MAC_SETUP_H


//-------------------------------------------------------------------------------------------------------
// Is the current device a FFD or RFD?
// Currently only FFD has been tested.
// Setting MAC_OPT_FFD to 0 will reduce code and RAM sizes
#ifndef MAC_OPT_FFD
    #define MAC_OPT_FFD 1
#endif
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// The maximum number of devices in the ACL
// If set to 0, ACL is not implemented.
// If MAC_OPT_SECURITY is set, MAC_OPT_ACL_SIZE must be >= 1
#ifndef MAC_OPT_ACL_SIZE
    #define MAC_OPT_ACL_SIZE 0
#endif
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Is security implemented?
// 0 : No
// 1 : Yes
#ifndef MAC_OPT_SECURITY
    #define MAC_OPT_SECURITY 0
#endif
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// If security is implemented, are optional sequential freshness included?
#ifndef MAC_OPT_SEQUENTIAL_FRESHNESS
    #define MAC_OPT_SEQUENTIAL_FRESHNESS 0
#endif
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// The maximum number of PAN descriptors returned from a passive or active scan. This memory space
// is set up by a layer above the MAC
#ifndef MAC_OPT_MAX_PAN_DESCRIPTORS
    #define MAC_OPT_MAX_PAN_DESCRIPTORS 4
#endif
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// The number of packets in the RX pool (at least 1)
#ifndef MAC_OPT_RX_POOL_SIZE
    #define MAC_OPT_RX_POOL_SIZE 4
#endif
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// The number of packets in the TX pool (at least 1, 2 when transmitting beacons)
#ifndef MAC_OPT_TX_POOL_SIZE
    #define MAC_OPT_TX_POOL_SIZE 6
#endif
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_setup.h,v $
 * Revision 1.1.1.1  2005/06/23 05:11:50  simonhan
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
 * Revision 1.3  2004/08/13 13:04:44  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
