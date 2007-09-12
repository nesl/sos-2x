/* -*- Mode: C; tab-width:2 -*- */
/* ex: set ts=2 shiftwidth=2 softtabstop=2 cindent: */

/**
 * @file hardware.c
 * @brief  Platform specific file for TMote Sky nodes
 * @author Ram Kumar {ram@ee.ucla.edu}
 * @author Au Lawrence
 */

#include <hardware.h>           // platform/tmote/include/
//#include <kertable.h>           // sos-1.x/kernel/include/
//#include <kertable_proc.h>      // processor/msp430/include/
#include <pin_alt_func.h>       // processor/msp430/include/


#include <msp430/usart.h>


#define LED_DEBUG
#include <led_dbg.h>

#ifndef NO_SOS_UART
#include <uart_system.h>
#include <sos_uart.h>
#include <uart_hal.h>
#endif

#ifndef NO_KERTABLE_PLAT
#include <kertable_plat.h>      // platform/tmote/include/
#endif

#ifdef NEW_SENSING_API
#ifdef TMOTE_INVENT_SENSOR_BOARD
#include <invent_sensor_init.h>
#endif
#endif

//----------------------------------------------------------------------------
//  GLOBAL DATA 
//----------------------------------------------------------------------------
/**
 * @brief Kernel jump table
 * The table entries are defined in kertable.h
 */
//const char* ker_jumptable[128] PROGMEM = SOS_KER_TABLE(KER_TABLE_MSP430);
/*
#if defined(PROC_KER_TABLE) && defined(PLAT_KER_TABLE)
const char* ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(CONCAT_TABLES(PROC_KER_TABLE,PLAT_KER_TABLE));
#elif defined(PROC_KER_TABLE)
const char* ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(PROC_KER_TABLE);
#elif defined(PLAT_KER_TABLE)
const char* ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(PLAT_KER_TABLE);
#else
const char* ker_jumptable[128] PROGMEM =
SOS_KER_TABLE(NULL);
#endif
*/

// Initialize the SHT1x driver for new sensing system
// and Tmote Sky sensor board.
#ifdef NEW_SENSING_API
#ifdef TMOTE_SENSOR_BOARD
extern int8_t sht1x_comm_init();
#endif
#endif

//-------------------------------------------------------------------------
// FUNCTION DECLARATION
//-------------------------------------------------------------------------
void hardware_init(void){
	init_IO();

	// WATCHDOG SETUP FOR MSP430
	// After PUC, watchdog is enabled by default
	// with a timeout of 32 ms. Till we support
	// watchdog, we will simply disable it
	DISABLE_WDT();

	// CLOCK SUBSYSTEM
	clock_hal_init();

	// LEDS
	led_init();

	// HARDWARE TIMERS
	timerb_hal_init();

	// SYSTEM TIMER
	timer_hardware_init(DEFAULT_INTERVAL);

	// UART
	uart_hardware_init();
	uart_system_init();
#ifndef NO_SOS_UART
	//! Initialize uart comm channel
	sos_uart_init();
#endif

	// I2C
	//! Limited I2C support for the ARL deployment
#ifdef NEW_SENSING_API
#ifdef TMOTE_INVENT_SENSOR_BOARD
	invent_sensor_init();
#endif
#endif

	// SPI
	spi_hardware_init();

	// RADIO
	//#ifndef NO_SOS_RADIO
	//	cc2420_hardware_init();
	mac_init();
	//#endif

	// ADC
#ifdef NEW_SENSING_API
	adc_driver_init();
#else
	adc_proc_init();
#endif

	// Interrupt controller
	interrupt_init();

	// SHT1x chip communication controller
#ifdef NEW_SENSING_API
#ifdef TMOTE_SENSOR_BOARD
	sht1x_comm_init();
#endif
#endif

	// Ram - I dont know which flash this is ?
	//  init_flash();



	// EXTERNAL FLASH
	// Currently there is no support

	// MSP430 PERIPHERALS (Optional)

}



// send a bit via bit-banging to the flash
#define FLASH_M25P_DP_bit(set) \
{ (set)?SET_SIMO0():CLR_SIMO0(); SET_UCLK0(); CLR_UCLK0(); }

// put the flash into deep sleep mode
// important to do this by default
void FLASH_M25P_DP() {
  //  SIMO0, UCLK0
  SET_SIMO0_DD_OUT();
  SET_UCLK0_DD_OUT();
  SET_FLASH_HOLD_DD_OUT();
  SET_FLASH_CS_DD_OUT();
  SET_FLASH_HOLD();
  SET_FLASH_CS();

  TOSH_wait();

  // initiate sequence;
  CLR_FLASH_CS();
  CLR_UCLK0();

  FLASH_M25P_DP_bit(1);  // 0
  FLASH_M25P_DP_bit(0);  // 1
  FLASH_M25P_DP_bit(1);  // 2
  FLASH_M25P_DP_bit(1);  // 3
  FLASH_M25P_DP_bit(1);  // 4
  FLASH_M25P_DP_bit(0);  // 5
  FLASH_M25P_DP_bit(0);  // 6
  FLASH_M25P_DP_bit(1);  // 7

  SET_FLASH_CS();

  SET_SIMO0();
  SET_SIMO0_DD_IN();
  SET_UCLK0_DD_IN();
  CLR_FLASH_HOLD();
}

void init_IO(void)
{
  // reset all of the ports to be input and using i/o functionality
  P1SEL = 0;
  P2SEL = 0;
  P3SEL = 0;
  P4SEL = 0;
  P5SEL = 0;
  P6SEL = 0;

  P1DIR = 0xe0;
  P1OUT = 0x00;

  P2DIR = 0x7b;
  P2OUT = 0x10;

  P3DIR = 0xf1;
  P3OUT = 0x00;

  P4DIR = 0xfd;
  P4OUT = 0xdd;

  P5DIR = 0xff;
  P5OUT = 0xff;

  P6DIR = 0xff;
  P6OUT = 0x00;

  P1IE = 0;
  P2IE = 0;
  // the commands above take care of the pin directions
  // there is no longer a need for explicit set pin
  // directions using the TOSH_SET/CLR macros
  // wait 10ms for the flash to startup
  TOSH_uwait(1024*10);
  // Put the flash in deep sleep state
  FLASH_M25P_DP();
}


//-------------------------------------------------------------------------
// MAIN
//-------------------------------------------------------------------------
int main(void)
{
	led_init();
  // ALIVE INDICATOR

  sos_main(SOS_BOOT_NORMAL);
  return 0;
}

