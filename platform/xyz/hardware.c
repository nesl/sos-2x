/* ex: set ts=4: */
/*
 * Copyright (c) 2005 Yale University.
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

#include <hardware.h>
#include <flash.h>
#include <wdt.h>
#include <kertable.h>
#include <kertable_proc.h>

#ifndef NO_SOS_UART
#include <uart_system.h>
#include <sos_uart.h>
#endif

/**
 * @brief Kernel jump table
 * The table entries are defined in kertable.h
 */
#if defined(PROC_KER_TABLE) && defined(PLAT_KER_TABLE)
PGM_VOID_P ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(CONCAT_TABLES(PROC_KER_TABLE,PLAT_KER_TABLE));
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

/**
 * Low level initialization for the platform running SOS
 * This must include timer_hardware_init() and uart_hardware_init().
 */
void hardware_init(void)
{
	//oki_init()
	put_wvalue(CGBCNT0, 0x0000003C);					// unlock code
	put_wvalue(CGBCNT0, 0x00000000); 					// 58 MHz

    //! Start watch dog timer???
	//wdt_hardware_init();

	flash_init();

    //! Initialize platform specific subsystems
    led_init();

	ENABLE_CACHE();

    //! Initialize the external SDRAM
	sdram_init();

	systime_init();
    timer_hardware_init(DEFAULT_INTERVAL, DEFAULT_SCALE);

    //uart_hardware_init();
  // UART
  uart_system_init();
#ifndef NO_SOS_UART
  //! Initalize uart comm channel
  sos_uart_init();
#endif


    adc_proc_init();

    //! initialize zigbee radio
    xyz_zigbee_radio_init();

    // Initializing the on-board sensor drivers
    photosensor_init();
}

/**
 * Main function
 */
int main(void)
{
    sos_main(SOS_BOOT_NORMAL);
    return 0;
}


