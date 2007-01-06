/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                        CHIPCON CC2420 INTEGRATED 802.15.4 MAC AND PHY                  *
 *      ***   + +   ***                MAC Security Support                                            *
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
 * This file implements the MAC security support functions, used internally in the MAC sublayer        *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB, CC2420 + any ATMEGA MCU                                                  *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/

#ifndef MAC_SECURITY_H
#define MAC_SECURITY_H

typedef struct {
    UBYTE secFlags2420;
    UBYTE secMode2420;
    UBYTE micLength;
    UBYTE clearTextLength;
} MSEC_SETUP_INFO;

// msecFindSecurityMaterial function prototype moved to mac_tx_pool.h because of 
// type definition conflicts between these files and function headers
// BYTE msecFindSecurityMaterial(MAC_TX_PACKET *pPacket, BOOL securityEnable, BYTE addrMode, WORD panId, ADDRESS *pAddr);

void msecDecodeSecuritySuite(MSEC_SETUP_INFO *pMSI, UBYTE securitySuite);
void msecSetupCC2420KeyAndNonce(BOOL isTx, MSEC_SETUP_INFO *pMSI, KEY pKey, UBYTE* pCounters);
void msecSetupCC2420Regs(MSEC_SETUP_INFO *pMSI);

#endif

/*******************************************************************************************************
 * Revision history:
 *
 * $Log: mac_security.h,v $
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
 * Revision 1.4  2004/08/13 13:04:44  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
