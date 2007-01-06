/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief Pin Map for TMote Sky Platform
 * @author Ram Kumar {ram@ee.ucla.edu}
 * $Id: pin_map.h,v 1.3 2006/04/06 10:33:06 ram Exp $
 */
//-----------------------------------------------------------
// TMOTE SKY PIN MAP
//-----------------------------------------------------------

#ifndef _PIN_MAP_H
#define _PIN_MAP_H

#include <pin_defs.h>

//--------------------------------------------
// LED
//--------------------------------------------
ALIAS_IO_PIN(   RED_LED, P5_4);
ALIAS_IO_PIN( GREEN_LED, P5_5);
ALIAS_IO_PIN(YELLOW_LED, P5_6);
//--------------------------------------------

//--------------------------------------------
// CC2420 RADIO
//--------------------------------------------
ALIAS_IO_PIN(  RADIO_CSN, P4_2); // Radio Chip Select (Active Low)
ALIAS_IO_PIN( RADIO_VREF, P4_5); // Radio VREG Enable (Active High)
ALIAS_IO_PIN(RADIO_RESET, P4_6); // Radio RESET (What is the polarity ?)
ALIAS_IO_PIN(RADIO_FIFOP, P1_0); 
ALIAS_IO_PIN(  RADIO_SFD, P4_1);
ALIAS_IO_PIN( RADIO_GIO0, P1_3);
ALIAS_IO_PIN( RADIO_FIFO, P1_3);
ALIAS_IO_PIN( RADIO_GIO1, P1_4);
ALIAS_IO_PIN(  RADIO_CCA, P1_4);


//ALIAS_IO_PIN(CC_FIFOP, P1_0);
//ALIAS_IO_PIN( CC_FIFO, P1_3);
//ALIAS_IO_PIN  (CC_SFD, P4_1);
//ALIAS_IO_PIN( CC_VREN, P4_5);
//ALIAS_IO_PIN( CC_RSTN, P4_6);


// ADC
ALIAS_IO_PIN(ADC0, P6_0);
//TOSH_ASSIGN_PIN(ADC0, 6, 0);
ALIAS_IO_PIN(ADC1, P6_1);
//TOSH_ASSIGN_PIN(ADC1, 6, 1);
ALIAS_IO_PIN(ADC2, P6_2);
//TOSH_ASSIGN_PIN(ADC2, 6, 2);
ALIAS_IO_PIN(ADC3, P6_3);
//TOSH_ASSIGN_PIN(ADC3, 6, 3);
ALIAS_IO_PIN(ADC4, P6_4);
//TOSH_ASSIGN_PIN(ADC4, 6, 4);
ALIAS_IO_PIN(ADC5, P6_5);
//TOSH_ASSIGN_PIN(ADC5, 6, 5);
ALIAS_IO_PIN(ADC6, P6_6);
//TOSH_ASSIGN_PIN(ADC6, 6, 6);
ALIAS_IO_PIN(ADC7, P6_7);
//TOSH_ASSIGN_PIN(ADC7, 6, 7);


// HUMIDITY
ALIAS_IO_PIN(HUM_SDA, P1_5);
//TOSH_ASSIGN_PIN(HUM_SDA, 1, 5);
ALIAS_IO_PIN(HUM_SCL, P1_6);
//TOSH_ASSIGN_PIN(HUM_SCL, 1, 6);
ALIAS_IO_PIN(HUM_PWR, P1_7);
//TOSH_ASSIGN_PIN(HUM_PWR, 1, 7);

// GIO pins
ALIAS_IO_PIN(GIO0, P2_0);
//TOSH_ASSIGN_PIN(GIO0, 2, 0);
ALIAS_IO_PIN(GIO1, P2_1);
//TOSH_ASSIGN_PIN(GIO1, 2, 1);
ALIAS_IO_PIN(GIO2, P2_3);
//TOSH_ASSIGN_PIN(GIO2, 2, 3);
ALIAS_IO_PIN(GIO3, P2_6);
//TOSH_ASSIGN_PIN(GIO3, 2, 6);


// 1-Wire
ALIAS_IO_PIN(ONEWORE, P2_4);
//TOSH_ASSIGN_PIN(ONEWIRE, 2, 4);


// FLASH
ALIAS_IO_PIN(FLASH_PWR, P4_3);
//TOSH_ASSIGN_PIN(FLASH_PWR, 4, 3);
ALIAS_IO_PIN(FLASH_CS, P4_4);
//TOSH_ASSIGN_PIN(FLASH_CS, 4, 4);
ALIAS_IO_PIN(FLASH_HOLD, P4_7);
//TOSH_ASSIGN_PIN(FLASH_HOLD, 4, 7);


// Detect if the mote is plugged into USB
ALIAS_IO_PIN(USB_DETECT, P1_2);
//TOSH_ASSIGN_PIN(USB_DETECT, 1, 2);


// PROGRAMMING PINS (tri-state)
//TOSH_ASSIGN_PIN(TCK, );
ALIAS_IO_PIN(PROG_RX, P1_1);
//TOSH_ASSIGN_PIN(PROG_RX, 1, 1);
ALIAS_IO_PIN(PROG_TX, P2_2);
//TOSH_ASSIGN_PIN(PROG_TX, 2, 2);

void init_IO(void);

#endif//_PIN_MAP_H
