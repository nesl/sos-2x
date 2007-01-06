/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                        CHIPCON HARDWARE ABSTRACTION LIBRARY FOR THE CC2420             *
 *      ***   + +   ***                                CC2420 Definitions                              *
 *      ***   +++   ***                                                                                *
 *      ***        ***                                                                                 *
 *       ************                                                                                  *
 *        **********                                                                                   *
 *                                                                                                     *
 *******************************************************************************************************
 * Copyright Chipcon AS, 2004                                                                          *
 *******************************************************************************************************
 * The Chipcon Hardware Abstraction Library is a collection of functions, macros and constants, which  *
 * can be used to ease access to the hardware on the CC2420 and the target microcontroller.            *
 *                                                                                                     *
 * This file contains register and RAM address definitions for CC2420, and additional useful stuff     *
 *******************************************************************************************************
 * Compiler: Any C compiler                                                                            *
 * Target platform: CC2420DB, CC2420 + any MCU                                                         *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef HAL_CC2420_H
#define HAL_CC2420_H


//-------------------------------------------------------------------------------------------------------
// CC2420 register constants

// Command strobes
#define CC2420_SNOP             0x00
#define CC2420_SXOSCON          0x01
#define CC2420_STXCAL           0x02
#define CC2420_SRXON            0x03
#define CC2420_STXON            0x04
#define CC2420_STXONCCA         0x05
#define CC2420_SRFOFF           0x06
#define CC2420_SXOSCOFF         0x07
#define CC2420_SFLUSHRX         0x08
#define CC2420_SFLUSHTX         0x09
#define CC2420_SACK             0x0A
#define CC2420_SACKPEND         0x0B
#define CC2420_SRXDEC           0x0C
#define CC2420_STXENC           0x0D
#define CC2420_SAES             0x0E

// Registers
#define CC2420_MAIN             0x10
#define CC2420_MDMCTRL0         0x11
#define CC2420_MDMCTRL1         0x12
#define CC2420_RSSI             0x13
#define CC2420_SYNCWORD         0x14
#define CC2420_TXCTRL           0x15
#define CC2420_RXCTRL0          0x16
#define CC2420_RXCTRL1          0x17
#define CC2420_FSCTRL           0x18
#define CC2420_SECCTRL0         0x19
#define CC2420_SECCTRL1         0x1A
#define CC2420_BATTMON          0x1B
#define CC2420_IOCFG0           0x1C
#define CC2420_IOCFG1           0x1D
#define CC2420_MANFIDL          0x1E
#define CC2420_MANFIDH          0x1F
#define CC2420_FSMTC            0x20
#define CC2420_MANAND           0x21
#define CC2420_MANOR            0x22
#define CC2420_AGCCTRL          0x23
#define CC2420_AGCTST0          0x24
#define CC2420_AGCTST1          0x25
#define CC2420_AGCTST2          0x26
#define CC2420_FSTST0           0x27
#define CC2420_FSTST1           0x28
#define CC2420_FSTST2           0x29
#define CC2420_FSTST3           0x2A
#define CC2420_RXBPFTST         0x2B
#define CC2420_FSMSTATE         0x2C
#define CC2420_ADCTST           0x2D
#define CC2420_DACTST           0x2E
#define CC2420_TOPTST           0x2F
#define CC2420_RESERVED         0x30

// FIFOs
#define CC2420_TXFIFO           0x3E
#define CC2420_RXFIFO           0x3F
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Memory

// Sizes
#define CC2420_RAM_SIZE         368
#define CC2420_FIFO_SIZE        128

// Addresses
#define CC2420RAM_TXFIFO        0x000
#define CC2420RAM_RXFIFO        0x080
#define CC2420RAM_KEY0          0x100
#define CC2420RAM_RXNONCE       0x110
#define CC2420RAM_SABUF         0x120
#define CC2420RAM_KEY1          0x130
#define CC2420RAM_TXNONCE       0x140
#define CC2420RAM_CBCSTATE      0x150
#define CC2420RAM_IEEEADDR      0x160
#define CC2420RAM_PANID         0x168
#define CC2420RAM_SHORTADDR     0x16A
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Status byte
#define CC2420_XOSC16M_STABLE_BM    BM(6)
#define CC2420_TX_UNDERFLOW_BM      BM(5)
#define CC2420_ENC_BUSY_BM          BM(4)
#define CC2420_TX_ACTIVE_BM         BM(3)
#define CC2420_LOCK_BM              BM(2)
#define CC2420_RSSI_VALID_BM        BM(1)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Security

// SECCTRL0.SEC_MODE (security mode)
#define CC2420_SECCTRL0_NO_SECURITY         0x0000
#define CC2420_SECCTRL0_CBC_MAC             0x0001
#define CC2420_SECCTRL0_CTR                 0x0002
#define CC2420_SECCTRL0_CCM                 0x0003

// SECCTRL0.SEC_M (number of bytes in the authentication field)
#define CC2420_SECCTRL0_SEC_M_IDX           2

// SECCTRL0.RXKEYSEL/TXKEYSEL (key selection)
#define CC2420_SECCTRL0_RXKEYSEL0           0x0000
#define CC2420_SECCTRL0_RXKEYSEL1           0x0020
#define CC2420_SECCTRL0_TXKEYSEL0           0x0000
#define CC2420_SECCTRL0_TXKEYSEL1           0x0040

// Others
#define CC2420_SECCTRL0_SEC_CBC_HEAD        0x0100
#define CC2420_SECCTRL0_RXFIFO_PROTECTION   0x0200

// MAC Security flags definitions. Note that the bits are shifted compared to the actual security flags 
// defined by IEEE 802.15.4, please see the CC2420 datasheet for details.
#define MAC_CC2420_CTR_FLAGS                0x42
#define MAC_CC2420_CCM_FLAGS                0x09
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// RSSI to Energy Detection conversion
// RSSI_OFFSET defines the RSSI level where the PLME.ED generates a zero-value
#define RSSI_OFFSET -38
#define RSSI_2_ED(rssi)   ((rssi) < RSSI_OFFSET ? 0 : ((rssi) - (RSSI_OFFSET)))
#define ED_2_LQI(ed) (((ed) > 63 ? 255 : ((ed) << 2)))
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// RX FIFO overflow detection
#define CC2420_RXFIFO_OVERFLOW() (FIFOP_IS_ACTIVE() && !FIFO_IS_ACTIVE())
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: hal_cc2420.h,v $
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
 * Revision 1.3  2004/10/27 03:49:25  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.5  2004/08/13 13:04:41  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
