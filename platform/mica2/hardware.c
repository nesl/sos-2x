/* -*- Mode: C; tab-width:4 -*- */
/* ex: set ts=4: */
/*
 * Copyright (c) 2003 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *       This product includes software developed by Networked &
 *       Embedded Systems Lab at UCLA
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * @brief    hardware related routines
 * @author Simon Han (simonhan@ee.ucla.edu)
 *
 *
 */
#include "hardware.h"
#include <flash.h>
#include <kertable.h>
#include <kertable_proc.h>
#include <kertable_plat.h>

#define LED_DEBUG
#include <led_dbg.h>

#ifndef NO_SOS_UART
#include <uart_system.h>
#include <sos_uart.h>
#endif

#ifndef NO_SOS_I2C
#include <sos_i2c.h>
#include <sos_i2c_mgr.h>
#endif

#ifdef SOS_SFI
#include <sfi_jumptable.h>
#endif


  
//TODO: For testing.  We may want to move this from CVS (Roy)
#include <one_wire.h>

//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  GLOBAL DATA 
//----------------------------------------------------------------------------
static uint8_t reset_flag NOINIT_VAR;
/**
 * @brief Kernel jump table
 * The table entries are defined in kertable.h
 */

#ifdef SOS_SFI
PGM_VOID_P ker_jumptable[128] PROGMEM = SOS_SFI_KER_TABLE;
#else
/** Append any processor or platform specific kernel tables */
#if defined(PROC_KER_TABLE) && defined(PLAT_KER_TABLE)
PGM_VOID_P ker_jumptable[128] PROGMEM =
SOS_KER_TABLE( CONCAT_TABLES(PROC_KER_TABLE , PLAT_KER_TABLE) );
#elif defined(PROC_KER_TABLE)
PGM_VOID_P ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(PROC_KER_TABLE);
#elif defined(PLAT_KER_TABLE)
PGM_VOID_P ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(PLAT_KER_TABLE);
#else
PGM_VOID_P ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(NULL);
#endif
#endif//SOS_SFI


//-------------------------------------------------------------------------
// FUNCTION DECLARATION
//-------------------------------------------------------------------------
void hardware_init(void){
  init_IO();

  // WATCHDOG SETUP FOR AVR
#ifndef DISABLE_WDT
  /*
   * Hardware Fuse Bit ensures WDT is always ON
   * The following timed sequence sets up a WDT
   * with the longest timeout period.
   */
  __asm__ __volatile__ ("wdr");
  WDTCR = (1 << WDCE) | (1 << WDE);
  WDTCR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);
#else
  /*
   * WDT may need to be disabled during debugging etc.
   * You must also unset the WDT fuse.  If it is set it 
   * is not possible to disable the WDT in software.
   * Setting the fuses to ff9fff will allow it to be disabled.
   */
  
  __asm__ __volatile__ ("wdr");
  WDTCR = (1 << WDCE) | (1 << WDE);
  WDTCR = 0;
#endif //DISABLE_MICA2_WDT

  // LEDS
  // We do led initialization in sos_watchdog_processing

  // LOCAL TIME
  systime_init();

  // SYSTEM TIMER
  timer_hardware_init(DEFAULT_INTERVAL, DEFAULT_SCALE);

  // UART
#ifdef USE_UART1
  SET_FLASH_SELECT_DD_OUT();
  SET_FLASH_SELECT();
#endif
  uart_system_init();
#ifndef NO_SOS_UART
  //! Initalize uart comm channel
  sos_uart_init();
#endif

  // I2C
  // always initalize the i2c system
  i2c_system_init();
#ifndef NO_SOS_I2C
  //! Initalize i2c comm channel
  sos_i2c_init();
  //! Initialize the I2C Comm Manager
  // Ram - Assuming that it is turned on
  // by default with the SOS_I2C component
  sos_i2c_mgr_init(); 
#endif

  // ADC
  adc_proc_init();
  
  // RADIO
#ifndef SOS_EMU
    cc1k_radio_init();
#ifndef DISABLE_RADIO
    cc1k_radio_start();
#ifdef RADIO_XMIT_POWER
  cc1k_cnt_SetRFPower(RADIO_XMIT_POWER);
#endif//RADIO_XMIT_POWER
#else
    cc1k_radio_stop();
#endif//DISABLE_RADIO
#endif//SOS_EMU
  
  // EXTERNAL FLASH
#ifndef USE_UART1
  exflash_init();
#endif


  // MICA2 PERIPHERALS (Optional)
#ifdef SOS_MICA2_PERIPHERAL
  mica2_peripheral_init();
#endif

  //TODO: We may want to move this out of the mica2 hardware (Roy)
  one_wire_init();

}


void init_IO(void)
{
  SET_CC_CHP_OUT_DD_IN();	// modified for mica2 series

#ifndef SECONDARY_DEVICE
  SET_PW7_DD_OUT();
  SET_PW6_DD_OUT();
  SET_PW5_DD_OUT();
  SET_PW4_DD_OUT();
  SET_PW3_DD_OUT(); 
  SET_PW2_DD_OUT();
  SET_PW1_DD_OUT();
  SET_PW0_DD_OUT();
#endif

  SET_SERIAL_ID_DD_IN();
  CLR_SERIAL_ID();  // Prevent sourcing current
    
}

/**
 * @brief functions for handling reset
 * 
 * This function will actually run before main() due to 
 * its section assignment.  
 */
#if 1
void sos_watchdog_processing() __attribute__ ((naked)) __attribute__ ((section (".init3")));

void sos_watchdog_processing()
{
  reset_flag = MCUCSR & 0x1F;
  MCUCSR = 0;

  // always init need to enable/disable pins
  led_init();
  if((reset_flag & ((1 << WDRF)))) {
	//booting_cond = SOS_BOOT_WDOGED;	
	LED_DBG(LED_RED_ON);
  }
  if(reset_flag & (1 << EXTRF)) {
	LED_DBG(LED_GREEN_ON);
  }
  if((reset_flag & ((1 << PORF)))) {
	LED_DBG(LED_GREEN_ON);
  }
  //MCUCSR = 0;
  /**
   * Check for watchdog reset, and resset due to 
   * illegal addressing (reset_flag == 0)
   */
  /*
	if(reset_flag == 0) {
	//booting_cond = SOS_BOOT_CRASHED;
	} else {
	LED_DBG(LED_RED_ON);
	//booting_cond = SOS_BOOT_NORMAL;
	}
  */
  /*
	if(booting_cond != SOS_BOOT_NORMAL) {
	sched_post_crash_checkup();
	}
  */
}
#endif


//-------------------------------------------------------------------------
// MAIN
//-------------------------------------------------------------------------
int main(void)
{
  sos_main(SOS_BOOT_NORMAL);
  return 0;
}
