/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */
/**
 * @brief  hardware related routines
 * @author Naim Busek (ndbusek@lecs.cs.ucla.edu)
 *
 */

#include "hardware.h"
#include <flash.h>
#include <kertable.h>
#include <kertable_proc.h>
#include <kertable_plat.h>

#ifndef NO_SOS_UART
#include <sos_uart.h>
#endif

#ifndef NO_SOS_I2C
#include <sos_i2c.h>
#endif

#ifdef SOS_SFI
#include <sfi_jumptable.h>
#endif

//----------------------------------------------------------------------------
//  Typedefs
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//  Global data declarations
//----------------------------------------------------------------------------
static uint8_t reset_flag NOINIT_VAR;
/**
 * @brief Kernel jump table
 * The table entries are defined in kertable.h
 */

#ifdef SOS_SFI
PGM_VOID_P ker_jumptable[128] PROGMEM = SOS_SFI_KER_TABLE;
#else
#if defined(PROC_KER_TABLE) && defined(PLAT_KER_TABLE)
PGM_VOID_P ker_jumptable[128] PROGMEM =
SOS_KER_TABLE( CONCAT_TABLES(PROC_KER_TABLE , PLAT_KER_TABLE) );
#else
#error PROC_KER_TABLE and PLAT_KER_TABLE must be defined
#endif
#endif//SOS_SFI


static inline void protosb_hardware_init() {
	ltc6915_amp_hardware_init();
};


static inline void protosb_peripheral_init() {
	adg715_mux_init();
	ltc6915_amp_init();
	vref_init();
	ads8341_adc_init();
	preamp_init();
	switches_init();
	vreg_init();
};


//----------------------------------------------------------------------------
//  Funcation declarations
//----------------------------------------------------------------------------

void hardware_init(void) {
  init_IO();

	EIMSK = 0;  // turn off external interrupts
 
#ifndef DISABLE_WDT
	/*
	 * Hardware Fuse Bit ensures WDT is always ON
	 * The following timed sequence sets up a WDT
	 * with the longest timeout period.
	 */
	watchdog_reset();
	WDTCR = (1 << WDCE) | (1 << WDE);
	WDTCR = (1 << WDE) | (1 << WDP2) | (1 << WDP1) | (1 << WDP0);
#else
	/*
	 * WDT may need to be disabled during debugging etc.
	 * You must also unset the WDT fuse.  If it is set it 
	 * is not possible to disable the WDT in software.
	 * Setting the fuses to ff9fff will allow it to be disabled.
	 */

	watchdog_reset();
	WDTCR = (1 << WDCE) | (1 << WDE);
	WDTCR = 0;
#endif //DISABLE_MICA2_WDT

	//! component level init
	systime_init();

	timer_hardware_init(DEFAULT_INTERVAL, DEFAULT_SCALE);

	// SPI
	spi_system_init();

	// UART
	uart_system_init();
#ifndef NO_SOS_UART
	//! Initalize uart comm channel
	sos_uart_init();
#endif

	// I2C
  i2c_system_init();
#ifndef NO_SOS_I2C
	//! Initalize i2c comm channel
	sos_i2c_init();
#endif

	// init processor ADC
  adc_proc_init();

	//! protoSB Peripheral Init
	protosb_hardware_init();
	protosb_peripheral_init();
}


void init_IO(void)
{
	/* set io pins to be compatable with mica2 */
	/* setting all shared pins */

	/* local pins driving hardware on sensor board */
  SET_VREF_SHDN_DD_OUT();
  SET_PROTO_SHDN_DD_OUT();
  SET_ADC_CS_DD_OUT();
  SET_AMP_CS_DD_OUT();
  SET_PREAMP_SHDN_DD_OUT(); 
  SET_SH15_CLK_DD_OUT();
  SET_ADC_SHDN_DD_OUT();
	SET_ADC_BUSY_DD_IN();
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
	// uint8_t booting_cond;
	reset_flag = MCUCSR & 0x1F;
	MCUCSR = 0;

	/**
	 * Check for watchdog reset, and resset due to 
	 * illegal addressing (reset_flag == 0)
	 */
/*	if(reset_flag == 0) {
		booting_cond = SOS_BOOT_CRASHED;
	} else {
		booting_cond = SOS_BOOT_NORMAL;
	}
	*/
	/*
	if(booting_cond != SOS_BOOT_NORMAL) {
		sched_post_crash_checkup();
	}
*/
}
#endif

int main(void)
{
	sos_main(SOS_BOOT_NORMAL);
	return SOS_OK;
}
