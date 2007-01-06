/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                         CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                 *
 *      ***   + +   ***                       Indirect Packet Polling + Related                        *
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
 * This module contains functions related to indirect packet polling, including packet formatting and  *
 * timeouts.                                                                                           *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef MACINDIRECTPOLLING_H
#define MACINDIRECTPOLLING_H




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               CONSTANTS AND MACROS                **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Time constants (number of backoff periods)

// Retry timeout if there was a resource shortage when polling for an association response
#define MIP_RETRY_ASSOCIATION_RESPONSE_POLLING_TIMEOUT 10
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               FUNCTION PROTOTYPES                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// Formats a data request frame and starts the transmission
MAC_ENUM mipTransmitDataRequest(UBYTE coordAddrMode, UHWORD coordPanId, ADDRESS *pCoordAddress, BOOL securityEnable);

// Internally generated data requests (uses mipTransmitDataRequest)
void mipTransmitAutoDataRequest(void);
MAC_ENUM mipTransmitAssocDataRequest(void);

// Timeouts and related
void mipDataRequestTimeout(void);
MAC_STATE_TYPE mipCancelDataRequestTimeout(void);
void mipPollAssociateResponse(void);
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_indirect_polling.h,v $
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
 * Revision 1.4  2004/08/13 13:04:43  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
