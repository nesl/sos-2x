/*******************************************************************************************************
 *                                                                                                     *
 *        **********                                                                                   *
 *       ************                                                                                  *
 *      ***        ***                                                                                 *
 *      ***   +++   ***                                                                                *
 *      ***   + +   ***                                                                                *
 *      ***   +                        CHIPCON HARDWARE ABSTRACTION LIBRARY FOR THE CC2420             *
 *      ***   + +   ***                               CC2420DB Definitions                             *
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
 * This file contains all definitions that are specific for the CC2420DB development platform.         *
 *******************************************************************************************************
 * Compiler: AVR-GCC                                                                                   *
 * Target platform: CC2420DB                                                                           *
 *******************************************************************************************************
 * The revision history is located at the bottom of this file                                          *
 *******************************************************************************************************/
#ifndef HAL_CC2400DB_H
#define HAL_CC2400DB_H



/*******************************************************************************************************
 *******************************************************************************************************
 **************************                   OKI I/O PORTS                   **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
// Port setup macros

// CC2420 interface config

// Set the directions for VREG_EN and RESET Radio pins. Initially the radio is turned off.
#define MAC_PORT_INIT() \
    do { \
        put_hvalue(GPPMD, get_hvalue(GPPMD) | 0x0011); \
		put_hvalue(GPPOD, get_hvalue(GPPOD) & 0xFFEF); /* VREG_EN -> LOW  */ \
		put_hvalue(GPPOD, get_hvalue(GPPOD) | 0x0001); /* RESET -> HIGH */ \
    } while (0)


// Enables/disables the SPI interface
#define	PIOD_PATTERN		0x0008
#define	PIOD_MASK_PATTERN	0xFFF7
#define SPI_ENABLE()                do { put_hvalue(GPPOD, (get_hvalue(GPPOD) & PIOD_MASK_PATTERN) ); } while (0)
#define SPI_DISABLE()               do { put_hvalue(GPPOD, (get_hvalue(GPPOD) | PIOD_PATTERN) ); } while (0)
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************                 CC2420 PIN ACCESS                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/


//-------------------------------------------------------------------------------------------------------
// CC2420 pin access

// Pin status
#define FIFO_IS_1()                 (get_hvalue(GPPID) & 0x0002)
#define CCA_IS_1()                  (get_hvalue(GPPID) & 0x0004)
#define RESET_IS_1()                (get_hvalue(GPPOD) & 0x0001)
#define VREG_IS_1()                 (get_hvalue(GPPOD) & 0x0010)
#define FIFOP_IS_1()                (get_hvalue(GPPIE) & 0x0080)
#define SFD_IS_1()                  (get_hvalue(GPPIE) & 0x0040)

#define FIFO_IS_ACTIVE()            FIFO_IS_1()
#define RESET_IS_ACTIVE()           !RESET_IS_1()
#define VREG_IS_ACTIVE()            VREG_IS_1()
#define FIFOP_IS_ACTIVE()           !FIFOP_IS_1()
#define SFD_IS_ACTIVE()             SFD_IS_1()
#define CCA_IS_ACTIVE()             CCA_IS_1()

// The CC2420 reset pin
#define SET_RESET_ACTIVE()          do { put_hvalue(GPPOD, get_hvalue(GPPOD) & 0xFFFE); } while (0)
#define SET_RESET_INACTIVE()        do { put_hvalue(GPPOD, get_hvalue(GPPOD) | 0x0001); } while (0)

// CC2420 voltage regulator enable pin
#define SET_VREG_ACTIVE()           do { put_hvalue(GPPOD, get_hvalue(GPPOD) | 0x0010); } while (0)
#define SET_VREG_INACTIVE()         do { put_hvalue(GPPOD, get_hvalue(GPPOD) & 0xFFEF); } while (0)
//-------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 **************************               EXTERNAL INTERRUPTS                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/



//-------------------------------------------------------------------------------------------------------
// Rising edge trigger for external interrupt 0 (FIFOP)
#define FIFOP_INT_INIT()            do { IRQ_HANDLER_TABLE[INT_EX2]=FifopHandler; 	\
										 set_wbit(IDM, IDM_INT_L_L & IDM_IRQ28); 	\
										 put_wvalue(IRQA, IRQA_IRQ28); 				\
										 put_wvalue(IRCL, 0x0000001C); 				\
										 put_wvalue(CILCL, 0xFFFFFFFF); 			\
										 set_wbit(ILC, ILC_ILC28 & ILC_INT_LV1); } while (0)

// FIFOP on external interrupt 0
#define ENABLE_FIFOP_INT()          do { set_wbit(ILC, ILC_ILC28 & ILC_INT_LV1); } while (0)
#define DISABLE_FIFOP_INT()         do { clr_wbit(ILC, ILC_ILC28); } while (0)
#define CLEAR_FIFOP_INT()           do { put_wvalue(IRQA, IRQA_IRQ28); 				\
										 put_wvalue(IRCL,0x0000001C); 				\
										 put_wvalue(CILCL, 0xFFFFFFFF); } while (0)
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// SFD interrupt on timer 1 capture pin
#define SFD_INT_INIT()				do { IRQ_HANDLER_TABLE[INT_EX1]=SfdHandler; 	\
										 set_wbit(IDM, IDM_INT_E_R & IDM_IRQ26); 	\
										 put_wvalue(IRQA, IRQA_IRQ26); 				\
										 put_wvalue(IRCL, 0x0000001A); 				\
										 put_wvalue(CILCL, 0xFFFFFFFF); 			\
										 set_wbit(ILC, ILC_ILC26 & ILC_INT_LV1); } while(0)
#define ENABLE_SFD_CAPTURE_INT()    do { set_wbit(ILC, ILC_ILC26 & ILC_INT_LV1); } while (0)
#define DISABLE_SFD_CAPTURE_INT()   do { clr_wbit(ILC, ILC_ILC26); } while (0)
#define CLEAR_SFD_CAPTURE_INT()     do { put_wvalue(IRQA, IRQA_IRQ26); 				\
										 put_wvalue(IRCL,0x0000001A); 				\
										 put_wvalue(CILCL, 0xFFFFFFFF); } while (0)
//-------------------------------------------------------------------------------------------------------


#endif




/*******************************************************************************************************
 * Revision history:
 *
 * $Log: hal_cc2420db.h,v $
 * Revision 1.2  2005/11/07 21:55:30  abs
 * add include string.h to the mac_headers to declare the memcpy routine.
 * clean up the definitions for the interrupt control macros.
 * increase the size of the onCounter to avoid the counter overflow that was effectively disabling the receiver.
 *
 * Revision 1.1.1.1  2005/06/23 05:11:47  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:44:26  simonhan
 * initial import
 *
 * Revision 1.1.1.1  2005/06/23 04:11:58  simonhan
 * initial import
 *
 * Revision 1.1  2005/04/25 07:50:03  simonhan
 * Check in XYZ device directory
 *
 * Revision 1.3  2004/10/27 03:00:52  simonhan
 * update xyz
 *
 * Revision 1.2  2004/10/27 00:20:40  asavvide
 * *** empty log message ***
 *
 * Revision 1.8  2004/08/13 13:04:41  jol
 * CC2420 MAC Release v0.7
 *
 *
 *******************************************************************************************************/
