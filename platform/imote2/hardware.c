/*
 * Copyright (c) 2006 Yale University.
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
 *       This product includes software developed by the Embedded Networks
 *       and Applications Lab (ENALAB) at Yale University.
 * 4. Neither the name of the University nor that of the Laboratory
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YALE UNIVERSITY AND CONTRIBUTORS ``AS IS''
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
 * @brief  Hardware initialization for the iMote2
 * @author Andrew Barton-Sweeney (abs@cs.yale.edu)
 */

#include "hardware.h"
#include <kertable.h>
#include <kertable_proc.h>
#include <kertable_plat.h>
//#include "pxa27xhardware_once.h"

#include "pxa27x_init.h"
#include "dvfs.h"
#include "pmic.h"

#define LED_DEBUG
#include <led_dbg.h>

// Move the conditional includes into the sub-routines
#ifndef NO_SOS_UART
#include <uart_system.h>
#include <sos_uart.h>
#endif

#ifndef NO_SOS_I2C
#include <sos_i2c.h>
#include <sos_i2c_mgr.h>
#endif

//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  GLOBAL DATA
//----------------------------------------------------------------------------
//static uint8_t reset_flag NOINIT_VAR;

/**
 * @brief Kernel jump table
 * The table entries are defined in kertable.h
 */

/** Append any processor or platform specific kernel tables */
#if defined(PROC_KER_TABLE) && defined(PLAT_KER_TABLE)

PGM_VOID_P ker_jumptable[128] PROGMEM = SOS_KER_TABLE( CONCAT_TABLES(PROC_KER_TABLE , PLAT_KER_TABLE) );

#elif defined(PROC_KER_TABLE)

PGM_VOID_P ker_jumptable[128] PROGMEM = SOS_KER_TABLE(PROC_KER_TABLE);

#elif defined(PLAT_KER_TABLE)

PGM_VOID_P ker_jumptable[128] PROGMEM = SOS_KER_TABLE(PLAT_KER_TABLE);

#else

PGM_VOID_P ker_jumptable[128] PROGMEM = SOS_KER_TABLE(NULL);

#endif

//-------------------------------------------------------------------------
// FUNCTION DECLARATION
//-------------------------------------------------------------------------
// Move the conditional code to inside the init sub-routines
void hardware_init(void)
{
	uart_system_init();						// UART
#ifndef NO_SOS_UART
	//! Initalize uart comm channel
	sos_uart_init();
#endif
	//i2c_system_init();					// I2C
#ifndef NO_SOS_I2C
	//! Initalize i2c comm channel
	sos_i2c_init();
	sos_i2c_mgr_init();
#endif
	//adc_proc_init();						// ADC
#ifndef SOS_EMU
	mac_init();								// init the vmac and start the radio
#endif
}

/**
 * @brief functions for handling reset
 */
//void sos_watchdog_processing() __attribute__ ((naked)) __attribute__ ((section (".init3")));
void sos_watchdog_processing()
{
	return;
}

//-------------------------------------------------------------------------
// MAIN
//-------------------------------------------------------------------------
int main(void)
{
	oscc_init();							// Oscillator
	PSSR = (PSSR_RDH | PSSR_PH);   			// Reenable the GPIO buffers (needed out of reset)
	//SOS_SET_PIN_DIRECTIONS();				// GPIO
	DVFS_SwitchCoreFreq(104, 104);			// Switch to 104MHz mode
	//DVFS_SwitchCoreFreq(13, 13);			// Switch to 13MHz mode
	memory_init();							// SDRAM
	mmu_init();								// MMU
	//gpio_init();							// GPIO Interrupts
	//wdt_init();							// WATCHDOG
	PXA27XGPIOInt_init();
	pmic_init();
	led_init();								// LEDS
	//systime_init();						// LOCAL TIME
	timer_hardware_init(
		DEFAULT_INTERVAL, DEFAULT_SCALE);	// SYSTEM TIMER
	sos_main(SOS_BOOT_NORMAL);
	return 0;
}
